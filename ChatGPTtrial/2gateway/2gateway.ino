#include <SPI.h>
#include <RH_RF95.h>

#define LED 13  // Status LED
//float frequency = 904.6;
float frequency = 915.0;
//RH_RF95 rf95(8, 3);  // LoRa radio on Feather M0
//Note - for Feather M0 can use Serial, but for SAMD21 pro rf use SerialUSB
RH_RF95 rf95(12, 6);  //RoRa radio on SAMD21 Pro RF

struct GPSData {
    float batteryLevel;
    //uint16_t senderID;
    //uint16_t nodeID;
    int senderID;
    int nodeID;
    float latitude;
    float longitude;
    float altitude;
    //uint32_t timestamp;
    int timestamp;
};

void setup() {
    SerialUSB.begin(9600);
    pinMode(LED, OUTPUT);
    
    if (!rf95.init()) {
        SerialUSB.println("LoRa init failed!");
        while (1);
    }

    rf95.setFrequency(frequency);
    //rf95.setTxPower(23, false);
    SerialUSB.println("LoRa Gateway Initialized.");
}

void loop() {
    if (rf95.available()) {
        GPSData incoming;
        uint8_t len = sizeof(GPSData);
        if (rf95.recv((uint8_t*)&incoming, &len)) {
            SerialUSB.print("Received from Node ");
            SerialUSB.print(incoming.nodeID);
            SerialUSB.print(" | Lat: ");
            SerialUSB.print(incoming.latitude, 6);
            SerialUSB.print(" | Lon: ");
            SerialUSB.print(incoming.longitude, 6);
            SerialUSB.print(" | Alt: ");
            SerialUSB.print(incoming.altitude);
            SerialUSB.print(" | Time: ");
            SerialUSB.print(incoming.timestamp);
            SerialUSB.print(" | Batt: ");
            SerialUSB.println(incoming.batteryLevel);
            //SerialUSB.print(" | rawData: ");
            //SerialUSB.println(incoming);
            
            digitalWrite(LED, HIGH);
            delay(100);
            digitalWrite(LED, LOW);
        }
    }
}