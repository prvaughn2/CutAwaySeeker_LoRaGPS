/* Heltec Automation Receive communication test example
 *
 * Function:
 * 1. Receive the same frequency band lora signal program
 *  
 * Description:
 * 
 * HelTec AutoMation, Chengdu, China
 * 成都惠利特自动化科技有限公司
 * www.heltec.org
 *
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"


#define RF_FREQUENCY                                915000000 // Hz

#define TX_OUTPUT_POWER                             14        // dBm

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
#define BUFFER_SIZE                                 28 // Define the payload size here

//char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi,rxSize;

bool lora_idle = true;

/*struct GPSData {
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
}__attribute__((packed));;
*/
struct __attribute__((packed)) GPSData {
    float batteryLevel;
    int senderID;
    int nodeID;
    float latitude;
    float longitude;
    float altitude;
    int timestamp;
};


void setup() {
    Serial.begin(115200);
    Mcu.begin(HELTEC_BOARD,SLOW_CLK_TPYE);
    
    txNumber=0;
    rssi=0;
  
    RadioEvents.RxDone = OnRxDone;
    Radio.Init( &RadioEvents );
    Radio.SetChannel( RF_FREQUENCY );
    Radio.SetRxConfig( MODEM_LORA, LORA_BANDWIDTH, LORA_SPREADING_FACTOR,
                               LORA_CODINGRATE, 0, LORA_PREAMBLE_LENGTH,
                               LORA_SYMBOL_TIMEOUT, LORA_FIX_LENGTH_PAYLOAD_ON,
                               0, true, 0, 0, LORA_IQ_INVERSION_ON, true );
}



void loop()
{
  if(lora_idle)
  {
    lora_idle = false;
    Serial.println("into RX mode");
    Radio.Rx(0);
  }
  Radio.IrqProcess( );
}

void OnRxDone( uint8_t *payload, uint16_t size, int16_t rssi, int8_t snr )
{
    ::rssi=rssi;
    rxSize=size;
    memcpy(rxpacket, payload, size );
    rxpacket[size]='\0';
    Radio.Sleep( );
    Serial.printf("\r\nreceived packet \"%s\" with rssi %d , length %d\r\n",rxpacket,rssi,rxSize);

    Serial.print("Raw payload: ");
    for (int i = 0; i < size; i++) {
        if (rxpacket[i] < 0x10) Serial.print("0"); // leading zero for single-digit hex
        Serial.print(rxpacket[i], HEX);
        Serial.print(" ");
    }
    Serial.println();




    GPSData incoming;
    uint8_t len = sizeof(GPSData);
        Serial.print(" sizeofGPSData: ");
    Serial.print(len);
    //memcpy( (uint8_t*)&incoming, payload, len);
    // Cast the received bytes to the struct
    //incoming = *(GPSData*)rxpacket;
    // Skip the first 4 bytes (if you know they are unnecessary)
uint8_t* payloadStart = (uint8_t*)(rxpacket + 4);
// Cast the remaining bytes to the struct
incoming = *(GPSData*)payloadStart;





    Serial.print(" | nodeID: ");
    Serial.println(incoming.nodeID);
    Serial.print(" | Lat: ");
    Serial.print(incoming.latitude, 6);
    Serial.print(" | Lon: ");
    Serial.print(incoming.longitude, 6);
    Serial.print(" | Alt: ");
    Serial.print(incoming.altitude);
    Serial.print(" | Time: ");
    Serial.print(incoming.timestamp);
    Serial.print(" | Batt: ");
    Serial.println(incoming.batteryLevel);


    lora_idle = true;
}