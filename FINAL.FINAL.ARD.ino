#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include <SoftwareSerial.h>

// ---------- LCD & ESP SERIAL ----------
LiquidCrystal_I2C lcd(0x27, 16, 2);
SoftwareSerial espSerial(2, 3);   // RX=2, TX=3 -> connect to ESP8266

// ---------- SENSOR PINS ----------
const int mq135Pin = A0;     // MQ-135 Air Quality
const int mq7Pin   = A1;     // MQ-7 CO Gas
const int soundPin = A2;     // KY-037 Sound

// ---------- ALERT PINS ----------
const int BUZZER_PIN = 8;    // single buzzer for both gases
const int LED_PIN    = 9;    // Sound LED

// ---------- GLOBALS ----------
String soundStatus = "";
String airStatus   = "";
String coStatus    = "";

int mq135Severity = 0;   // 0..3
int mq7Severity   = 0;   // 0..3

float g_mq135_ppm = 0.0; // scaled 0–5000 for thresholds
int   g_mq7_raw   = 0;   // scaled 0–300 for thresholds
int   g_sound_raw = 0;
int   g_sound_db  = 0;

unsigned long currentMillis       = 0;
unsigned long lastReadTime        = 0;
const unsigned long readInterval  = 1000;   // 1 s

unsigned long lastCloudTime       = 0;
const unsigned long cloudInterval = 10000;  // 10 s -> ESP

unsigned long lastLcdPageTime     = 0;
const unsigned long lcdPageInterval = 3000; // 3 s

unsigned long lastLedTime         = 0;
unsigned long lastBuzzerTime      = 0;

bool ledState    = LOW;
bool buzzerState = LOW;

int displayMode = 0;

// ---------- SETUP ----------
void setup() {
  Serial.begin(9600);      // PC debug
  espSerial.begin(9600);   // ESP8266 link

  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  digitalWrite(LED_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);

  lcd.begin();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Env. Monitering");
  lcd.setCursor(0, 1);
  lcd.print("Warming up...");

  for (int i = 30; i > 0; i--) {
    lcd.setCursor(0, 1);
    lcd.print("Wait: ");
    lcd.print(i);
    lcd.print("s   ");
    delay(1000);
  }

  lcd.clear();
  lcd.print("System Ready!");
  delay(1500);
}

// ---------- MAIN LOOP ----------
void loop() {
  currentMillis = millis();

  // 1) Read sensors + status every 1 s
  if (currentMillis - lastReadTime >= readInterval) {
    lastReadTime = currentMillis;
    readSensors();
    determineStatus();

    Serial.print("MQ135: "); Serial.print(g_mq135_ppm, 1);
    Serial.print(" | MQ7: "); Serial.print(g_mq7_raw);
    Serial.print(" | dB: "); Serial.println(g_sound_db);
  }

  // 2) Local alerts
  handleSoundAlerts();   // LED based on sound
  handleBuzzerLogic();   // single buzzer based on max severity

  // 3) LCD pages
  updateLCD();

  // 4) Send to ESP every 10 s as CSV
  if (currentMillis - lastCloudTime >= cloudInterval) {
    lastCloudTime = currentMillis;
    sendDataToESP();
  }
}

// ---------- SENSOR READING ----------
void readSensors() {
  // MQ-135: map raw 0–1023 to approx 0–5000 “ppm-like”
  int mq135_raw = analogRead(mq135Pin);
  g_mq135_ppm = map(mq135_raw, 100, 800, 0, 5000);   // adjust 100/800 after observing real raw values
  if (g_mq135_ppm < 0) g_mq135_ppm = 0;

  // MQ-7: map raw 0–1023 to 0–300 scale
  int mq7_adc = analogRead(mq7Pin);
  g_mq7_raw = map(mq7_adc, 50, 800, 0, 300);         // adjust 50/800 for your sensor
  if (g_mq7_raw < 0) g_mq7_raw = 0;

  // Sound: envelope + dB mapping
  g_sound_raw = getSoundEnvelope();
  g_sound_db = map(g_sound_raw, 0, 300, 0, 300);    // adjust 300 based on your max envelope
  if (g_sound_db < 40)  g_sound_db = 40;
  if (g_sound_db > 120) g_sound_db = 120;
}

// ---------- STATUS LOGIC (YOUR THRESHOLDS) ----------
void determineStatus() {
  // 1. Sound
  if (g_sound_db <= 80)        soundStatus = "Good";
  else if (g_sound_db <= 120)  soundStatus = "High";
  else                         soundStatus = "V.High";

  // 2. MQ-135 Air
  if (g_mq135_ppm <= 600) {
    airStatus = "Excellent"; mq135Severity = 0;
  } else if (g_mq135_ppm <= 800) {
    airStatus = "Good";      mq135Severity = 0;
  } else if (g_mq135_ppm <= 1200) {
    airStatus = "Fair";      mq135Severity = 1;
  } else if (g_mq135_ppm <= 2000) {
    airStatus = "Bad";       mq135Severity = 2;
  } else if (g_mq135_ppm <= 4000) {
    airStatus = "V.Bad";     mq135Severity = 2;
  } else {
    airStatus = "Hazard";    mq135Severity = 3;
  }

  // 3. MQ-7 CO
  if (g_mq7_raw <= 10) {
    coStatus = "Excellent"; mq7Severity = 0;
  } else if (g_mq7_raw <= 51) {
    coStatus = "Good";      mq7Severity = 0;
  } else if (g_mq7_raw <= 100) {
    coStatus = "Fair";      mq7Severity = 1;
  } else if (g_mq7_raw <= 199) {
    coStatus = "Bad";       mq7Severity = 2;
  } else if (g_mq7_raw <= 300) {
    coStatus = "V.Bad";     mq7Severity = 2;
  } else {
    coStatus = "Hazard";    mq7Severity = 3;
  }
}

// ---------- LED: SOUND ALERT ----------
void handleSoundAlerts() {
  if (g_sound_db <= 80) {
    digitalWrite(LED_PIN, LOW);
    ledState = LOW;
  } else {
    long interval = (g_sound_db <= 120) ? 300 : 100;
    if (currentMillis - lastLedTime >= interval) {
      lastLedTime = currentMillis;
      ledState = !ledState;
      digitalWrite(LED_PIN, ledState);
    }
  }
}

// ---------- SINGLE BUZZER: MQ135 + MQ7 ----------
void handleBuzzerLogic() {
  int maxSeverity = max(mq135Severity, mq7Severity);

  if (maxSeverity == 0) {
    noTone(BUZZER_PIN);
    buzzerState = LOW;
  }
  else if (maxSeverity == 1) {
    // 1 beep every 2 s
    unsigned long cycle = currentMillis % 2000;
    if (cycle < 200) tone(BUZZER_PIN, 1000);
    else noTone(BUZZER_PIN);
  }
  else if (maxSeverity == 2) {
    // 4 beeps per second
    if (currentMillis - lastBuzzerTime >= 125) {
      lastBuzzerTime = currentMillis;
      buzzerState = !buzzerState;
      if (buzzerState) tone(BUZZER_PIN, 1000);
      else noTone(BUZZER_PIN);
    }
  }
  else if (maxSeverity == 3) {
    // continuous
    tone(BUZZER_PIN, 1000);
  }
}

// ---------- LCD PAGES ----------
void updateLCD() {
  if (currentMillis - lastLcdPageTime >= lcdPageInterval) {
    lastLcdPageTime = currentMillis;
    lcd.clear();

    if (displayMode == 0) {
      lcd.setCursor(0, 0); lcd.print("Air:"); lcd.print(airStatus);
      lcd.setCursor(0, 1); lcd.print("PPM:"); lcd.print(g_mq135_ppm, 0);
    } else if (displayMode == 1) {
      lcd.setCursor(0, 0); lcd.print("CO:"); lcd.print(coStatus);
      lcd.setCursor(0, 1); lcd.print("Raw:"); lcd.print(g_mq7_raw);
    } else {
      lcd.setCursor(0, 0); lcd.print("Sound:"); lcd.print(soundStatus);
      lcd.setCursor(0, 1); lcd.print("dB:"); lcd.print(g_sound_db);
    }

    displayMode = (displayMode + 1) % 3;
  }
}

// ---------- SEND CSV TO ESP8266 ----------
void sendDataToESP() {
  String dataString = String(g_mq135_ppm, 1) + "," +
                      String(g_mq7_raw) + "," +
                      String(g_sound_db);

  espSerial.println(dataString);              // to ESP
  Serial.print("SENT TO ESP: ");             // debug to PC
  Serial.println(dataString);
}

// ---------- SOUND ENVELOPE ----------
int getSoundEnvelope() {
  unsigned int signalMax = 0;
  unsigned int signalMin = 1023;
  unsigned long startTime = millis();
  while (millis() - startTime < 50) {
    int sample = analogRead(soundPin);
    if (sample > signalMax) signalMax = sample;
    if (sample < signalMin) signalMin = sample;
  }
  return signalMax - signalMin;
}
