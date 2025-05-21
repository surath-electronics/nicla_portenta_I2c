#include "Arduino_BHY2.h"
#include <Wire.h>

#define I2C_SLAVE_ADDRESS 0x08

#define BUFFER_SIZE 8

// Sensor declarations
SensorXYZ accel(SENSOR_ID_ACC);
SensorXYZ gyro(SENSOR_ID_GYRO);
Sensor temp(SENSOR_ID_TEMP);
Sensor baro(SENSOR_ID_BARO);
Sensor hum(SENSOR_ID_HUM);

float computeODR(uint32_t currentTimestamp) {
  const int windowSize = 100;
  static uint32_t firstTimestamp = 0;
  static int sampleCount = 0;

  if (sampleCount == 0) {
    firstTimestamp = currentTimestamp;
  }

  sampleCount++;

  if (sampleCount >= windowSize) {
    uint32_t deltaTime = currentTimestamp - firstTimestamp;  // microseconds
    float odr = 1000000.0f * (windowSize - 1) / deltaTime;

    sampleCount = 0;  // reset
    return odr;
  }

  return -1.0f;  // not ready yet
}

struct SensorData {
  uint32_t seq;
  int16_t ax, ay, az;
  int16_t gx, gy, gz;
  float temp, baro, hum;
};

SensorData buffer[BUFFER_SIZE];
volatile int writeIndex = 0;
uint32_t seqCounter = 0;

void setup() {
  Serial.begin(1000000);
  while (!Serial);

  BHY2.begin(NICLA_STANDALONE);

  accel.configure(400, 0);
  gyro.configure(400, 0);
  temp.configure(1, 0);
  baro.configure(1, 0);
  hum.configure(1, 0);

  accel.begin();
  gyro.begin();
  temp.begin();
  baro.begin();
  hum.begin();

  Wire.begin(I2C_SLAVE_ADDRESS);
  Wire.setClock(1000000); // 1 MHz I2C
  Wire.onRequest(sendI2CData);
}

void loop() {
  static uint32_t lastUpdate = 0;

  BHY2.update();

  static int16_t lastAx = 0;
  int16_t currentAx = accel.x();

  if (currentAx != lastAx) {
    lastAx = currentAx;

    SensorData& s = buffer[writeIndex];
    s.seq = seqCounter++;
    s.ax = accel.x();
    s.ay = accel.y();
    s.az = accel.z();
    s.gx = gyro.x();
    s.gy = gyro.y();
    s.gz = gyro.z();
    s.temp = temp.value();
    s.baro = baro.value();
    s.hum = hum.value();

    writeIndex = (writeIndex + 1) % BUFFER_SIZE;

    //Serial.print("Buffered Seq: ");
    //Serial.println(buffer[writeIndex == 0 ? BUFFER_SIZE - 1 : writeIndex - 1].seq);

    // 🖨️ Debug print to Serial 
    /*Serial.print("Seq: "); Serial.print(s.seq);
    Serial.print(" | Accel: ");
    Serial.print(s.ax); Serial.print(", ");
    Serial.print(s.ay); Serial.print(", ");
    Serial.print(s.az); Serial.print(" | ");

    Serial.print("Gyro: ");
    Serial.print(s.gx); Serial.print(", ");
    Serial.print(s.gy); Serial.print(", ");
    Serial.print(s.gz); Serial.print(" | ");

    Serial.print("Temp: ");
    Serial.print(s.temp); Serial.print(" °C | ");

    Serial.print("Baro: ");
    Serial.print(s.baro); Serial.print(" hPa | ");

    Serial.print("Hum: ");
    Serial.println(s.hum);*/

    uint32_t now = micros();
    float odr = computeODR(now);
    if (odr > 0) {
      char buf[32];
      snprintf(buf, sizeof(buf), "📈 ODR: %.2f Hz\n", odr);
      Serial.print(buf);
    }
  }
}

void sendI2CData() {
  const uint8_t* ptr = (const uint8_t*)buffer;
  size_t total = sizeof(buffer);

  while (total > 0) {
    size_t chunkSize = min(total, 32);  // Conservative 32-byte chunks
    Wire.write(ptr, chunkSize);
    ptr += chunkSize;
    total -= chunkSize;
  }
}
