/*
  Smart Home Automation System - ESP32-S3-N16R8
  Full main firmware: relays, sensors, RTC, 20x4 I2C LCD, Blynk dashboard
  -----------------------------------------------------------------------
  Corrected pin map (July 2026 revision):
    I2C bus (LCD + RTC): SDA=G11, SCL=G12
    Safety Mode LED: G8   (moved off G12)
    WiFi status LED: G9   (moved off G11)

  Blynk virtual pin map (matched to Blynk.Console Dashboard, July 12 2026):
    V0 = Bulb     V1 = Fridge   V2 = Safety Mode
    V3 = Temp     V4 = Water    V5 = Fan        V6 = Pump

  NOTE: Replace the placeholders below with your own Blynk template ID,
  template name, auth token, WiFi SSID, and WiFi password before flashing.
  Never commit real credentials to a public repository.
*/

#define BLYNK_TEMPLATE_ID   "YOUR_BLYNK_TEMPLATE_ID"
#define BLYNK_TEMPLATE_NAME "YOUR_BLYNK_TEMPLATE_NAME"
#define BLYNK_AUTH_TOKEN    "YOUR_BLYNK_AUTH_TOKEN"

#include <WiFi.h>
#include <BlynkSimpleEsp32.h>
#include <Wire.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <LiquidCrystal_I2C.h>
#include <RTClib.h>
#include <Preferences.h>

// ---------- WiFi / Blynk credentials ----------
char ssid[] = "YOUR_WIFI_SSID";
char pass[] = "YOUR_WIFI_PASSWORD";

// ---------- Pin definitions ----------
#define PIN_RELAY_BULB    4
#define PIN_RELAY_FAN     5
#define PIN_RELAY_PUMP    6
#define PIN_RELAY_FRIDGE  7
#define PIN_DS18B20       15
#define PIN_WATER_LEVEL   1
#define PIN_PIR           16
#define PIN_BUZZER        17
#define PIN_SDA           11
#define PIN_SCL           12
#define PIN_BTN_BULB      13
#define PIN_BTN_FRIDGE    14
#define PIN_BTN_SAFETY    21
#define PIN_LED_SAFETY    8
#define PIN_LED_WIFI      9
#define PIN_BTN_MUTE      18
#define PIN_LED_FAN       47
#define PIN_LED_PUMP      48

// ---------- Relay module logic ----------
// Most 4-channel relay boards are ACTIVE-LOW: pulling the control pin LOW
// energizes the relay (load ON). Confirmed by testing, July 2026.
#define RELAY_ON  LOW
#define RELAY_OFF HIGH

// ---------- Thresholds ----------
const float TEMP_THRESHOLD_C = 29.0;
const int WATER_LOW_PERCENT = 20;
const int WATER_FULL_PERCENT = 90;

// ---------- Blynk virtual pins (matched to Blynk.Console dashboard, July 2026) ----------
#define VPIN_BULB   V0
#define VPIN_FAN    V5
#define VPIN_PUMP   V6
#define VPIN_FRIDGE V1
#define VPIN_TEMP   V3
#define VPIN_WATER  V4
#define VPIN_SAFETY V2

// ---------- Objects ----------
OneWire oneWire(PIN_DS18B20);
DallasTemperature tempSensor(&oneWire);
LiquidCrystal_I2C lcd(0x27, 20, 4);
RTC_DS1307 rtc; // swap for RTC_DS3231 if that's your module
Preferences prefs;

// ---------- State ----------
bool pumpState = false;
bool fanState = false;
bool fridgeState = false;
bool bulbState = false;
bool safetyArmed = false;

bool lastBtnSafety = HIGH;
bool lastBtnFridge = HIGH;
bool lastBtnBulb = HIGH;
bool lastBtnMute = HIGH;

bool buzzerMuted = false;
unsigned long muteUntilMs = 0;
const unsigned long MUTE_DURATION_MS = 60000; // 60 seconds

unsigned long lastLoopMs = 0;
const unsigned long LOOP_INTERVAL_MS = 300;

unsigned long lastBlynkSyncMs = 0;
const unsigned long BLYNK_SYNC_INTERVAL_MS = 2000;

// ---- WiFi non-blocking connect ----
const unsigned long WIFI_CONNECT_TIMEOUT_MS = 30000; // 30 sec, then boot offline
unsigned long lastWifiRetryMs = 0;
const unsigned long WIFI_RETRY_INTERVAL_MS = 15000; // retry every 15 sec if offline

// ---- Buzzer beep pattern (motion alarm) ----
bool buzzerBeepState = false;
unsigned long lastMotionEventMs = 0;
const unsigned long MOTION_EVENT_INTERVAL_MS = 10000; // throttle Blynk notification

bool rtcFound = false;

// ---------- Blynk write handlers (dashboard -> device) ----------
BLYNK_WRITE(VPIN_BULB) {
  bulbState = param.asInt();
  digitalWrite(PIN_RELAY_BULB, bulbState ? RELAY_ON : RELAY_OFF);
  prefs.putBool("bulb", bulbState);
}

BLYNK_WRITE(VPIN_FRIDGE) {
  fridgeState = param.asInt();
  digitalWrite(PIN_RELAY_FRIDGE, fridgeState ? RELAY_ON : RELAY_OFF);
  prefs.putBool("fridge", fridgeState);
}

BLYNK_WRITE(VPIN_SAFETY) {
  safetyArmed = param.asInt();
  digitalWrite(PIN_LED_SAFETY, safetyArmed ? HIGH : LOW);
  prefs.putBool("safety", safetyArmed);
}

void setup() {
  Serial.begin(115200);
  delay(300);

  pinMode(PIN_RELAY_BULB, OUTPUT);
  pinMode(PIN_RELAY_FAN, OUTPUT);
  pinMode(PIN_RELAY_PUMP, OUTPUT);
  pinMode(PIN_RELAY_FRIDGE, OUTPUT);
  pinMode(PIN_PIR, INPUT);
  pinMode(PIN_BUZZER, OUTPUT);
  pinMode(PIN_BTN_SAFETY, INPUT_PULLUP);
  pinMode(PIN_BTN_FRIDGE, INPUT_PULLUP);
  pinMode(PIN_BTN_BULB, INPUT_PULLUP);
  pinMode(PIN_LED_SAFETY, OUTPUT);
  pinMode(PIN_LED_WIFI, OUTPUT);
  pinMode(PIN_BTN_MUTE, INPUT_PULLUP);
  pinMode(PIN_LED_FAN, OUTPUT);
  pinMode(PIN_LED_PUMP, OUTPUT);

  // ---- Restore last known state (survives power loss) ----
  prefs.begin("smarthome", false);
  bulbState = prefs.getBool("bulb", false);
  fridgeState = prefs.getBool("fridge", false);
  safetyArmed = prefs.getBool("safety", false);
  digitalWrite(PIN_RELAY_BULB, bulbState ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_RELAY_FRIDGE, fridgeState ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_LED_SAFETY, safetyArmed ? HIGH : LOW);

  // ---- I2C bus: LCD + RTC ----
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(100000);

  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Smart Home Sys");
  lcd.setCursor(0, 1);
  lcd.print("Connecting WiFi...");

  tempSensor.begin();

  if (!rtc.begin()) {
    Serial.println("RTC not found - check wiring on G11/G12");
    rtcFound = false;
  } else {
    rtcFound = true;
    if (!rtc.isrunning()) {
      rtc.adjust(DateTime(F(__DATE__), F(__TIME__)));
    }
  }

  // ---- WiFi + Blynk (non-blocking: boot offline after 30s if no WiFi) ----
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true); // ESP32 will keep retrying WiFi in the background
  WiFi.persistent(true);
  WiFi.begin(ssid, pass);

  Serial.print("Connecting to WiFi");
  unsigned long wifiStartMs = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - wifiStartMs < WIFI_CONNECT_TIMEOUT_MS) {
    delay(250);
    Serial.print(".");
  }

  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("\nWiFi connected: " + WiFi.localIP().toString());
    Blynk.config(BLYNK_AUTH_TOKEN);
    Blynk.connect(4500); // one non-hanging attempt; Blynk.run() retries later if needed
  } else {
    Serial.println("\nNo WiFi after 30s - starting system OFFLINE. Will auto-connect later.");
    Blynk.config(BLYNK_AUTH_TOKEN); // ready to connect the moment WiFi appears
  }

  digitalWrite(PIN_LED_WIFI, WiFi.status() == WL_CONNECTED ? HIGH : LOW);
  lcd.clear();
}

void loop() {
  // Only let Blynk run its network stack when WiFi is actually connected,
  // otherwise Blynk.run() can stall the loop while retrying.
  if (WiFi.status() == WL_CONNECTED) {
    Blynk.run();
  } else if (millis() - lastWifiRetryMs > WIFI_RETRY_INTERVAL_MS) {
    lastWifiRetryMs = millis();
    WiFi.reconnect(); // periodic nudge; WiFi.setAutoReconnect(true) also does this passively
  }

  if (millis() - lastLoopMs < LOOP_INTERVAL_MS) return;
  lastLoopMs = millis();

  // ---- WiFi status LED ----
  digitalWrite(PIN_LED_WIFI, WiFi.status() == WL_CONNECTED ? HIGH : LOW);

  // ---- Water pump automation ----
  int waterRaw = analogRead(PIN_WATER_LEVEL);
  int waterPercent = map(waterRaw, 0, 4095, 0, 100);
  if (waterPercent <= WATER_LOW_PERCENT) {
    pumpState = true;
  } else if (waterPercent >= WATER_FULL_PERCENT) {
    pumpState = false;
  }
  digitalWrite(PIN_RELAY_PUMP, pumpState ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_LED_PUMP, pumpState ? HIGH : LOW);

  // ---- Fan automation ----
  tempSensor.requestTemperatures();
  float tempC = tempSensor.getTempCByIndex(0);
  fanState = (tempC > TEMP_THRESHOLD_C);
  digitalWrite(PIN_RELAY_FAN, fanState ? RELAY_ON : RELAY_OFF);
  digitalWrite(PIN_LED_FAN, fanState ? HIGH : LOW);

  // ---- Safety Mode toggle (physical button) ----
  bool btnSafety = digitalRead(PIN_BTN_SAFETY);
  if (btnSafety == LOW && lastBtnSafety == HIGH) {
    safetyArmed = !safetyArmed;
    digitalWrite(PIN_LED_SAFETY, safetyArmed ? HIGH : LOW);
    prefs.putBool("safety", safetyArmed);
    Blynk.virtualWrite(VPIN_SAFETY, safetyArmed);
  }
  lastBtnSafety = btnSafety;

  // ---- Mute button (temporarily silences buzzer without disarming) ----
  bool btnMute = digitalRead(PIN_BTN_MUTE);
  if (btnMute == LOW && lastBtnMute == HIGH) {
    buzzerMuted = true;
    muteUntilMs = millis() + MUTE_DURATION_MS;
  }
  lastBtnMute = btnMute;
  if (millis() > muteUntilMs) {
    buzzerMuted = false;
  }

  // ---- PIR + buzzer (only when armed, and not muted) ----
  // Toggles each loop cycle (~300ms) to produce an audible beep-beep-beep
  // pattern instead of one continuous tone.
  bool motion = digitalRead(PIN_PIR);
  if (safetyArmed && motion && !buzzerMuted) {
    buzzerBeepState = !buzzerBeepState;
    digitalWrite(PIN_BUZZER, buzzerBeepState ? HIGH : LOW);
    if (millis() - lastMotionEventMs > MOTION_EVENT_INTERVAL_MS) {
      lastMotionEventMs = millis();
      Blynk.logEvent("motion_alert", "Motion detected while Safety Mode armed");
    }
  } else {
    digitalWrite(PIN_BUZZER, LOW);
    buzzerBeepState = false;
  }

  // ---- Fridge manual (physical button) ----
  bool btnFridge = digitalRead(PIN_BTN_FRIDGE);
  if (btnFridge == LOW && lastBtnFridge == HIGH) {
    fridgeState = !fridgeState;
    digitalWrite(PIN_RELAY_FRIDGE, fridgeState ? RELAY_ON : RELAY_OFF);
    prefs.putBool("fridge", fridgeState);
    Blynk.virtualWrite(VPIN_FRIDGE, fridgeState);
  }
  lastBtnFridge = btnFridge;

  // ---- Bulb manual (physical button) ----
  bool btnBulb = digitalRead(PIN_BTN_BULB);
  if (btnBulb == LOW && lastBtnBulb == HIGH) {
    bulbState = !bulbState;
    digitalWrite(PIN_RELAY_BULB, bulbState ? RELAY_ON : RELAY_OFF);
    prefs.putBool("bulb", bulbState);
    Blynk.virtualWrite(VPIN_BULB, bulbState);
  }
  lastBtnBulb = btnBulb;

  // ---- RTC time ----
  String timeStr = "--:--:--";
  if (rtcFound) {
    DateTime now = rtc.now();
    char buf[9];
    sprintf(buf, "%02d:%02d:%02d", now.hour(), now.minute(), now.second());
    timeStr = String(buf);
  }

  // ---- LCD (20x4 layout) ----
  lcd.setCursor(0, 0);
  lcd.print("Temp:");
  lcd.print(tempC, 1);
  lcd.print("C Water:");
  lcd.print(waterPercent);
  lcd.print("% ");

  lcd.setCursor(0, 1);
  lcd.print("Fan:");
  lcd.print(fanState ? "ON " : "OFF");
  lcd.print(" Pump:");
  lcd.print(pumpState ? "ON " : "OFF");

  lcd.setCursor(0, 2);
  lcd.print("Bulb:");
  lcd.print(bulbState ? "ON " : "OFF");
  lcd.print(" Fridge:");
  lcd.print(fridgeState ? "ON " : "OFF");

  lcd.setCursor(0, 3);
  lcd.print(safetyArmed ? "ARMED " : "SAFE ");
  lcd.print(timeStr);

  // ---- Sync to Blynk (throttled) ----
  if (millis() - lastBlynkSyncMs >= BLYNK_SYNC_INTERVAL_MS) {
    lastBlynkSyncMs = millis();
    Blynk.virtualWrite(VPIN_TEMP, tempC);
    Blynk.virtualWrite(VPIN_WATER, waterPercent);
    Blynk.virtualWrite(VPIN_FAN, fanState);
    Blynk.virtualWrite(VPIN_PUMP, pumpState);
  }
}
