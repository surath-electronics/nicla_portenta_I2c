#include <Wire.h>

#define NICLA_I2C_ADDRESS 0x08
#define BUFFER_SIZE 8  // Must match Nicla

// Must exactly match the Nicla struct
struct SensorData {
  uint32_t seq;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  float temp, baro, hum;
};

SensorData buffer[BUFFER_SIZE];

void setup() {
  Wire.begin();                // Start I2C as master
  Wire.setClock(1000000);      // 1 MHz I2C (match Nicla)
  Serial.begin(1000000);       // High-speed Serial for debug
}

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

void loop() {
  static uint32_t lastSeq = 0;
  static int missedPackets = 0;

  int requested = sizeof(buffer);
  int received = Wire.requestFrom(NICLA_I2C_ADDRESS, requested);
  if (received < requested) {
    Serial.print("⚠️ Incomplete I2C read: got ");
    Serial.print(received);
    Serial.print(" / ");
    Serial.print(requested);
    Serial.println(" bytes");
    return;
  }

  Wire.readBytes((char*)buffer, requested);

  int newSamples = 0;

  for (int i = 0; i < BUFFER_SIZE; i++) {
    SensorData& s = buffer[i];

    if (s.seq <= lastSeq) continue;  // Skip stale

    int32_t delta = (int32_t)s.seq - (int32_t)lastSeq;

    // Packets lost
    /*if (lastSeq != 0 && delta > 1) {
      missedPackets += (delta - 1);
      Serial.print("⚠️ Missed ");
      Serial.print(delta - 1);
      Serial.println(" packet(s)");
    }*/

    lastSeq = s.seq;
    newSamples++;
  }

  if (newSamples > 0) {
    uint32_t now = micros();
    for (int i = 0; i < newSamples; i++) {
      float odr = computeODR(now);
      if (odr > 0) {
        Serial.print("📈 ODR: ");
        Serial.print(odr, 2);
        Serial.println(" Hz");
      }
    }
  }

  // Optional: drop stats every second
  /*static uint32_t lastReport = millis();
  if (millis() - lastReport > 1000) {
    Serial.print("📊 Missed packets in last second: ");
    Serial.println(missedPackets);
    missedPackets = 0;
    lastReport = millis();
  }*/
}

