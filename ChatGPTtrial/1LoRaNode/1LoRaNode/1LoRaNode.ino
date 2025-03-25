#include <SPI.h>
#include <RH_RF95.h>
#include <TinyGPS++.h>
#include <time.h>  

#define LED 13  
//float frequency = 904.6;  //915?
float frequency = 915.0;  //915?
#define BATTERY_PIN A5  // Analog pin for battery voltage


RH_RF95 rf95(12, 6);  
TinyGPSPlus gps;
static uint16_t curr_ID = (*(uint32_t*)0x0080A00C); 

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

struct NodeHistory {
    GPSData positions[50];  
    uint8_t count = 0;
};

NodeHistory nodeHistories[10]; 

uint32_t convertToUnixTimestamp() {
    if (gps.date.isValid() && gps.time.isValid()) {
        struct tm t = {0};
        t.tm_year = gps.date.year() - 1900;
        t.tm_mon = gps.date.month() - 1;
        t.tm_mday = gps.date.day();
        t.tm_hour = gps.time.hour();
        t.tm_min = gps.time.minute();
        t.tm_sec = gps.time.second();
        return mktime(&t);
    }
    return 0;
}

float readBatteryLevel() {
    float raw = analogRead(BATTERY_PIN);  
    return (raw / 1023.0) * 3.7 * 2;  // Convert to voltage (assuming a resistor divider)
}

void storePosition(GPSData newData) {
    for (int i = 0; i < 10; i++) {
        if (nodeHistories[i].positions[0].nodeID == newData.nodeID || nodeHistories[i].count == 0) {
            if (nodeHistories[i].count < 50) {
                nodeHistories[i].positions[nodeHistories[i].count++] = newData;
            } else {
                for (int j = 1; j < 50; j++) {
                    nodeHistories[i].positions[j - 1] = nodeHistories[i].positions[j];
                }
                nodeHistories[i].positions[49] = newData;
            }
            break;
        }
    }
}

void setup() {
    SerialUSB.begin(9600);
    Serial1.begin(9600);  
    pinMode(LED, OUTPUT);
    
    if (!rf95.init()) {
        SerialUSB.println("LoRa init failed!");
        while (1);
    }

    
    //rf95.setTxPower(23, false);
    //TRYING NEW THINGS
    //rf95.setModemConfig(RH_RF95::Bw125Cr48Sf4096);
    //Thisworkedbeforeimessedwithit//rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
    rf95.setTxPower(20, false); // with false output is on PA_BOOST, power from 2 to 20 dBm, use this setting for high power demos/real usage
    //END TYING NEW THINGS
    rf95.setFrequency(frequency);

    SerialUSB.print("LoRa Mesh Node Initialized. ID: ");
    SerialUSB.println(curr_ID);
}

void loop() {
    static uint32_t lastDebugTime = 0;
    static uint32_t lastBroadcast = 0;
    static uint32_t gpsLastValidTime = millis();  

    // **Debugging statement every 5 seconds**
    if (millis() - lastDebugTime >= 5000) {
        lastDebugTime = millis();
        SerialUSB.print("Debug: Node ");
        SerialUSB.print(curr_ID);
        SerialUSB.print(" running at ");
        SerialUSB.println(millis() / 1000);
    }

    while (Serial1.available()) {
        gps.encode(Serial1.read());
    }

    // **Check if GPS data is valid, otherwise set fake coordinates**
    bool gpsValid = gps.location.isValid() && gps.altitude.isValid();
    if (gpsValid) {
        gpsLastValidTime = millis();
    }

    // **Send GPS data every 5 seconds**
    if (millis() - lastBroadcast >= 5000) {
        lastBroadcast = millis();

        GPSData gpsData;
        gpsData.senderID = curr_ID;
        gpsData.nodeID = curr_ID;
        gpsData.batteryLevel = readBatteryLevel();  // Read battery level

        if (gpsValid) {
            gpsData.latitude = gps.location.lat();
            gpsData.longitude = gps.location.lng();
            gpsData.altitude = gps.altitude.meters();
            gpsData.timestamp = convertToUnixTimestamp();
        } else if (millis() - gpsLastValidTime >= 10000) {
            // GPS invalid for 10+ seconds, send fake coordinates
            gpsData.latitude = -10.0;
            gpsData.longitude = -10.0;
            gpsData.altitude = 0;
            gpsData.timestamp = millis() / 1000; // Use system time as a fallback
            SerialUSB.println("Warning: GPS data invalid for 10+ sec, broadcasting fake location!");
        } else {
            return;  // Skip broadcast if within the 10-second grace period
        }

        rf95.send((uint8_t*)&gpsData, sizeof(GPSData));
        rf95.waitPacketSent();
        
        digitalWrite(LED, HIGH);  
        delay(100);
        digitalWrite(LED, LOW);

        //SerialUSB.print("Sent GPS: Node ");
        //SerialUSB.print(curr_ID);
        //SerialUSB.print(" at ");
        //SerialUSB.println(gpsData.timestamp);
        SerialUSB.print("Sent GPS: Node ");
        SerialUSB.print(curr_ID);
        SerialUSB.print(" | Battery: ");
        SerialUSB.print(gpsData.batteryLevel, 2);
        SerialUSB.print("V | Time: ");
        SerialUSB.println(gpsData.timestamp);
    }

    // **Always listen for incoming data**
    if (rf95.available()) {
        GPSData incoming;
        uint8_t len = sizeof(GPSData);
        if (rf95.recv((uint8_t*)&incoming, &len)) {
            if (incoming.nodeID != curr_ID) {  
                SerialUSB.print("Received GPS from Node ");
                SerialUSB.print(incoming.nodeID);
                SerialUSB.print(" at ");
                SerialUSB.println(incoming.timestamp);

                storePosition(incoming);

                if (incoming.senderID != curr_ID) {
                    incoming.senderID = curr_ID;
                    rf95.send((uint8_t*)&incoming, sizeof(GPSData));
                    rf95.waitPacketSent();
                    SerialUSB.println("Relayed GPS data.");
                }
            }
        }
    }
}
