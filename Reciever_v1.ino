#include <SPI.h>
#include <LoRa.h>

#define SS    18
#define RST   14
#define DIO0  26

void setup() {
  Serial.begin(9600);
  while (!Serial);

  Serial.println("LoRa Receiver");

  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(868E6)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }

  // Match the settings from the sender
  LoRa.setSpreadingFactor(12);
  LoRa.setSignalBandwidth(125E3);
  LoRa.setSyncWord(0x34);  // Sync word for private network

  Serial.println("LoRa Initialized");
}

void loop() {
  int packetSize = LoRa.parsePacket();
  if (packetSize) {
    // Received a packet
    Serial.print("Received packet: ");

    // Read packet
    while (LoRa.available()) {
      String incoming = LoRa.readString();
      Serial.print(incoming);
    }

    // Print RSSI (Signal Strength)
    Serial.print(" with RSSI: ");
    Serial.println(LoRa.packetRssi());
  }
}
