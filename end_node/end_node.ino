#include <LoRa.h>
#include <SPI.h>
#include <base64.hpp>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>
#include <ArduinoJson.h>
#include <cppQueue.h>

// Pin definitions
#define ss 5    // Pin connected to LoRa module's NSS (CS)
#define rst 14  // Pin connected to LoRa module's RESET
#define dio0 2  // Pin connected to LoRa module's DIO0
#define MAX_PACKETS 20
#define QUEUE_SIZE 10
#define IMPLEMENTATION FIFO

//BLE configurations
#define SERVICE_UUID "12345678-1234-1234-1234-123456789abc"
#define GPS_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef123456"
#define SOS_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef654321"
#define CHAT_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef987654"
#define WEATHER_CHARACTERISTIC_UUID "abcd1234-5678-1234-5678-abcdef111213"

int receivedCount = 0;
int sosPacketIndex = 0;
bool sendLoRaGPSMessageFlag = false;
bool sendLoRaSOSMessageFlag = false;
bool sendWeatherRequestFlag = false;
String loraGPSBuffer = "";
String loraSOSBuffer = "";

cppQueue chatQueue(sizeof(String), QUEUE_SIZE, IMPLEMENTATION);
String weatherData = "";
String weatherRequestData = "";
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

  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (true);
  }

  LoRa.setSpreadingFactor(12);     
  LoRa.setSignalBandwidth(125E3);   
  LoRa.setCodingRate4(5);           
  LoRa.setSyncWord(0x12);            
  LoRa.enableCrc();                
  LoRa.receive();

  Serial.println("LoRa Initialized...");

  initializeBLE();
}

void loop() {

  receiveAndProcessLoRaPacket();

  if (sendLoRaGPSMessageFlag) {
    sendMessage(loraGPSBuffer);
    sendLoRaGPSMessageFlag = false; 
  }
  if (sendLoRaSOSMessageFlag) {
    sendMessage(loraSOSBuffer);
    sendLoRaSOSMessageFlag = false;
  }
  if(sendWeatherRequestFlag) {
    sendMessage(weatherRequestData);
    sendWeatherRequestFlag = false;
  }
}

/**
 * Function to receive and process LoRa packets.
 */
void receiveAndProcessLoRaPacket() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    Serial.print("Received packet: ");
    String encodedData;

    while (LoRa.available()) {
      encodedData += (char)LoRa.read();
    }

    receivedCount++;

    //Received Packets are in Base 64
    String decodedData = decodeBase64(encodedData);
    Serial.println(decodedData);

    String jsonData = convertStringToJSONString(decodedData);
    Serial.print(jsonData);

    // Print Received data RSSI (signal strength)
    // Serial.println(jsonSOS);
    Serial.print(" with RSSI ");
    Serial.println(LoRa.packetRssi());
    
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

String convertJSONToGPSFormattedString(const String& jsonString) {
    StaticJsonDocument<200> doc;  // Create a JSON document

    // Parse JSON string
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print("JSON Parsing Failed: ");
        Serial.println(error.c_str());
        return "";  
    }

    String id = doc["id"].as<String>();       
    String location = doc["l"].as<String>();  
    String status = doc["s"].as<String>();         

    // Split location into latitude and longitude
    int separatorIndex = location.indexOf('|');
    if (separatorIndex == -1) {
        Serial.println("Invalid location format");
        return "";  
    }

    String latitude = location.substring(0, separatorIndex);
    String longitude = location.substring(separatorIndex + 1);

    String formattedData = id + "|0000|" + latitude + "|" + longitude + "|" + status;
    Serial.print("Formatted Data: ");
    Serial.println(formattedData); 
    return formattedData;
}

/**
 * Converts a JSON object to a formatted string.
 *
 * @param jsonString The JSON string to be converted.
 * @return A formatted string in the format: "w|id|latitude|longitude|wr"
 */
String convertJSONToWeatherFormattedString(const String& jsonString) {
    StaticJsonDocument<200> doc;  // Create a JSON document

    // Parse JSON string
    DeserializationError error = deserializeJson(doc, jsonString);
    if (error) {
        Serial.print("JSON Parsing Failed: ");
        Serial.println(error.c_str());
        return "";  // Return empty string on failure
    }

    // Extract values from JSON
    String id = doc["id"].as<String>();       
    String location = doc["l"].as<String>();  
    String wr = String(doc["wr"].as<int>());  // Convert `wr` to string

    // Split location into latitude and longitude
    int separatorIndex = location.indexOf('|');
    if (separatorIndex == -1) {
        Serial.println("Invalid location format");
        return "";  // Return empty string if format is incorrect
    }

    String latitude = location.substring(0, separatorIndex);
    String longitude = location.substring(separatorIndex + 1);

    // Construct the formatted string
    String formattedData = "w|" + id + "|" + latitude + "|" + longitude + "|" + wr;
    
    // Debug Output
    Serial.print("Formatted Data: ");
    Serial.println(formattedData);  

    return formattedData;
}

/**
 * Function to process incoming data, convert it to JSON, and update respective queues.
 *
 * @param input The received string in the format "w|005|Uf6rVNA|100" or "c|005|Uf6rVNA|100".
 * @return A JSON-formatted string.
 */
String convertStringToJSONString(const String &input) {

  int firstDelim = input.indexOf('|');
  int secondDelim = input.indexOf('|', firstDelim + 1);
  int thirdDelim = input.indexOf('|', secondDelim + 1);

  if (firstDelim == -1 || secondDelim == -1 || thirdDelim == -1) {
    Serial.println("Invalid data format received.");
    return "";
  }

  String type = input.substring(0, firstDelim);
  String id = input.substring(firstDelim + 1, secondDelim) + "|" + input.substring(secondDelim + 1, thirdDelim);
  int value = input.substring(thirdDelim + 1).toInt(); 

  StaticJsonDocument<200> jsonDoc;
  jsonDoc["id"] = id;

  if (type == "w") {
    jsonDoc["w"] = value;  // Weather Data
  } else if (type == "c") {
    jsonDoc["m"] = value;  // Chat Data
  } else {
    Serial.println("Unknown data type received.");
    return "";
  }


  String jsonString;
  serializeJson(jsonDoc, jsonString);

  if (type == "w") {
    weatherData = jsonString;
  } else if (type == "c") {
    if (chatQueue.isFull()) chatQueue.pop(nullptr);
      chatQueue.push(&jsonString);
  }

  Serial.print("Weather Data:");
  Serial.println(weatherData);

  return jsonString;
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
    String formattedString = "";

    if (receivedData.length() > 0) {
      dataBuffer += receivedData;
      lastReceivedTime = millis();

      if (receivedData.endsWith("}")) {
        Serial.print("✅ GPS Data received: ");
        Serial.println(dataBuffer);

        formattedString = convertJSONToGPSFormattedString(dataBuffer);
        Serial.print("📌 Formatted Output: ");
        Serial.println(formattedString);
        
        loraGPSBuffer = formattedString;
        sendLoRaGPSMessageFlag = true;

        dataBuffer = "";
      }
    }
  }
};

// Callbacks for SOS characteristic
class SOSCharacteristicCallback : public BLECharacteristicCallbacks {
  String sosBuffer = "";
  void onWrite(BLECharacteristic *pCharacteristic) override {
    String receivedData = pCharacteristic->getValue();
    String formattedString = "";

    if (receivedData.length() > 0) {
      sosBuffer += receivedData;
      lastReceivedTime = millis();

      if (receivedData.endsWith("}")) {
        Serial.print("🚨 SOS Received: ");
        Serial.println(sosBuffer);

        formattedString = convertJSONToGPSFormattedString(sosBuffer);
        Serial.print("📌 Formatted Output: ");
        Serial.println(formattedString);
        
        loraSOSBuffer = formattedString;
        sendLoRaSOSMessageFlag = true;

        sosBuffer = "";
      }
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
    String weatherRequest = pCharacteristic->getValue();
    
    if (weatherRequest.length() > 0) {
      Serial.print("🌦️ Weather Request received: ");
      Serial.println(weatherRequest);

      weatherRequestData = convertJSONToWeatherFormattedString(weatherRequest);
      sendWeatherRequestFlag = true;
    }
  }

  void onRead(BLECharacteristic *pCharacteristic) override {
    if(weatherData) {
      Serial.print("Writing Weather Data to Mobile: ");
      Serial.println(weatherData);
      pCharacteristic->setValue(weatherData.c_str());
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

  // gpsCharacteristic = pService->createCharacteristic(
  //   GPS_CHARACTERISTIC_UUID,
  //   BLECharacteristic::PROPERTY_READ | BLECharacteristic::PROPERTY_WRITE | BLECharacteristic::PROPERTY_NOTIFY);
  // gpsCharacteristic->addDescriptor(new BLE2902());
  // gpsCharacteristic->setCallbacks(new GPSCharacteristicCallback());

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