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
long duration;
int distance;
Servo myServo;

int objDistance[181]; // stores last known distance per angle, 0 = nothing detected

void setup() {
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  Serial.begin(115200);

  myServo.setPeriodHertz(50);
  myServo.attach(servoPin, 500, 2400);

  Wire.begin(32, 33); // SDA, SCL

  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    for(;;);
  }
  display.clearDisplay();
  display.setTextColor(SSD1306_WHITE);

  for (int i = 0; i < 181; i++) { // Reseting the graph
    objDistance[i] = 0;
  }
}

void loop() {
  // Clear the stored data before this sweep direction starts
  for (int i = 0; i < 181; i++) {
    objDistance[i] = 0;
  }

  for(int i=15; i<=165; i++){
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    sendData(i, distance);
    updateOLED(i, distance);
  }

  // Clear again before the return sweep
  for (int i = 0; i < 181; i++) {
    objDistance[i] = 0;
  }

  for(int i=165; i>15; i--){
    myServo.write(i);
    delay(30);
    distance = calculateDistance();
    sendData(i, distance);
    updateOLED(i, distance);
  }
}

int calculateDistance(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);

  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  return distance;
}

void sendData(int angle, int dist) {
  Serial.print(angle);
  Serial.print(",");
  Serial.print(dist);
  Serial.print(".");
}

void updateOLED(int angle, int dist) {
  // Store this reading so it persists across frames
  if (dist < 40 && dist > 0) {
    objDistance[angle] = dist;
  } else {
    objDistance[angle] = 0;
  }

  display.clearDisplay();

  int centerX = 64;
  int centerY = 60;
  int maxRadius = 55;

  // Draw the radar arc (range rings), only across the actual 15-165 sweep
  for (int r = 15; r <= maxRadius; r += 20) {
    for (int a = 15; a <= 165; a += 2) {
      float rad = radians(a);
      int x = centerX - r * cos(rad);
      int y = centerY - r * sin(rad);
      display.drawPixel(x, y, SSD1306_WHITE);
    }
  }

  // Draw angle reference lines at the sweep limits and center
  float rad15 = radians(15);
  float rad165 = radians(165);
  display.drawLine(centerX, centerY, centerX - maxRadius * cos(rad15), centerY - maxRadius * sin(rad15), SSD1306_WHITE);
  display.drawLine(centerX, centerY, centerX - maxRadius * cos(rad165), centerY - maxRadius * sin(rad165), SSD1306_WHITE);
  display.drawLine(centerX, centerY, centerX, centerY - maxRadius, SSD1306_WHITE); // 90 deg line

  // Draw every stored detection, shaded from the object out to the radar edge
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

  // Current sweep line (live servo position, drawn on top)
  float sweepRad = radians(angle);
  int sweepX = centerX - maxRadius * cos(sweepRad);
  int sweepY = centerY - maxRadius * sin(sweepRad);
  display.drawLine(centerX, centerY, sweepX, sweepY, SSD1306_WHITE);

  // Readout
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
