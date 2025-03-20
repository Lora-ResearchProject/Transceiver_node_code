
# ESP32 LoRa 

## 📡 Overview
This project provides an ESP32-based firmware that bridges communication between a LoRa transceiver and Bluetooth Low Energy (BLE).
It allows:
- Receiving data packets over LoRa, decoding them from Base64, and processing them into JSON formats.
- Transmitting GPS, SOS, weather, hotspot, chat, and linking information between mobile devices via BLE and remote nodes via LoRa.
- Requesting data (weather, hotspot alerts) via BLE and receiving responses from the LoRa network.

---

## ⚙️ Features
- LoRa receiver and transmitter with base64 decoding.
- BLE server with multiple characteristics for:
  - **GPS data transmission**
  - **SOS alerts** (read/write)
  - **Chat messages**
  - **Weather requests and responses**
  - **Hotspot requests and data streaming**
  - **Device linking**
- JSON parsing and formatting for LoRa messages.
- Queue management for chat and hotspot data.
- Configurable timeouts and automatic BLE advertising restart.

---

## 📶 Hardware Used
- ESP32 Dev Module
- LoRa Module (433MHz)
- Required SPI connections: NSS (CS), RESET, DIO0

**Pin Configurations:**
| Signal   | ESP32 Pin |
|----------|-----------|
| NSS (CS) | GPIO 5    |
| RESET    | GPIO 14   |
| DIO0     | GPIO 2    |

---

## 📡 LoRa Configuration
- Frequency: **433 MHz**
- Spreading Factor: **12**
- Bandwidth: **125 kHz**
- Coding Rate: **4/5**
- CRC Enabled
- Sync Word: **0x12**

---

## 📱 BLE Services & Characteristics

| Characteristic        | UUID                                   | Description                              |
|-----------------------|----------------------------------------|------------------------------------------|
| GPS                  | `abcd1234-5678-1234-5678-abcdef123456` | Send GPS data to LoRa                   |
| SOS                  | `abcd1234-5678-1234-5678-abcdef654321` | Send emergency alerts                   |
| Chat                 | `abcd1234-5678-1234-5678-abcdef987654` | Receive chat messages from mobile       |
| Weather              | `abcd1234-5678-1234-5678-abcdef111213` | Request weather data or read responses  |
| Hotspot              | `abcd1234-5678-1234-5678-abcdef111214` | Request and read hotspot information    |
| Linking              | `abcd1234-5678-1234-5678-abcdef111215` | Send device linking information         |

**Service UUID:**
`12345678-1234-1234-1234-123456789abc`

---

## 🔄 Data Formats

### LoRa Incoming Message Types:
| Type   | Format                                             | Description                           |
|--------|----------------------------------------------------|---------------------------------------|
| Weather| `w|id|latitude|longitude|value`                    | Weather request/response             |
| Chat   | `c|id|latitude|longitude|message`                  | Chat message received from LoRa       |
| Hotspot| `h|hotspotID|latitude|longitude`                   | Hotspot data                         |

### BLE to LoRa formatted strings:
- **GPS:** `id|latitude|longitude|status` (status: `0` for normal, `1` for SOS)
- **Weather:** `w|id|latitude|longitude|weather_request_code`
- **Hotspot Request:** `h|latitude|longitude`

---

## 🗂 Dependencies
Make sure to install the following libraries in the Arduino IDE:
- [LoRa](https://github.com/sandeepmistry/arduino-LoRa)
- SPI (built-in)
- [base64](https://github.com/adamvr/arduino-base64)
- [ArduinoJson](https://arduinojson.org/)
- [cppQueue](https://github.com/cppthings/cppQueue)
- BLE Libraries (included with ESP32 core)

---

## 🚀 Setup Instructions
1. Connect LoRa module pins to the ESP32 board according to the pin definitions.
2. Open the provided `.ino` file in the Arduino IDE.
3. Select your ESP32 board under **Tools > Board > ESP32 Arduino**.
4. Install all dependencies.
5. Upload the sketch.
6. Open the Serial Monitor at **9600 baud** to debug and view logs.

---

## ✅ Key Functionalities
- **Automatic LoRa reception and packet decoding**
- **Forwarding BLE-written data to LoRa**
- **Receiving LoRa packets, decoding, and storing structured JSON in queues**
- **BLE read requests trigger sending stored queue data to the mobile client**
- **SOS alert history retrieval over BLE**
- **Hotspot and weather requests trigger immediate LoRa transmission**

---

## 💡 Potential Improvements
- Add persistent SOS alert storage in EEPROM/Flash
- Dynamic configuration of LoRa parameters via BLE
- Add CRC and integrity checks on incoming data
- OTA firmware updates

---

## 🤝 Contributions
Feel free to fork, modify, and contribute!

---

## 🛠 License
This project is licensed under the MIT License.


