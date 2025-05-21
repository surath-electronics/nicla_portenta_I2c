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
  uint32_t seq;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  float temp, baro, hum;
};

SensorData buffer[BUFFER_SIZE];

WiFiClient wifiClient;
PubSubClient mqttClient(wifiClient);

// ---------------- ODR Calculation ----------------
float computeODR(uint32_t currentTimeMicros) {
  const int sampleWindow = 100;
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
  static uint32_t lastSeq = 0;
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

  for (int i = 0; i < BUFFER_SIZE; i++) {
    SensorData& s = buffer[i];

    if (s.seq <= lastSeq) continue;

    int32_t delta = (int32_t)s.seq - (int32_t)lastSeq;
    lastSeq = s.seq;
    newSamples++;

    // Format as JSON
    String payload = "{";
    payload += "\"seq\":" + String(s.seq) + ",";
    payload += "\"ax\":" + String(s.ax) + ",";
    payload += "\"ay\":" + String(s.ay) + ",";
    payload += "\"az\":" + String(s.az) + ",";
    payload += "\"gx\":" + String(s.gx) + ",";
    payload += "\"gy\":" + String(s.gy) + ",";
    payload += "\"gz\":" + String(s.gz) + ",";
    payload += "\"temp\":" + String(s.temp, 2) + ",";
    payload += "\"baro\":" + String(s.baro, 2) + ",";
    payload += "\"hum\":" + String(s.hum, 2);
    payload += "}";

    // MQTT publish with LED feedback
    if (mqttClient.publish(mqtt_topic, payload.c_str())) {
      lastPublishTime = millis();  // Update last successful publish time
    }

    // MQTT publish with printing of data
    /*if (mqttClient.publish(mqtt_topic, payload.c_str())) {
      Serial.print("📤 Published: ");
      Serial.println(payload);
    } else {
      Serial.println("❌ MQTT publish failed");
    }*/
  }

  // Show ODR once per batch
  /*if (newSamples > 0) {
    uint32_t now = micros();
    for (int i = 0; i < newSamples; i++) {
      float odr = computeODR(now);
      if (odr > 0) {
        Serial.print("📈 ODR: ");
        Serial.print(odr, 2);
        Serial.println(" Hz");
      }
    }
  }*/

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
