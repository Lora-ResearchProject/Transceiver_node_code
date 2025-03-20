#include <LoRa.h>
#include <SPI.h>

#define ss 5    // Pin connected to LoRa module's NSS (CS)
#define rst 14  // Pin connected to LoRa module's RESET
#define dio0 2  // Pin connected to LoRa module's DIO0

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver Initializing...");

  LoRa.setPins(ss, rst, dio0);

  // Initialize LoRa at 433 MHz
  if (!LoRa.begin(433E6)) {
    Serial.println("LoRa Initialization Failed!");
    while (true);
  }

  Serial.println("LoRa Initialized. Ready to receive messages!");
}

void loop() {
  // Check for incoming LoRa packets
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Read the incoming packet
    String receivedMessage = "";
    while (LoRa.available()) {
      receivedMessage += (char)LoRa.read();
    }

    Serial.println("Message Received:");
    Serial.println(receivedMessage);

    // Process the received message
    processMessage(receivedMessage);
  }
}

void processMessage(String message) {
  // Split the payload based on type
  int pipePos = message.indexOf('|');
  if (pipePos != -1) {
    String type = message.substring(0, pipePos);

    if (type == "003") {
      handleChatData(message);
    } else if (type == "128") {
      handleGpsData(message);
    } else {
      Serial.println("Unknown data type");
    }
  } else {
    Serial.println("Malformed payload");
  }
}

void handleChatData(String data) {
  // Example data: "003|UVckeFY|5"
  int firstPipe = data.indexOf('|');
  int secondPipe = data.indexOf('|', firstPipe + 1);

  String id = data.substring(0, firstPipe);
  String user = data.substring(firstPipe + 1, secondPipe);
  String message = data.substring(secondPipe + 1);

  Serial.println("Chat Data Received:");
  Serial.println("ID: " + id);
  Serial.println("User: " + user);
  Serial.println("Message: " + message);
}

void handleGpsData(String data) {
  // Example data: "128|0000|80.12321|13.32432|1"
  int firstPipe = data.indexOf('|');
  int secondPipe = data.indexOf('|', firstPipe + 1);
  int thirdPipe = data.indexOf('|', secondPipe + 1);
  int fourthPipe = data.indexOf('|', thirdPipe + 1);

  String id = data.substring(0, firstPipe);
  String device = data.substring(firstPipe + 1, secondPipe);
  String latitude = data.substring(secondPipe + 1, thirdPipe);
  String longitude = data.substring(thirdPipe + 1, fourthPipe);
  String status = data.substring(fourthPipe + 1);

  Serial.println("GPS Data Received:");
  Serial.println("ID: " + id);
  Serial.println("Device: " + device);
  Serial.println("Latitude: " + latitude);
  Serial.println("Longitude: " + longitude);
  Serial.println("Status: " + status);
}
