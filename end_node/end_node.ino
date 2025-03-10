#include <LoRa.h>
#include <SPI.h>
#include <base64.hpp>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>

// Pin definitions
#define ss 5    // Pin connected to LoRa module's NSS (CS)
#define rst 14  // Pin connected to LoRa module's RESET
#define dio0 2  // Pin connected to LoRa module's DIO0
#define MAX_PACKETS 20

//BLE configurations
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define GPS_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"
#define SOS_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef654321"
#define CHAT_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef987654"
#define WEATHER_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef111213"

int receivedCount = 0;
int sosPacketIndex = 0;

String sosAlerts[MAX_PACKETS];


bool deviceConnected = false;
unsigned long lastReceivedTime = 0;
const unsigned long timeoutDuration = 10000;

// BLE object pointers
BLECharacteristic *gpsCharacteristic;
BLECharacteristic *sosCharacteristic;
BLECharacteristic *chatCharacteristic;
BLECharacteristic *weatherCharacteristic;

void setup() {
  Serial.begin(9600);
  while (!Serial);
  Serial.println("LoRa Tranceiver Initializing...");
  LoRa.setPins(ss, rst, dio0);

  // Initialize LoRa at 433 MHz (433000000 Hz)
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (true);
  }

  // Configure LoRa parameters
  LoRa.setSpreadingFactor(12);     
  LoRa.setSignalBandwidth(125E3);   
  LoRa.setCodingRate4(5);           
  LoRa.setSyncWord(0x12);            
  LoRa.enableCrc();                

  Serial.println("LoRa Initialized...");
  //Listen on LoRa
  LoRa.receive();
  //Initialize the BLE connection
  initializeBLE();
}

void loop() {
  static int sendCount = 0;
  receiveAndProcessLoRaPacket();

  if (sendCount < 5) {
    sendMessage("145|0000|80.12321|13.32432|1");
    delay(2000);
  }
}

/**
 * Function to receive and process LoRa packets.
 */
void receiveAndProcessLoRaPacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    Serial.print("Received packet: ");
    String encodedData;

    // Read packet
    while (LoRa.available()) {
      encodedData += (char)LoRa.read();
    }

    receivedCount++;

    String decodedData = decodeBase64(encodedData);
    String jsonSOS = convertSOSToJSON(decodedData);
    
    // Caching
    sosAlerts[sosPacketIndex] = jsonSOS;
    sosPacketIndex = (sosPacketIndex + 1) % MAX_PACKETS;

    // Print Received data RSSI (signal strength)
    Serial.println(encodedData);
    Serial.println(decodedData);
    Serial.println(jsonSOS);
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());
    
    if (receivedCount % 3 == 0) {
      printStoredPackets();
    }
  }
}

/**
 * Decodes a Base64 encoded string.
 *
 * @param encodedData The Base64 encoded input string.
 * @return The decoded string.
 */
String decodeBase64(const String& encodedData) {

  unsigned int encodedLength = encodedData.length();
  unsigned char encodedDataArray[encodedLength + 1];
  encodedData.toCharArray((char*)encodedDataArray, encodedLength + 1);

  unsigned int decodedLength = decode_base64_length(encodedDataArray, encodedLength);
  unsigned char decodedData[decodedLength + 1];  

  decode_base64(encodedDataArray, decodedData);
  decodedData[decodedLength] = '\0';

  return String((char*)decodedData);
}

void printStoredPackets() {
  Serial.println("Stored SOS Alerts:");
  for (int i = 0; i < MAX_PACKETS; i++) {
    if (sosAlerts[i].length() > 0) {
      Serial.println(sosAlerts[i]);
    }
  }
}
/**
 * Converts an SOS alert string into a JSON-formatted string.
  * @param input The input SOS alert string with values separated by '|'.
 * @return A JSON-formatted string representing the SOS alert.
 */
String convertSOSToJSON(const String& input) {
  //Splitting the input to the needed parts
  int firstDelim = input.indexOf('|');
  int secondDelim = input.indexOf('|', firstDelim + 1);
  int thirdDelim = input.indexOf('|', secondDelim + 1);
  int fourthDelim = input.indexOf('|', thirdDelim + 1);

  // Extract each component based on split components
  String id = input.substring(0, secondDelim);
  String latitude = input.substring(secondDelim + 1, thirdDelim);
  String longitude = input.substring(thirdDelim + 1, fourthDelim);
  String status = input.substring(fourthDelim + 1);

  // Format the JSON string
  String json = "{\"id\":\"" + id + "\",\"l\":\"" + latitude + "-" + longitude + "\",\"s\":" + status + "}";

  return json;
}

String convertJSONToGPSFormattedString(const String& jsonString) {
    StaticJsonDocument<200> doc;  // Create a JSON document

    // Parse JSON string
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print("JSON Parsing Failed: ");
        Serial.println(error.c_str());
        return "";  // Return empty string on failure
    }

    // Extract values
    String id = doc["id"].as<String>();       // Extract ID
    String location = doc["l"].as<String>();  // "7.05008|79.91997"
    int status = doc["s"].as<int>();          // Extract status (0 or 1)

    // Split location into latitude and longitude
    int separatorIndex = location.indexOf('|');
    if (separatorIndex == -1) {
        Serial.println("Invalid location format");
        return "";  // Return empty string if format is incorrect
    }

    String latitude = location.substring(0, separatorIndex);
    String longitude = location.substring(separatorIndex + 1);

    // Generate the formatted string with ID
    String formattedData = id + "|0000|" + latitude + "|" + longitude + "|" + "0";
    Serial.print("Formatted Data Length: ");
    Serial.println(formattedData.length());  // Should be > 0 if valid
    Serial.print("Formatted Data: ");
    Serial.println(formattedData);  // Print the actual string
    return formattedData;
}

/**
 * Converts an SOS alert string into a JSON-formatted string.
  * @param message The input message in string.
 */
void sendMessage(const String &message) {
  LoRa.beginPacket();
  LoRa.print(message);
  LoRa.endPacket();
  Serial.println("Sent packet: " + message);
}

//BLE Server Callback
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer *pServer) override {
    deviceConnected = true;
    Serial.println("Client connected");
  }

  void onDisconnect(BLEServer *pServer) override {
    deviceConnected = false;
    Serial.println("Client disconnected");
    BLEDevice::startAdvertising();
  }
};

// Callbacks for GPS characteristic
class GPSCharacteristicCallback : public BLECharacteristicCallbacks {
  String dataBuffer = "";

   void onWrite(BLECharacteristic *pCharacteristic) override {
    String receivedData = pCharacteristic->getValue();

    if (receivedData.length() > 0) {
      dataBuffer += receivedData;
      lastReceivedTime = millis();

      if (receivedData.endsWith("}")) {
        Serial.print("✅ GPS Data received: ");
        Serial.println(dataBuffer);

        String formattedString = convertJSONToGPSFormattedString(dataBuffer);
        Serial.print("📌 Formatted Output: ");
        Serial.println(formattedString);
        

        dataBuffer = "";
      }
    }
  }
};

// Callbacks for SOS characteristic
class SOSCharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("🚨 SOS Received: ");
      Serial.println(value);
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    Serial.println("📩 Fetch SOS Alerts request received");

    // Build a valid JSON array string
    String sosAlertsArray = "["; 

    for (int i = 0; i < MAX_PACKETS; i++) {
        if (sosAlerts[i].length() > 0) { // Ensure the entry is not empty
            if (sosAlertsArray.length() > 1) {  // If not the first item, add a comma separator
                sosAlertsArray += ",";
            }
            sosAlertsArray += sosAlerts[i];  // Append the JSON object
        }
    }

    sosAlertsArray += "]";  // Close the JSON array

    // Set the formatted JSON array string as the characteristic value
    pCharacteristic->setValue(sosAlertsArray.c_str());

    // Log the response being sent
    Serial.print("📤 SOS Alerts sent to mobile: ");
    Serial.println(sosAlertsArray);
  }
};

// Callbacks for Chat characteristic
class ChatCharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Chat Message received from Mobile App: ");
      Serial.println(value);
    }
  }
};

// Callbacks for Weather characteristic
class WeatherCharacteristicCallback : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String value = pCharacteristic->getValue();
    if (value.length() > 0) {
      Serial.print("Weather Data received from Mobile App: ");
      Serial.println(value);
    }
  }
};

//Setup and intialize BLE characteristics
void initializeBLE() {
  Serial.println("Initializing BLE...");

  BLEDevice::init("ESP32-MultiService");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);

  gpsCharacteristic = pService->createCharacteristic(
    GPS_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  gpsCharacteristic->addDescriptor(new BLE2902());
  gpsCharacteristic->setCallbacks(new GPSCharacteristicCallback());

  sosCharacteristic = pService->createCharacteristic(
    SOS_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  sosCharacteristic->addDescriptor(new BLE2902());
  sosCharacteristic->setCallbacks(new SOSCharacteristicCallback());

  chatCharacteristic = pService->createCharacteristic(
    CHAT_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  chatCharacteristic->addDescriptor(new BLE2902());
  chatCharacteristic->setCallbacks(new ChatCharacteristicCallback());

  weatherCharacteristic = pService->createCharacteristic(
    WEATHER_CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_NOTIFY);
  weatherCharacteristic->addDescriptor(new BLE2902());
  weatherCharacteristic->setCallbacks(new WeatherCharacteristicCallback());

  pService->start();

  BLEDevice::setMTU(250);

  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->start();

  Serial.println("BLE Initialized. Waiting for a client connection...");
};