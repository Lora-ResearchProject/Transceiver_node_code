#include <SPI.h>
#include <LoRa.h>

#define SS    18
#define RST   14
#define DIO0  26

String mode = "receive";

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Initialized with Mode Control");

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(433.775E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

 
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSyncWord(0x34);  

  Serial.println("LoRa Initialized. Default Mode: Receive");
  Serial.println("Enter 't' for transmit mode or 'r' for receive mode.");
}

void loop() {
  if (Serial.available() > 0) {
    char command = Serial.read();
    if (command == 't') {
      mode = "transmit";
      Serial.println("Switched to Transmit Mode");
    } else if (command == 'r') {
      mode = "receive";
      Serial.println("Switched to Receive Mode");
    }
  }

  if (mode == "receive") {
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
      Serial.print("Received packet: ");

      while (LoRa.available()) {
        String incoming = LoRa.readString();
        Serial.print(incoming);
      }

      Serial.print(" with RSSI: ");
      Serial.println(LoRa.packetRssi());
    }
  } else if (mode == "transmit") {
    String message = "Hello from transmitter";
    LoRa.beginPacket();
    LoRa.print(message);
    LoRa.endPacket();
    Serial.println("Message sent: " + message);

    delay(2000);
  }
}
