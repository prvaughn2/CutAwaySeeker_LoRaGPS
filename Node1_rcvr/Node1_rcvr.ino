/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3" piece of wire.
  This example is a modified version of the example provided by the Radio Head
  Library which can be found here:
  www.github.com/PaulStoffregen/RadioHeadd
*/

#include <SPI.h>

//Radio Head Library:
#include <RH_RF95.h> 

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
int curr_ID = 1;

// We need to provide the RFM95 module's chip select and interrupt pins to the
// rf95 instance below.On the SparkFun ProRF those pins are 12 and 6 respectively.
RH_RF95 rf95(12, 6);

int LED = 13; //Status LED is on pin 13

int packetCounter = 0; //Counts the number of packets sent
long timeSinceLastPacket = 0; //Tracks the time stamp of last packet received

// The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
// anywhere in the range of 902-928MHz in the Americas.
// Europe operates in the frequencies 863-870, center frequency at 868MHz.
// This works but it is unknown how well the radio configures to this frequency:
//float frequency = 864.1; 
float frequency = 921.2; //Broadcast frequency

void setup()
{
  pinMode(LED, OUTPUT);

  SerialUSB.begin(9600);
  // It may be difficult to read SerialUSB messages on startup. The following line
  // will wait for SerialUSB to be ready before continuing. Comment out if not needed.
  //while(!SerialUSB); 
  SerialUSB.println("RFM Client!"); 

  //Initialize the Radio.
  if (rf95.init() == false){
    SerialUSB.println("Radio Init Failed - Freezing");
    while (1);
  }
  else{
    //An LED inidicator to let us know radio initialization has completed. 
    SerialUSB.println("Transmitter up!"); 
    digitalWrite(LED, HIGH);
    delay(500);
    digitalWrite(LED, LOW);
    delay(500);
  }

  // Set frequency
  rf95.setFrequency(frequency);

   // The default transmitter power is 13dBm, using PA_BOOST.
   // If you are using RFM95/96/97/98 modules which uses the PA_BOOST transmitter pin, then 
   // you can set transmitter powers from 5 to 23 dBm:
   // Transmitter power can range from 14-20dbm.
   rf95.setTxPower(14, false);
}


void loop()
{
  SerialUSB.println("Sending message");

  //Send a message to the other radio
  uint8_t toSend[] = "Hi there!";
  //sprintf(toSend, "Hi, my counter is: %d", packetCounter++);
  rf95.send(toSend, sizeof(toSend));
  rf95.waitPacketSent();

  // Now wait for a reply
  //byte buf[RH_RF95_MAX_MESSAGE_LEN];
  uint8_t buf[sizeof(curr_info)];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(2000)) {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      SerialUSB.print("Got reply: ");
      SerialUSB.println(sizeof(buf));
      
//    SerialUSB.println((char*)buf);

      // convert curr_info struct to byte array
      //uint8_t temp[sizeof(curr_info)];
      //temp = (char*)buf;
     //char temp[sizeof(buf)];
     memcpy(&curr_info, &buf, sizeof(buf));
     //SerialUSB.print("temp:"); SerialUSB.print(temp); SerialUSB.println(".");
     //memcpy(curr_info, &temp, sizeof(temp));
    
    //curr_info = (information)&buf;

    SerialUSB.print("0sLAT=");  SerialUSB.println(curr_info.curr_Lat,6);
    
    SerialUSB.print("0sLONG="); SerialUSB.println(curr_info.curr_Lon,6);
    SerialUSB.print("0sALT=");  SerialUSB.println(curr_info.curr_Alt);
    SerialUSB.print("0sTime="); SerialUSB.print(curr_info.curr_Hour); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(curr_info.curr_min); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(curr_info.curr_sec); // Second (0-59) (u8)
    SerialUSB.print("0sDate="); SerialUSB.println(curr_info.curr_date); // Raw date in DDMMYY format (u32)

      //SerialUSB.println((char*)buf);
      //SerialUSBUSB.print(" RSSI: ");
      //SerialUSBUSB.print(rf95.lastRssi(), DEC);
    }
    else {
      SerialUSB.println("Receive failed");
    }
  }
  else {
    SerialUSB.println("No reply, is the receiver running?");
  }
  delay(500);
}
