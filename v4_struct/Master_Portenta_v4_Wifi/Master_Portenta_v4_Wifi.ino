#include <WiFi.h>
#include <PubSubClient.h>
#include <Wire.h>

#define NICLA_I2C_ADDRESS 0x08
#define BUFFER_SIZE 8

// For LED
unsigned long lastPublishTime = 0;
const unsigned long LED_TIMEOUT_MS = 1000;
const unsigned long BLINK_INTERVAL_MS = 100;
static unsigned long lastBlink = 0;
static bool ledState = false;

// WiFi credentials
const char* ssid = "IoT_EAA8";
const char* password = "26815718";

// MQTT broker
const char* mqtt_server = "broker.hivemq.com";
const int mqtt_port = 1883;
const char* mqtt_topic = "nicla/data";

// Struct must match Nicla side
struct SensorData {
  uint32_t timestamp;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  float temp, baro, hum;
};

SensorData buffer[BUFFER_SIZE];

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ---------------- ODR Calculation ----------------
float computeODR(uint32_t currentTimeMicros) {
  const int sampleWindow = 500;
  static uint32_t firstTimestamp = 0;
  static int sampleCount = 0;

  if (sampleCount == 0) {
    firstTimestamp = currentTimeMicros;
  }

  sampleCount++;

  if (sampleCount >= sampleWindow) {
    uint32_t deltaTime = currentTimeMicros - firstTimestamp;
    float odr = 1000000.0f * (sampleWindow - 1) / deltaTime;
    sampleCount = 0;
    return odr;
  }

  return -1.0f;
}
// --------------------------------------------------

void setupWiFi() {
  Serial.println("🌐 Connecting to WiFi...");
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); Serial.print(".");
  }
  Serial.println("\n✅ WiFi connected. IP: " + WiFi.localIP().toString());
}

void reconnectMQTT() {
  while (!mqttClient.connected()) {
    Serial.print("🔌 Connecting to MQTT...");
    if (mqttClient.connect("PortentaClient")) {
      Serial.println("connected");
    } else {
      Serial.print("❌ MQTT failed, rc=");
      Serial.println(mqttClient.state());
      delay(2000);
    }
  }
}

void setup() {
  Serial.begin(1000000);
  delay(1000);

  pinMode(LED_BUILTIN, OUTPUT);

  Wire.begin();
  Wire.setClock(1000000);  // 1 MHz I2C

  setupWiFi();
  mqttClient.setServer(mqtt_server, mqtt_port);

}

void loop() {
  static uint32_t lastTimestamp = 0;
  static int newSamples = 0;

  if (!mqttClient.connected()) {
    reconnectMQTT();
  }
  mqttClient.loop();

  // Request binary data from Nicla
  int expectedBytes = sizeof(buffer); // 8 × 28 = 224 bytes
  int received = Wire.requestFrom(NICLA_I2C_ADDRESS, expectedBytes);
  if (received < expectedBytes) {
    Serial.println("⚠️ Incomplete I2C read");
    return;
  }

  Wire.readBytes((char*)buffer, expectedBytes);
  newSamples = 0;

  char payload[1024];
  strcpy(payload, "[");  // Start JSON array

  bool first = true;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    SensorData& s = buffer[i];

    if (s.timestamp <= lastTimestamp) continue;

    int32_t delta = (int32_t)s.timestamp - (int32_t)lastTimestamp;

    lastTimestamp = s.timestamp;
    newSamples++;

    if (!first) strcat(payload, ",");
    first = false;

    // Format as JSON
    char entry[128];
    snprintf(entry, sizeof(entry),
      "{\"timestamp\":%lu,\"ax\":%d,\"ay\":%d,\"az\":%d,"
      "\"gx\":%d,\"gy\":%d,\"gz\":%d,\"temp\":%.2f,"
      "\"baro\":%.2f,\"hum\":%.2f}",
      s.timestamp, s.ax, s.ay, s.az,
      s.gx, s.gy, s.gz, s.temp,
      s.baro, s.hum);

    strcat(payload, entry);
  }
  strcat(payload, "]");  // Close JSON array

  // MQTT publish with LED feedback
  if (mqttClient.publish(mqtt_topic, payload)) {
    lastPublishTime = millis();  // Update last successful publish time
  }

  static uint32_t firstTimestamp = 0;
  static int sampleCount = 0;

  if (newSamples > 0) {
    if (sampleCount == 0) {
      firstTimestamp = lastTimestamp;
    }

    sampleCount += newSamples;

    if (sampleCount >= 500) { //change to change the sample windows
      uint32_t deltaTime = lastTimestamp - firstTimestamp;  // in microseconds
      float odr = 1000000.0f * (sampleCount - 1) / deltaTime;
      Serial.print("📈 Averaged ODR over 500 samples: ");
      Serial.print(odr, 2);
      Serial.println(" Hz");
      sampleCount = 0;
    }
  }

  // Blink LED_BUILTIN every 200 ms as long as we're publishing
  // ---------------- Blinking LED for feedback ----------------
  if (millis() - lastPublishTime < LED_TIMEOUT_MS) {
    if (millis() - lastBlink >= BLINK_INTERVAL_MS) {
      lastBlink = millis();
      ledState = !ledState;
      digitalWrite(LED_BUILTIN, ledState);
    }
  } else {
    digitalWrite(LED_BUILTIN, LOW);  // Off if not publishing
  }

}
