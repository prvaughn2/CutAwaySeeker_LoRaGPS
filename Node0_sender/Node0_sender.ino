/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3" piece of wire.
  This example is a modified version of the example provided by the Radio Head
  Library which can be found here: 
  www.github.com/PaulStoffregen/RadioHeadd
*/

#include <SPI.h>

//Radio Head Library: 
#include <RH_RF95.h>

//NMEA Parsing
#include "TinyGPS++.h"
TinyGPSPlus gps;

// Battery info
const long InternalReferenceVoltage = 1062;  // Adjust this value to your board's specific internal BG voltage
//TODO verify this number...
 int A0adc;
float battVolts;


//Information for current location and time
struct information {
  int ID ;
  double curr_Lat;
  double curr_Lon;
  double curr_Alt;
  uint8_t curr_Hour; 
  uint8_t curr_min;
  uint8_t curr_sec;
  uint32_t curr_date;
  int curr_battery;
} curr_info;

//ID for this node
int curr_ID = 0;


// We need to provide the RFM95 module's chip select and interrupt pins to the 
// rf95 instance below.On the SparkFun ProRF those pins are 12 and 6 respectively.
RH_RF95 rf95(12, 6);

int LED = 13; //Status LED on pin 13

int packetCounter = 0; //Counts the number of packets sent
long timeSinceLastPacket = 0; //Tracks the time stamp of last packet received
// The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
// anywhere in the range of 902-928MHz in the Americas.
// Europe operates in the frequencies 863-870, center frequency at 
// 868MHz.This works but it is unknown how well the radio configures to this frequency:
//float frequency = 864.1;
float frequency = 921.2;

//Functions


void setup()
{
  pinMode(LED, OUTPUT);

  SerialUSB.begin(9600);
  // It may be difficult to read SerialUSB messages on startup. The following
  // line will wait for SerialUSB to be ready before continuing. Comment out if not needed.
  //while(!SerialUSB);
  SerialUSB.println("RFM Server!");

  //Initialize the serial pin input
  Serial.begin(9600);
  //Serial.println("Hello");
    Serial1.begin(9600);
      //Serial.println("Hello2");

  //Initialize the Radio. 
  if (rf95.init() == false){
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  }
  else{
  // An LED indicator to let us know radio initialization has completed.
    SerialUSB.println("Receiver up!");
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  rf95.setFrequency(frequency); 

   // The default transmitter power is 13dBm, using PA_BOOST.
   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
   // you can set transmitter powers from 5 to 23 dBm:
   // Transmitter power can range from 14-20dbm.
   rf95.setTxPower(14, false);

}

void loop()
{
  //SerialUSB.println("begining loop");
  /////////////////////
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (3.3 / 1023.0);

  //SerialUSB.println(voltage);
  //////////////////////
  
if (SerialUSB.available()) {      // If anything comes in Serial (USB),

    Serial1.write(SerialUSB.read());   // read it and send it out Serial1 (pins 0 & 1)
    
    

  }

  if (Serial1.available()) {     // If anything comes in Serial1 (pins 0 & 1)

    //SerialUSB.write(Serial1.read());   // read it and send it out Serial (USB)

    //This is where I can process the NMEA input
    gps.encode(Serial1.read());

    /*
    SerialUSB.print("LAT=");  SerialUSB.println(gps.location.lat(), 6);
    SerialUSB.print("LONG="); SerialUSB.println(gps.location.lng(), 6);
    SerialUSB.print("ALT=");  SerialUSB.println(gps.altitude.meters());
    SerialUSB.print("Time="); SerialUSB.print(gps.time.hour()); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(gps.time.minute()); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(gps.time.second()); // Second (0-59) (u8)
    SerialUSB.print("Date="); SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
    */
    curr_info.ID        = curr_ID ;
    curr_info.curr_Lat  = gps.location.lat();
    curr_info.curr_Lon  = gps.location.lng();
    curr_info.curr_Alt  = gps.altitude.meters();
    curr_info.curr_Hour = gps.time.hour();
    curr_info.curr_min  = gps.time.minute();
    curr_info.curr_sec  = gps.time.second();
    curr_info.curr_date = gps.date.value();


    A0adc = analogRead(A0); // dummy
    A0adc = analogRead(A0); // reading
    battVolts = A0adc * 0.004567; // calibrate here
    Serial.print("Volts:");
    Serial.println(battVolts);
    //curr_info.curr_battery = getBandgap();

  }

    /*
    SerialUSB.print("2LAT=");  SerialUSB.println(curr_info.curr_Lat,6);
    SerialUSB.print("2LONG="); SerialUSB.println(curr_info.curr_Lon,6);
    SerialUSB.print("2ALT=");  SerialUSB.println(curr_info.curr_Alt);
    SerialUSB.print("2Time="); SerialUSB.print(curr_info.curr_Hour); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(curr_info.curr_min); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(curr_info.curr_sec); // Second (0-59) (u8)
    SerialUSB.print("2Date="); SerialUSB.println(curr_info.curr_date); // Raw date in DDMMYY format (u32)
    */
  
  
  if (rf95.available()){
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len)){
      digitalWrite(LED, HIGH); //Turn on status LED
      timeSinceLastPacket = millis(); //Timestamp this packet

      SerialUSB.print("Got message: ");
      SerialUSB.print((char*)buf);
      //SerialUSB.print(" RSSI: ");
      //SerialUSB.print(rf95.lastRssi(), DEC);
      SerialUSB.println();

      // Send a reply
      //uint8_t toSend[] = "Hello Back!"; 
      //rf95.send(toSend, sizeof(toSend));

      // convert curr_info struct to byte array
      uint8_t buffer[sizeof(curr_info)];

      memcpy(buffer, &curr_info, sizeof(curr_info));

      if(sizeof(buffer) < 1)
      {
        SerialUSB.println("buffer is empty"); 
      }
      else
      SerialUSB.println(sizeof(buffer));
      
      //SerialUSB.print("buffer:"); 
      //SerialUSB.print(buffer); 
      //SerialUSB.println(".");
            
      //pass buffer to radio
      rf95.send(buffer, sizeof(buffer));
      
      
      
      rf95.waitPacketSent();
      SerialUSB.println("Sent a reply");
      digitalWrite(LED, LOW); //Turn off status LED

    }
    else
      SerialUSB.println("Recieve failed");
  }


  
 // send data only when you receive data:
  if (Serial.available() > 0) {
    // read the incoming byte:
    int incomingByte = Serial.read();

    // say what you got:
    Serial.print("I received: ");
    Serial.println(incomingByte, DEC);
  }
  
  //Turn off status LED if we haven't received a packet after 1s
  if(millis() - timeSinceLastPacket > 1000){
    digitalWrite(LED, LOW); //Turn off status LED
    timeSinceLastPacket = millis(); //Don't write LED but every 1s
  }

  
}
