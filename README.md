# ESP32 Ultrasonic Radar
 
A self-contained 180-degree ultrasonic radar built with an ESP32, HC-SR04 distance sensor, SG90 servo motor, and a 0.96" OLED display. The servo sweeps the sensor across a 150-degree arc (15-165°) and renders a live radar visualization directly on the OLED — no PC or external software required.
 
## Hardware
 
- Elegoo ESP32 Board
- HC-SR04 Ultrasonic Sensor
- SG90 Servo Motor
- 0.96" SSD1306 OLED Display (I2C)
- 400-point Breadboard
- Jumper Wires
## Software
 
- Arduino IDE (ESP32 board package)
- Libraries: `ESP32Servo`, `Adafruit GFX`, `Adafruit SSD1306`
## How It Works
 
1. The servo sweeps from 15° to 165° and back in a continuous loop
2. At each angle, the HC-SR04 fires an ultrasonic pulse and calculates distance from the echo return time
3. Detections within 40cm are stored per angle in a 181-element array
4. The OLED updates every step, drawing range rings, sweep reference lines, a live sweep line, and shaded detection blips from the object outward to the radar edge
5. Current angle and distance are shown as a text readout in the top corners
## Wiring
 
| Component | ESP32 Pin |
|---|---|
| HC-SR04 VCC | 5V |
| HC-SR04 GND | GND |
| HC-SR04 TRIG | GPIO 27 |
| HC-SR04 ECHO | GPIO 14 |
| SG90 Signal | GPIO 13 |
| SG90 VCC | 5V (external recommended) |
| SG90 GND | GND |
| OLED SDA | GPIO 32 |
| OLED SCL | GPIO 33 |
 
## Setup
 
1. Install Arduino IDE and add the ESP32 board package via Boards Manager
2. Install libraries via Library Manager: `ESP32Servo`, `Adafruit GFX Library`, `Adafruit SSD1306`
3. Wire components as above
4. Flash `src/radar.ino` to the ESP32
5. Power on — the radar starts sweeping immediately with no PC connection needed
## Repository Structure
 
```
esp32-ultrasonic-radar/
├── src/
│   └── radar.ino
└── README.md
```

## Skills Demonstrated
 
- Embedded C++ (Arduino/ESP32 framework)
- PWM servo control
- Ultrasonic time-of-flight distance sensing
- I2C communication with OLED display
- Real-time graphics rendering on constrained hardware (128x64 display, no GPU)
