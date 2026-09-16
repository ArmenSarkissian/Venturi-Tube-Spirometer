#include "AdafruitIO_WiFi.h"
#include <WiFi.h>
#include "soc/soc.h"
#include "soc/rtc_cntl_reg.h"

// --- Adafruit IO & Wi-Fi Configuration ---
#define IO_USERNAME  ""
#define IO_KEY       ""
#define WIFI_SSID    ""
#define WIFI_PASS    ""

AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Feed *flowFeed;
AdafruitIO_Feed *pressureFeed;

// --- Hardware Pins ---
#define SENSOR_PIN 34  
#define LED_PIN 2     

// --- Venturi Constants ---
const float D1 = 0.020;  
const float D2 = 0.013;  
const float rho = 1.204; 

const float A1 = (3.14159 / 4.0) * D1 * D1;
const float A2 = (3.14159 / 4.0) * D2 * D2;

const float DIVIDER_MULTIPLIER = 1.5;
const float V_SPAN_POSITIVE = 2.0; 
const float P_MAX_PA = 5000.0;     
const float VISIBLE_MAX_FLOW = 50.0; 

float vZero = 2.5; 

// --- Non-blocking Publish Timing ---
unsigned long lastPublishTime = 0;
const unsigned long PUBLISH_INTERVAL = 2000; // Publish every 2 seconds (Adafruit IO rate limit)

void setup() {
  Serial.begin(115200);
  delay(1000);
  WiFi.setTxPower(WIFI_POWER_11dBm);
  WRITE_PERI_REG(RTC_CNTL_BROWN_OUT_REG, 0);

  flowFeed     = io.feed("flow-rate");
  pressureFeed = io.feed("pressure");

  analogSetAttenuation(ADC_11db);
  pinMode(LED_PIN, OUTPUT);

  // Tare calibration
  float voltageSum = 0.0;
  for (int i = 0; i < 50; i++) {
    int raw = analogRead(SENSOR_PIN);
    float pinV = (raw / 4095.0) * 3.3;
    voltageSum += pinV * DIVIDER_MULTIPLIER;
    delay(20);
  }
  vZero = voltageSum / 50.0;

  // --- Explicit WiFi connection with timeout + diagnostics ---
  Serial.print("Connecting to WiFi: ");
  Serial.println(WIFI_SSID);
  WiFi.begin(WIFI_SSID, WIFI_PASS);

  unsigned long wifiStartTime = millis();
  const unsigned long WIFI_TIMEOUT_MS = 15000; // give up after 15 seconds

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if (millis() - wifiStartTime > WIFI_TIMEOUT_MS) {
      Serial.println();
      Serial.print("WiFi failed to connect. Status code: ");
      Serial.println(WiFi.status());
      Serial.println("Restarting...");
      delay(1000);
      ESP.restart();  // clean reboot instead of letting the watchdog panic
    }
  }

  Serial.println();
  Serial.println("WiFi connected!");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // --- Now connect to Adafruit IO (WiFi is already up) ---
  Serial.print("Connecting to Adafruit IO");
  io.connect();

  unsigned long aioStartTime = millis();
  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);

    if (millis() - aioStartTime > 15000) {
      Serial.println();
      Serial.println("Adafruit IO failed to connect. Restarting...");
      delay(1000);
      ESP.restart();
    }
  }

  Serial.println();
  Serial.println(io.statusText());
}