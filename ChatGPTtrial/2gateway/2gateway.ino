#include <SPI.h>
#include <RH_RF95.h>

#define LED 13  // Status LED
float frequency = 904.6;

RH_RF95 rf95(8, 3);  // LoRa radio on Feather M0

struct GPSData {
    uint16_t senderID;
    uint16_t nodeID;
    float latitude;
    float longitude;
    float altitude;
    uint32_t timestamp;
};

void setup() {
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    
    if (!rf95.init()) {
        Serial.println("LoRa init failed!");
        while (1);
    }

    rf95.setFrequency(frequency);
    rf95.setTxPower(23, false);
    Serial.println("LoRa Gateway Initialized.");
}

void loop() {
    if (rf95.available()) {
        GPSData incoming;
        uint8_t len = sizeof(GPSData);
        if (rf95.recv((uint8_t*)&incoming, &len)) {
            Serial.print("Received from Node ");
            Serial.print(incoming.nodeID);
            Serial.print(" | Lat: ");
            Serial.print(incoming.latitude, 6);
            Serial.print(" | Lon: ");
            Serial.print(incoming.longitude, 6);
            Serial.print(" | Alt: ");
            Serial.print(incoming.altitude);
            Serial.print(" | Time: ");
            Serial.println(incoming.timestamp);

            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
        }
    }
}