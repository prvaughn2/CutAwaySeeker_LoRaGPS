#include <SPI.h>
//#include <RH_RF95.h>
#include "TinyGPS++.h"
#include <time.h>  
//added
#include "LoRaWan_APP.h"
//#include "Arduino.h"


//////
#define LORA_SS 18
#define LORA_RST 14
#define LORA_DIO0 26
///



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
bool lora_idle=true;
int16_t rssi,rxSize;
// wirelesstracker fns
#define RF_FREQUENCY                                915000000 // Hz
#define TX_OUTPUT_POWER                             5        // dBm
#define LORA_BANDWIDTH                              0         // [0: 125 kHz,
                                                              //  1: 250 kHz,
                                                              //  2: 500 kHz,
                                                              //  3: Reserved]
#define LORA_SPREADING_FACTOR                       7         // [SF7..SF12]
#define LORA_CODINGRATE                             1         // [1: 4/5,
                                                              //  2: 4/6,
                                                              //  3: 4/7,
                                                              //  4: 4/8]
#define LORA_PREAMBLE_LENGTH                        8         // Same for Tx and Rx
#define LORA_SYMBOL_TIMEOUT                         0         // Symbols
#define LORA_FIX_LENGTH_PAYLOAD_ON                  false
#define LORA_IQ_INVERSION_ON                        false

#define RX_TIMEOUT_VALUE                            1000
//#define BUFFER_SIZE                                 sizeof(GPSData); // Define the payload size here

char txpacket[sizeof(GPSData)];
char rxpacket[sizeof(GPSData)];

double txNumber;

static RadioEvents_t RadioEvents;

//end fns

#define LED 13  
//float frequency = 904.6;  //915?
float frequency = 915.0;  //915?
#define BATTERY_PIN A3  // Analog pin for battery voltage


//RH_RF95 rf95(12, 6);  //??????????????
TinyGPSPlus gps;
//static uint16_t curr_ID = (*(uint32_t*)0x0080A00C); //this made the kernel panic :(

static uint16_t curr_ID = 21; //make it static for now

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
    return (raw / 1023.0) * 3.7 * 2;  // Convert to voltage (assuming a resistor divider) // ???????????????????????????
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
    Serial.begin(9600);
    Serial1.begin(9600); 
    Serial.println("Entered setup!"); 
    //pinMode(LED, OUTPUT);
        
        
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    rssi=0;
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);

    //if (!Radio.Init( &RadioEvents )) {  <-------------------------I should check to see if it init'd correctly but im not
    //    Serial.println("LoRa init failed!");
    //    while (1);
    //}

    
    //rf95.setTxPower(23, false);
    //TRYING NEW THINGS
    //rf95.setModemConfig(RH_RF95::Bw125Cr45Sf2048);
    //rf95.setModemConfig(RH_RF95::Bw31_25Cr48Sf512);
    //rf95.setTxPower(2, false); // with false output is on PA_BOOST, power from 2 to 20 dBm, use this setting for high power demos/real usage
    //END TYING NEW THINGS
    //rf95.setFrequency(frequency);
        txNumber=0;

    //RadioEvents.TxDone = OnTxDone;
    //RadioEvents.TxTimeout = OnTxTimeout;
    
    
    Radio.SetChannel( RF_FREQUENCY );
    //Radio.SetTxConfig( MODEM_LORA, TX_OUTPUT_POWER, 0, LORA_BANDWIDTH,
    //                               LORA_SPREADING_FACTOR, LORA_CODINGRATE,
    //                               LORA_PREAMBLE_LENGTH, LORA_FIX_LENGTH_PAYLOAD_ON,
    //                               true, 0, 0, LORA_IQ_INVERSION_ON, 3000 ); 
   

    Serial.print("LoRa Mesh Node Initialized. ID: ");
    Serial.println(curr_ID);
}

void loop() {
    static uint32_t lastDebugTime = 0;
    static uint32_t lastBroadcast = 0;
    static uint32_t gpsLastValidTime = millis();  

    // **Debugging statement every 5 seconds**
    if (millis() - lastDebugTime >= 5000) {
        lastDebugTime = millis();
        Serial.print("Debug: Node ");
        Serial.print(curr_ID);
        Serial.print(" running at ");
        Serial.println(millis() / 1000);
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
            Serial.println("Warning: GPS data invalid for 10+ sec, broadcasting fake location!");
        } else {
            return;  // Skip broadcast if within the 10-second grace period
        }
        
        //Send out the data
        Radio.Send( (uint8_t *)&gpsData, sizeof(GPSData) ); //send the package out	
        //rf95.send((uint8_t*)&gpsData, sizeof(GPSData));
        //rf95.waitPacketSent();
        
        digitalWrite(LED, HIGH);  
        delay(100);
        digitalWrite(LED, LOW);

        //Serial.print("Sent GPS: Node ");
        //Serial.print(curr_ID);
        //Serial.print(" at ");
        //Serial.println(gpsData.timestamp);
        Serial.print("Sent GPS: Node ");
        Serial.print(curr_ID);
        Serial.print(" | Battery: ");
        Serial.print(gpsData.batteryLevel, 2);
        Serial.print(" | Time: ");
        Serial.println(gpsData.timestamp);
        //Serial.print("freq: ");
        //Serial.print(Radio.Rssi);
        Serial.print("rssi: ");
         Serial.println(Radio.Rssi(MODEM_LORA));
    }

    // **Always listen for incoming data**
    if (lora_idle){ //??????????????????????
    //if (rf95.available()) {
        GPSData incoming;
        uint8_t len = sizeof(GPSData);
        //if (Radio.Rx((uint8_t*)&incoming, sizeof(GPSData))) {
          Radio.Rx(0);
          Radio.IrqProcess( );
            /*if (incoming.nodeID != curr_ID) {  
                Serial.print("Received GPS from Node ");
                Serial.print(incoming.nodeID);
                Serial.print(" at ");
                Serial.println(incoming.timestamp);

                storePosition(incoming);

                if (incoming.senderID != curr_ID) {
                    incoming.senderID = curr_ID;
                    Radio.Send((uint8_t*)&incoming, sizeof(GPSData));
                    Radio.IrqProcess();
                    Serial.println("Relayed GPS data.");
                }
            }*/
        //}
    }
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );
    Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);
    lora_idle = true;
}
