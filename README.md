# Nicla-Portenta: High-Frequency Sensor Data Acquisition and Streaming

This repository is part of a Master’s thesis project focused on developing a high-speed data acquisition and communication system using the **Nicla Sense ME** and the **Portenta H7** boards by Arduino. The system enables real-time streaming of multi-sensor data over I²C, Wi-Fi, and MQTT, with capabilities for cloud integration and storage.

---

## 🎯 Project Objectives

- Interface **Nicla Sense ME** as an I²C sensor node for Portenta H7
- Enable high-frequency sampling (up to 400Hz) for inertial sensors
- Batch and stream sensor data reliably with minimal loss
- Upload and visualize real-time data using MQTT and cloud services
- Analyze bottlenecks and limitations in embedded I²C and wireless streaming

---

## 🧩 Repository Structure

```
Nicla-Portenta/
├── Nicla/               → Firmware for Nicla Sense ME (sensor setup, batching)
├── Portenta/            → Firmware for Portenta H7 (I²C master, MQTT streaming)
├── DataStream/          → Python scripts for MQTT logging and InfluxDB upload
├── .gitignore           → Ignores large or temporary files
└── README.md
```

---

## 🧠 Technologies Used

- **Arduino MBED OS** for Nicla and Portenta development
- **BHY2 Sensor API** for configuring Bosch sensors
- **I²C communication** at high bus speeds (`Wire.setClock()`)
- **Wi-Fi and MQTT** for real-time data transmission
- **Python, InfluxDB, Grafana** for data ingestion and visualization

---

## 📌 Notable Features

- Multi-sensor support: accelerometer, gyroscope, barometer, temperature, humidity
- Automatic ODR detection and dynamic sampling adjustment
- Data batching and sequential integrity tracking (via sequence numbers)
- Detection and logging of dropped samples for reliability analysis
- Real-time MQTT streaming to cloud endpoints

---

## ⚠️ Notes

- `test.portenta_stream.json` (large JSON test data) has been excluded due to GitHub’s 100MB file limit.
- The project is ongoing; expect further refactoring and modularization.
- Git LFS is recommended if large files need to be tracked in future versions.

---

## 📚 Academic Context

--

---

## 📝 License

--