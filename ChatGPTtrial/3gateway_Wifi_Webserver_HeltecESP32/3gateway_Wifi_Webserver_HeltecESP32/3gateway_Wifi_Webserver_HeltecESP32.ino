#include <WiFi.h>
#include <WebServer.h>
#include <heltec.h>
#include <ESPmDNS.h>
#include <Update.h>
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

bool WiFiDownLink = false;
uint32_t WiFidonwlinkTime;
String Write_data="";


WebServer server(80);
uint16_t num=0;
String LoRa_data;
String htmlS ="<html>\
  <script type=\"text/javascript\">\
    function myrefresh()\
    {\
      window.location.reload();\
    }\
window.onload=function(){\
      setTimeout('myrefresh()',10000);\
      }   \
  </script>\
  ";
  String htmlF = "\
</html>";

String htmlWString = "";
String htmlW = "<html>" + htmlWString + "</html>";


/*String htmlW = "<html>\
  <head>\
    <title>Wireless_BridgeW</title>\
      <meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">\
  </head>\
  <style type=\"text/css\">\
    .header{display: block;margin-top:10px;text-align:center;width:100%;font-size:10px}\
    .input{display: block;text-align:center;width:100%}\
    .input input{height: 20px;width: 300px;text-align:center;border-radius:15px;}\
    .input select{height: 26px;width: 305px;text-align:center;border-radius:15px;}\
    .btn,button{width: 305px;height: 40px;border-radius:20px; background-color: #000000; border:0px; color:#ffffff; margin-top:20px;}\
  </style>\
  <script type=\"text/javascript\">\
window.onload=function(){\
      document.getElementsByName(\"server\")[0].value = \"\";\
      }   \
  </script>\
  <body>\
    <div style=\"width:100%;text-align:center;font-size:25px;font-weight:bold;margin-bottom:20px\">Wireless_Bridge</div>\
      <div style=\"width:100%;text-align:center;\">\
        <div class=\"header\"><span>(Note 1: The data sent from this webpage to LoRa needs to be decoded to be able to be viewed normally.)</span></div>\
        <div class=\"header\"><span>(Note 2: The default size of data sent by LoRa is 4 bytes, which can be modified in the program.)</span></div>\
        <form method=\"POST\" action=\"\" onsubmit=\"\">\
          <div class=\"header\"><span>DATA</span></div>\
          <div class=\"input\"><input type=\"text\"  name=\"server\" value=\"\"></div>\
        <div class=\"header\"><input class=\"btn\" type=\"submit\" name=\"submit\" value=\"Submit\"></div>\
      </form>\
    </div>\
  </body>\
</html>";
*/


String Page_data="";
String symbol=":";

void ROOT_HTML()
{
  //Page_data =Page_data+ (String)num+ symbol + LoRa_data +"<br>";
  String html = htmlS + Page_data + htmlF;
  server.send(200,"text/html",html);
}

void ROOT_HTMLW()
{
  /*if(server.hasArg("server"))
  {
    Serial.println(server.arg("server"));
    Write_data=server.arg("server");
    WiFiDownLink = true;
    WiFidonwlinkTime = millis();
    unsigned char *puc;
    puc = (unsigned char *)(&Write_data);
    appDataSize = 4;//AppDataSize max value is 64
    appData[0] = puc[0];
    appData[1] = puc[1];
    appData[2] = puc[2];
    appData[3] = puc[3];
    //LoRaWAN.send();
  }
  server.send(200,"text/html",htmlW);
  */
  String html4W = htmlWString;
  server.send(200,"text/html",htmlW);
}

//char txpacket[BUFFER_SIZE];
char rxpacket[BUFFER_SIZE];

static RadioEvents_t RadioEvents;

int16_t txNumber;

int16_t rssi,rxSize;

bool lora_idle = true;


struct __attribute__((packed)) GPSData {
    float batteryLevel;
    int senderID;
    int nodeID;
    float latitude;
    float longitude;
    float altitude;
    int timestamp;
};


const char* ssid = "A_TPLink_to_the_Past";
const char* password = "";//REDACTED FOR UPLOAD

//WebServer server(80);
String loraData = "No data received yet."; // Store incoming LoRa data

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



  // Setup WiFi
WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  if (WiFi.waitForConnectResult() == WL_CONNECTED) {
    server.on("/", ROOT_HTML);
    server.on("/w", ROOT_HTMLW);
    server.begin();
    MDNS.addService("http", "tcp", 80);
    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("View page IP address: ");
    Serial.println(WiFi.localIP());
    Serial.println("Write page IP address: ");
    Serial.print(WiFi.localIP());
    Serial.println("/w");
  } else {
    Serial.println("WiFi Failed");
  }



  // Setup webserver
  //server.on("/", HTTP_GET, []() {
  //  server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='0.5'><style>body { font-family: Arial; background-color: #f4f4f4; text-align: center; padding: 50px; } h1 { color: #333; } p { background: #fff; display: inline-block; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }</style></head><body><h1>LoRa Data</h1><p>" + loraData + "</p></body></html>");
  //});
  //server.begin();
}

void loop() {
  // Handle web requests
  server.handleClient();

  // Check for LoRa packets
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
    Serial.print(incoming.latitude, 8);
    Serial.print(" | Lon: ");
    Serial.print(incoming.longitude, 8);
    Serial.print(" | Alt: ");
    Serial.print(incoming.altitude);
    Serial.print(" | Time: ");
    Serial.print(incoming.timestamp);
    Serial.print(" | Batt: ");
    Serial.println(incoming.batteryLevel);

    //String myString = "test123" + incoming.timestamp; 
String webPageString = 
    String(incoming.batteryLevel, 2) + "," + 
    String(incoming.senderID) + "," + 
    String(incoming.nodeID) + "," + 
    String(incoming.latitude, 8) + "," + 
    String(incoming.longitude, 8) + "," + 
    String(incoming.altitude, 2) + "," + 
    String(incoming.timestamp);

    Page_data = webPageString; // Store received LoRa data for web display
    htmlWString = htmlWString + String(incoming.timestamp);
  
//////////////////////////

    lora_idle = true;
}
