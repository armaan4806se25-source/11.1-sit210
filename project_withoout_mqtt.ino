
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 32
#define OLED_RESET -1
#define OLED_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
Servo gateServo;

const int FLOW_PIN = 2;
const int LEVEL_PIN = A0;
const int RAIN_PIN = A1;
const int BUZZER_PIN = 7;
const int SERVO_PIN = 9;
const int PUMP_PIN = 10;

const float NORMAL_LEVEL = 45.0;
const float CRITICAL_LEVEL = 100.0;
const float MAX_WATER_MM = 100.0;

// Replace these with measured calibration readings.
const int LEVEL_ADC_AT_0MM = 15;
const int LEVEL_ADC_AT_70MM = 255;

const float FLOW_PULSES_PER_LITRE = 450.0;

const int RAIN_THRESHOLD = 600;

volatile unsigned long flowPulseCount = 0;

float waterLevelMM = 0;
float flowRate = 0;

bool pumpOn = false;
bool buzzerOn = false;
bool gateOpen = false;
bool rainDetected = false;

unsigned long previousFlowTime = 0;
unsigned long previousDisplayTime = 0;

void flowPulse() {
  flowPulseCount++;
}

float readWaterLevel() {
  int adcValue = analogRead(LEVEL_PIN);

  float level = (float)(adcValue - LEVEL_ADC_AT_0MM) *
                MAX_WATER_MM /
                (LEVEL_ADC_AT_70MM - LEVEL_ADC_AT_0MM);

  level = constrain(level, 0, MAX_WATER_MM);

  return level;
}

String getFloodStatus() {
  if (waterLevelMM >= CRITICAL_LEVEL) {
    return "CRITICAL";
  } else if (waterLevelMM > NORMAL_LEVEL) {
    return "WARNING";
  } else {
    return "NORMAL";
  }
}

void controlOutputs() {
  bool critical = (waterLevelMM >= CRITICAL_LEVEL);

  pumpOn = critical;
  buzzerOn = critical;
  gateOpen = critical;

  digitalWrite(PUMP_PIN, pumpOn ? HIGH : LOW);

  if (buzzerOn) {
    tone(BUZZER_PIN, 2000);
  } else {
    noTone(BUZZER_PIN);
  }

  gateServo.write(gateOpen ? 180 : 90);
}

void calculateFlowRate() {
  unsigned long currentTime = millis();

  if (currentTime - previousFlowTime >= 1000) {
    noInterrupts();
    unsigned long pulses = flowPulseCount;
    flowPulseCount = 0;
    interrupts();

    flowRate = (pulses / FLOW_PULSES_PER_LITRE) * 60.0;

    previousFlowTime = currentTime;
  }
}

void updateRainSensor() {
  int rainValue = analogRead(RAIN_PIN);

  rainDetected = (rainValue < RAIN_THRESHOLD);
}

void updateOLED() {
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  display.setCursor(0, 0);
  display.print("Water: ");
  display.print(waterLevelMM, 1);
  display.println(" mm");

  display.setCursor(0, 10);
  display.print("Status: ");
  display.println(getFloodStatus());

  display.setCursor(0, 20);
  display.print("Pump:");
  display.print(pumpOn ? "ON " : "OFF");
  display.print(" Gate:");
  display.print(gateOpen ? "OPEN" : "CLOSED");

  display.display();
}

void printReadings() {
  Serial.println("-------------------------");

  Serial.print("Water Level: ");
  Serial.print(waterLevelMM);
  Serial.println(" mm");

  Serial.print("Flood Status: ");
  Serial.println(getFloodStatus());

  Serial.print("Rain Detected: ");
  Serial.println(rainDetected ? "YES" : "NO");

  Serial.print("Flow Rate: ");
  Serial.print(flowRate);
  Serial.println(" L/min");

  Serial.print("Pump: ");
  Serial.println(pumpOn ? "ON" : "OFF");

  Serial.print("Buzzer: ");
  Serial.println(buzzerOn ? "ON" : "OFF");

  Serial.print("Gate: ");
  Serial.println(gateOpen ? "OPEN" : "CLOSED");
}

void setup() {
  Serial.begin(115200);

  pinMode(FLOW_PIN, INPUT_PULLUP);
  pinMode(RAIN_PIN, INPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);

  digitalWrite(PUMP_PIN, LOW);
  noTone(BUZZER_PIN);

  gateServo.attach(SERVO_PIN);
  gateServo.write(90);

  Wire.begin();

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println("OLED not detected");
  } else {
    display.clearDisplay();
    display.display();
  }

  attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowPulse, RISING);

  previousFlowTime = millis();

  Serial.println("Flood Monitoring System Started");
  Serial.println("Normal level: 25 mm");
  Serial.println("Critical level: 70 mm");
  Serial.println("MQTT and Wi-Fi disabled");
}

void loop() {
  waterLevelMM = readWaterLevel();

  updateRainSensor();
  calculateFlowRate();

  controlOutputs();

  if (millis() - previousDisplayTime >= 1000) {
    previousDisplayTime = millis();

    printReadings();
    updateOLED();
  }

  delay(100);
}
