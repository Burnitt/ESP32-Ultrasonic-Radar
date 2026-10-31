#include <ESP32Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

const int trigPin = 27;
const int echoPin = 14;
const int servoPin = 13;
Servo myServo;

int objDistance[181]; // stores last known distance per angle, 0 = nothing detected

// ---- test additions ----
unsigned long lastEchoUs = 0;   // raw echo pulse width from the last ping (us)
float lastDistanceCm = 0;       // distance with decimals
bool holdMode = false;          // true = servo parked at holdAngle
int holdAngle = 90;

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.begin(115200);

  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);

  Wire.begin(32, 33); // SDA, SCL

  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for (;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  clearDetections();
}

void loop() {
  if (holdMode) {
    runStep(holdAngle);
    return;
  }

  clearDetections();
  for (int i = 15; i <= 165 && !holdMode; i++) {
    runStep(i);
  }

  clearDetections();
  for (int i = 165; i > 15 && !holdMode; i--) {
    runStep(i);
  }
}

// One radar step: check for commands, move, ping, report, draw
void runStep(int angle) {
  checkSerial();
  if (holdMode) angle = holdAngle;

  myServo.write(angle);
  delay(30);
  int distance = calculateDistance();
  sendData(angle);
  updateOLED(angle, distance);
}

// Commands from the PC: "H90" = hold at 90 degrees, "S" = resume sweep
void checkSerial() {
  if (!Serial.available()) return;
  String cmd = Serial.readStringUntil('\n');
  cmd.trim();
  if (cmd.startsWith("H")) {
    holdAngle = constrain(cmd.substring(1).toInt(), 0, 180);
    holdMode = true;
  } else if (cmd == "S") {
    holdMode = false;
  }
}

void clearDetections() {
  for (int i = 0; i < 181; i++) {
    objDistance[i] = 0;
  }
}

int calculateDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  lastEchoUs = pulseIn(echoPin, HIGH, 30000);   // 30 ms timeout (Test 4 finding)
  lastDistanceCm = lastEchoUs * 0.0343 / 2.0;   // speed of sound at ~20 C (Test 2 finding)
  return (int)lastDistanceCm;
}

// One CSV line per reading: angle,distance_cm,echo_us
void sendData(int angle) {
  Serial.print(angle);
  Serial.print(",");
  Serial.print(lastDistanceCm, 2);
  Serial.print(",");
  Serial.println(lastEchoUs);
}

void updateOLED(int angle, int dist) {
  if (dist < 40 && dist > 0) {
    objDistance[angle] = dist;
  } else {
    objDistance[angle] = 0;
  }

  display.clearDisplay();

  int centerX = 64;
  int centerY = 60;
  int maxRadius = 55;

  for (int r = 15; r <= maxRadius; r += 20) {
    for (int a = 15; a <= 165; a += 2) {
      float rad = radians(a);
      int x = centerX - r * cos(rad);
      int y = centerY - r * sin(rad);
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }

  float rad15 = radians(15);
  float rad165 = radians(165);
  display.drawLine(centerX, centerY, centerX - maxRadius * cos(rad15), centerY - maxRadius * sin(rad15), SSD1306_WHITE);
  display.drawLine(centerX, centerY, centerX - maxRadius * cos(rad165), centerY - maxRadius * sin(rad165), SSD1306_WHITE);
  display.drawLine(centerX, centerY, centerX, centerY - maxRadius, SSD1306_WHITE);

  for (int a = 15; a <= 165; a++) {
    if (objDistance[a] > 0) {
      float rad = radians(a);
      int objRadius = map(objDistance[a], 0, 40, 0, maxRadius);
      int objX = centerX - objRadius * cos(rad);
      int objY = centerY - objRadius * sin(rad);
      int edgeX = centerX - maxRadius * cos(rad);
      int edgeY = centerY - maxRadius * sin(rad);
      display.drawLine(objX, objY, edgeX, edgeY, SSD1306_WHITE);
    }
  }

  float sweepRad = radians(angle);
  int sweepX = centerX - maxRadius * cos(sweepRad);
  int sweepY = centerY - maxRadius * sin(sweepRad);
  display.drawLine(centerX, centerY, sweepX, sweepY, SSD1306_WHITE);

  display.setTextSize(1);
  display.setCursor(0, 0);
  display.print(angle);
  display.print((char)247);
  display.setCursor(95, 0);
  if (dist < 40 && dist > 0) {
    display.print(dist);
    display.print("cm");
  } else {
    display.print("--");
  }

  display.display();
}