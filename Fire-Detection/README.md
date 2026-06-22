# 🔥 IoT Fire & Gas Detection System

An advanced, ESP32-based smart monitoring system designed for automotive environments. This project continuously monitors ambient temperature and combustible gas concentrations, providing local visual/audible alerts and real-time cloud logging via Firebase.

---

## 📸 Project Showcase
<img width="2040" height="1530" alt="breadboard" src="https://github.com/user-attachments/assets/71ace93b-de4d-49cb-9420-8a582dbe95c5" />
<img width="2040" height="1530" alt="temp" src="https://github.com/user-attachments/assets/2b1477e3-faca-406b-99c5-ba2fb8fd2db4" />
<img width="2040" height="1530" alt="wifimode" src="https://github.com/user-attachments/assets/6af3d297-b132-42a7-9c28-43f4c74aa09e" />
<img width="1867" height="918" alt="Screenshot 2026-06-22 142517" src="https://github.com/user-attachments/assets/036441cf-713e-40b7-9884-4c7e4c7c8f41" />



---

## 📚 Required Libraries
Before uploading the code, ensure the following libraries are installed via the Arduino Library Manager:
1. **WiFi.h** (Built-in with ESP32 board package)
2. **Firebase ESP32 Client** by *Mobizt* (Handles real-time database connection)
3. **LiquidCrystal I2C** by *Frank de Brabander* (Handles the LCD screen)
4. **DHT sensor library** by *Adafruit* (Reads temperature & humidity)

---

## ☁️ Firebase Realtime Database Setup
To make the cloud logging work, you need to configure a Firebase project:
1. Go to the [Firebase Console](https://console.firebase.google.com/) and create a new project.
2. Navigate to **Build > Realtime Database** and click **Create Database**.
3. Go to the **Rules** tab and set both `read` and `write` to `true` for testing purposes.
4. **Get the Database URL:** Copy the link provided at the top of the Realtime Database page (this is your `FIREBASE_HOST`).
5. **Get the Secret Token:** Go to **Project Settings** (gear icon) > **Service Accounts** > **Database Secrets**. Click "Show" and copy the token (this is your `FIREBASE_AUTH`).

---

## 🧠 Theory of Operation & Hardware Logic

### 1. The Sensors
* **DHT22 (Temperature):** A digital sensor that uses a capacitive humidity sensor and a thermistor to measure the surrounding air. It processes the analog signal internally and outputs a digital signal to the ESP32. It requires a minimum of 2 seconds between readings to clear its internal cache.
* **MQ-2 (Gas/Smoke):** An analog sensor containing an SnO2 (Tin Dioxide) sensing element. It requires 5V to heat up. When combustible gases or smoke interact with the heated element, its electrical conductivity changes, outputting a variable voltage proportional to the gas concentration.

### 2. The Voltage Divider (Protecting the ESP32)
The ESP32 pins operate at a maximum of **3.3V**. However, the MQ-2 sensor outputs an analog signal up to **5V**. Connecting them directly would fry the ESP32's ADC (Analog-to-Digital Converter) pin. 

To safely read the sensor, we use a voltage divider consisting of two resistors ($R_1=1k\Omega$ and $R_2=2k\Omega$). The mathematical relationship is:



Plugging in our maximum values:

$$V_{out}=5V\cdot\frac{2000\Omega}{1000\Omega+2000\Omega}=5V\cdot\frac{2}{3}\approx3.33V$$

This perfectly scales the 0-5V signal down to a safe 0-3.3V range for the ESP32's GPIO 34.

### 3. I2C Serial Communication Protocol
The 1602 LCD uses an I2C (Inter-Integrated Circuit) adapter (PCF8574). I2C is a synchronous, multi-master/multi-slave protocol that requires only two wires:
* **SDA (Serial Data):** Transmits the actual data.
* **SCL (Serial Clock):** Synchronizes the data transfer.

Unlike the gas sensor, I2C uses an "open-drain" architecture. The lines are pulled up to 3.3V by the ESP32. Devices communicate by pulling the line down to Ground (GND). Therefore, even though the LCD module is powered by 5V, it never pushes 5V into the data lines, making it inherently safe for the 3.3V ESP32 without a voltage divider.

### 4. Alert System
* **Yellow LED:** Connected via a **220Ω current-limiting resistor** to prevent drawing too much current, which could burn both the LED and the ESP32 pin.
* **Active Buzzer:** Driven by a PWM (Pulse Width Modulation) signal to play the Star Wars "Imperial March" frequencies when a fire is detected.

### 5. LCD UI Logic & Memory Management
The 1602 LCD display is strictly constrained by its hardware architecture:
* **Grid System:** It admits a maximum of 32 characters at any given time, organized in a rigid grid of **16 columns** and **2 rows**. The positioning is zero-indexed (Columns: 0 to 15, Rows: 0 and 1).
* **CGRAM Limitation (Custom Characters):** The internal Hitachi HD44780 controller only allocates memory for a maximum of **8 custom characters** (saved in slots 0 through 7). In this project, slot `0` is used to map the 5x8 pixel matrix of the custom smiley face 😊.
* **Flicker-Free Rendering:** To prevent the visual "flickering" caused by the `lcd.clear()` command in a continuous loop, the UI logic employs dynamic padding. The code actively overwrites old data by printing empty spaces over unused columns, ensuring a smooth and highly responsive data readout.

---

## 💻 Code Breakdown

* **`secrets.h` Integration:** A separate header file keeps the Wi-Fi credentials and Firebase tokens out of the main code for security.
* **`setup()` Function:** Initializes the I2C bus, LCD, DHT sensor, establishes the Wi-Fi connection, and authenticates with Firebase.
* **`loop()` Function:**
  1. **Data Acquisition:** Reads temperature from DHT22 and analog voltage from the MQ-2 sensor (scaled via the voltage divider).
  2. **Mathematical Conversions:** Calculates Fahrenheit and Kelvin dynamically from the Celsius reading.
  3. **Cloud Transmission:** Pushes all metric units to the Firebase Realtime Database.
  4. **Logic & UI:** Checks against predefined danger thresholds (`TEMP_THRESHOLD = 50.0`, `GAS_THRESHOLD = 1800`). If safe, it updates the local LCD with environmental data and Wi-Fi signal strength. If danger is detected, it enters a blocking alarm cycle (flashing UI and playing the melody).

---

## 🔌 Pinout Configuration

| ESP32 Pin | Component Pin | Notes |
| :--- | :--- | :--- |
| **GPIO 22 / 23** | LCD SDA / SCL | Custom I2C Data and Clock lines. |
| **GPIO 5** | DHT22 Data | Digital Input. |
| **GPIO 34** | MQ-2 A0 | **ADC1 Pin.** Connected via the 1kΩ/2kΩ voltage divider. |
| **GPIO 15** | LED Anode | Output via 220Ω resistor. |
| **GPIO 2** | Buzzer IN | Output (PWM). |
| **VIN / 5V** | MQ-2 & LCD VCC | Required for sensor heating and LCD contrast. |

---

## 🚀 Future Enhancements & Extensibility

This project is built with modularity and scalability in mind. The repository will receive continuous updates managed and committed via **SourceTree** to maintain a clean, professional version control history. 

Planned future developments include:

* **3D Printed Custom Enclosure:** Designing and manufacturing a custom 3D-printed case to securely house the ESP32, wiring, and LCD. The design will include strategic ventilation slots for the MQ-2 and DHT22 sensors, transitioning the build from a breadboard prototype to a physically deployable automotive unit.
* **Mobile App & Smart Home Integration:** Upgrading the telemetry pipeline from raw Firebase data logging to a fully interactive user interface. This includes integration with platforms like Home Assistant (via MQTT) or a custom mobile application to provide active push notifications and remote system management.
* **Advanced Procedural Security & Architecture:** Further expanding the procedural programming approach to strictly separate core logic from network and configuration layers. This design pattern ensures maximum data protection, guaranteeing that sensitive credentials (currently isolated in `secrets.h`) and internal variables are never exposed to the global scope or version control.
* **Black Box Data Logging:** Planned implementation of the SPIFFS/LittleFS file system to cache telemetry logs (.csv) locally during periods of no internet connectivity. This "black box" feature will include an automatic sync-on-reconnect mechanism to ensure 100% data integrity for your cloud dashboard, regardless of network availability.
