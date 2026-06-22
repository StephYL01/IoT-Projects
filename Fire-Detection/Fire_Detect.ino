/*
 * -------------------------------------------------------------------
 * IoT Automotive Fire & Gas Detection System
 * -------------------------------------------------------------------
 * TECHNICAL OVERVIEW:
 * - Sensors: DHT22 (Temp/Humidity) & MQ-2 (Gas/Smoke).
 * - Networking: Asynchronous WiFi connection with automatic reconnect.
 * - Cloud: Firebase Realtime Database integration (Temp C/F/K, Hum, Gas).
 * - UI Logic: Alternating 16x2 LCD screens to manage limited real estate.
 * - System Safety: Offline-First design; alarms function without WiFi.
 * -------------------------------------------------------------------
 */

#include <WiFi.h>
#include <FirebaseESP32.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>
#include "DHT.h"
#include "secrets.h" // Sensitive credentials (SSID, Pass, Tokens) stored externally

// --- HARDWARE PIN DEFINITIONS ---
#define I2C_SDA       22
#define I2C_SCL       23
#define LED_PIN       15
#define BUZZER_PIN    2
#define DHT_PIN       5
#define MQ2_PIN       34    // ADC1 pin mandatory for ESP32 WiFi functionality
#define DHT_TYPE      DHT22

// --- THRESHOLDS ---
const float TEMP_THRESHOLD = 50.0;
const int GAS_THRESHOLD = 1800;

// --- INITIALIZATION ---
LiquidCrystal_I2C lcd(0x27, 16, 2);
DHT dht(DHT_PIN, DHT_TYPE);
FirebaseData firebaseData;
FirebaseConfig config;
FirebaseAuth auth;

// Custom 5x8 pixel character stored in HD44780 CGRAM
byte smileyFace[8] = { B00000, B01010, B01010, B00000, B10001, B01110, B00000, B00000 };

int melody[] = {392, 392, 392, 311, 466, 392, 311, 466, 392};
int noteDurations[] = {500, 500, 500, 350, 150, 500, 350, 150, 1000};

unsigned long lastBlink = 0;
unsigned long lastScreenChange = 0;
bool ledState = false;
bool showMainScreen = true;

void setup() {
  Serial.begin(115200);
  Wire.begin(I2C_SDA, I2C_SCL);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);

  lcd.init();
  lcd.backlight();
  lcd.createChar(0, smileyFace);
  dht.begin();

  // Firebase Config
  config.host = FIREBASE_HOST;
  config.signer.tokens.legacy_token = FIREBASE_AUTH;
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Non-blocking WiFi start
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  lcd.print("System Ready...");
  delay(1000);
  lcd.clear();
}

void loop() {
  // --- HEARTBEAT LOGIC ---
  // Signals that the CPU is executing the loop correctly.
  if (millis() - lastBlink >= 1000) {
    lastBlink = millis();
    ledState = !ledState;
    digitalWrite(LED_PIN, ledState);
  }

  // --- SENSOR ACQUISITION ---
  float tempC = dht.readTemperature();
  float hum = dht.readHumidity();
  int gas = analogRead(MQ2_PIN);

  // Error check for DHT22 (Requires 2s interval)
  if (isnan(tempC) || isnan(hum)) return;

  float tempF = (tempC * 1.8) + 32.0;
  float tempK = tempC + 273.15;
  bool wifiStatus = (WiFi.status() == WL_CONNECTED);

  // --- CLOUD SYNC ---
  if (wifiStatus) {
    Firebase.setFloat(firebaseData, "/fire_system/temp_C", tempC);
    Firebase.setFloat(firebaseData, "/fire_system/temp_F", tempF);
    Firebase.setFloat(firebaseData, "/fire_system/temp_K", tempK);
    Firebase.setFloat(firebaseData, "/fire_system/humidity", hum);
    Firebase.setInt(firebaseData, "/fire_system/gas", gas);
  }

  // --- ALARM LOGIC ---
  if (tempC >= TEMP_THRESHOLD || gas >= GAS_THRESHOLD) {
    triggerAlarm();
  } else {
    // --- LCD UI MANAGEMENT ---
    // Toggle between screens every 3 seconds to optimize 16x2 space.
    if (millis() - lastScreenChange > 3000) {
      lastScreenChange = millis();
      showMainScreen = !showMainScreen;
      lcd.clear(); // Only clear when switching to avoid screen flickering
    }

    if (showMainScreen) {
      lcd.setCursor(0, 0);
      lcd.print("T:"); lcd.print(tempC, 1); lcd.print("C ");
      lcd.print("H:"); lcd.print(hum, 0); lcd.print("%");
      lcd.setCursor(0, 1);
      lcd.print("Gas:"); lcd.print(gas);
      lcd.setCursor(15, 1); lcd.write(0); // Smiley in bottom-right corner
    } else {
      lcd.setCursor(0, 0);
      lcd.print(wifiStatus ? "WiFi: Connected " : "WiFi: DISCON.   ");
      lcd.setCursor(0, 1);
      lcd.print(wifiStatus ? "RSSI:" + String(WiFi.RSSI()) + "dBm" : "Offline Mode    ");
      lcd.setCursor(15, 1); lcd.write(0);
    }
  }

  // DHT22 sampling interval explanation:
  // Sensors of this type require a minimum of 2 seconds for internal 
  // capacitive stabilization. Polling faster causes sensor drift/errors.
  delay(2000);
}

void triggerAlarm() {
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("! FIRE DANGER !");
  for (int i = 0; i < 9; i++) {
    lcd.noBacklight();
    tone(BUZZER_PIN, melody[i], noteDurations[i]);
    delay(noteDurations[i] + 50);
    lcd.backlight();
    delay(100);
  }
  lcd.clear();
}
