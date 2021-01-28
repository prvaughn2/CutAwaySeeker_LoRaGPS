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

// FUTURE DEVELOPMENT Battery info
const long InternalReferenceVoltage = 1062;  // Adjust this value to your board's specific internal BG voltage
//TODO verify this number...
 int A0adc;
float battVolts;

//Information for current readings location and time. This is the columns for the manifest.
struct information {
  uint16_t readingID;
  uint8_t nodeID ;
  double curr_Lat;
  double curr_Lon;
  double curr_Alt;
  uint8_t curr_Hour; 
  uint8_t curr_min;
  uint8_t curr_sec;
  uint32_t curr_date;
  //int curr_battery;
} curr_info;



//Counter used to say what reading we are on. This will exceed the NumofReadings and overwrite the oldest reading. 0 becomes 51 etc. 
int readingCounter = 0; 
int posofManifestDataWriter = 0;

//3d table for all the information that I know (AKA Manifest) tablenamedefinedas[rows][col][depth]
#define NumofNodes 2     //Starting with 2 nodes. this can be a variable for later development. 
#define NumofReadings 3 //Start with last 50 readings
#define NumofCol 9      //See struct above

//Only this nodes readings history 
information thisNodesManifest[NumofReadings];

//need 2 (depth)  of the things above and call it manifest
struct manifestType
{
  information thisNodesManifest[NumofReadings];
}manifest[NumofNodes];

 
 //manifest[NumofReadings][NumofCol][NumofNodes];
//Not sure if this ^^ is correct. Manifest is a 3d table where depth is the nodes (ID) rows are readings, cols are reading types. Is the struct in whole of type information or something else?

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
  Serial1.begin(9600);

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

  //There are 4 activities to this project:
  //1. Get my position and update my manifest.
  //2. Relay my manifest to anyone who will listen.
  //3. Listen for anyone's transmitting data. If so, update my manifest
  //4. Relay my updated manifest to the gateway, and in turn the server.

  //For now, I am doing the first 3 activities with the hope that the gateway will be for future development. 

    
  //SerialUSB.println("begining loop");
  
  //FUTURE DEVELOPMENT TO GET BATTERY STATUS:
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (3.3 / 1023.0);

  //SerialUSB.println(voltage);
  //END BATTERY CHECK

  //ACTIVITY 1:
   if (SerialUSB.available()) {          // If anything comes in Serial (USB),
    Serial1.write(SerialUSB.read());     // read it and send it out Serial1 (pins 0 & 1) 
   }

   if (Serial1.available()) {            // If anything comes in Serial1 (pins 0 & 1)
    //SerialUSB.write(Serial1.read());   // read it and send it out Serial (USB)
    
    
    //Process the NMEA input
    gps.encode(Serial1.read());
    /*
    SerialUSB.print("readingCounter=");  SerialUSB.println(readingCounter);
    SerialUSB.print("LAT=");  SerialUSB.println(gps.location.lat(), 6);
    
    SerialUSB.print("LONG="); SerialUSB.println(gps.location.lng(), 6);
    SerialUSB.print("ALT=");  SerialUSB.println(gps.altitude.meters());
    SerialUSB.print("Time="); SerialUSB.print(gps.time.hour()); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(gps.time.minute()); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(gps.time.second()); // Second (0-59) (u8)
    SerialUSB.print("Date="); SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
    */
    curr_info.readingID = readingCounter;
    curr_info.nodeID    = curr_ID ;
    curr_info.curr_Lat  = gps.location.lat();
    curr_info.curr_Lon  = gps.location.lng();
    curr_info.curr_Alt  = gps.altitude.meters();
    curr_info.curr_Hour = gps.time.hour();
    curr_info.curr_min  = gps.time.minute();
    curr_info.curr_sec  = gps.time.second();
    curr_info.curr_date = gps.date.value();

    //Add the above information to the current Node's manifest

    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].readingID = readingCounter;
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].nodeID    = curr_ID ;
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_Lat  = gps.location.lat();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_Lon  = gps.location.lng();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_Alt  = gps.altitude.meters();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_Hour = gps.time.hour();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_min  = gps.time.minute();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_sec  = gps.time.second();
    manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].curr_date = gps.date.value();
    
    //SerialUSB.print("readingID: "); SerialUSB.println(manifest[curr_ID].thisNodesManifest[posofManifestDataWriter].readingID);
    
    //SerialUSB.print("posofManifestDataWriter: "); SerialUSB.println(posofManifestDataWriter);
    
    //Update counters
    readingCounter = readingCounter + 1;
    posofManifestDataWriter = posofManifestDataWriter + 1;
    
    if (posofManifestDataWriter > NumofReadings){
      posofManifestDataWriter = 0; //manifest is full. delete oldest reading. 
    }
  



      //FUTURE DEVELOPMENT
    A0adc = analogRead(A0); // dummy reading
    A0adc = analogRead(A0); // actual reading
    battVolts = A0adc * 0.004567; // calibrate here
    Serial.print("Volts:");
    Serial.println(battVolts);
    //curr_info.curr_battery = getBandgap();

  }
  //END ACTIVITY 1

    /*
    SerialUSB.print("2LAT=");  SerialUSB.println(curr_info.curr_Lat,6);
    SerialUSB.print("2LONG="); SerialUSB.println(curr_info.curr_Lon,6);
    SerialUSB.print("2ALT=");  SerialUSB.println(curr_info.curr_Alt);
    SerialUSB.print("2Time="); SerialUSB.print(curr_info.curr_Hour); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(curr_info.curr_min); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(curr_info.curr_sec); // Second (0-59) (u8)
    SerialUSB.print("2Date="); SerialUSB.println(curr_info.curr_date); // Raw date in DDMMYY format (u32)
    */
  
  // START ACTIVITY 2
  //The communication has a few steps: 1 announce that I am about to send. Get a reply that someone is listening (if no response, don't send anything). Send my entire manifest.
  
  SerialUSB.println("Sending message anyone out there.");

  //Send a message to the other radio
  uint8_t toSendRUThere[] = "Hi there! Anyone out there?"; 
  //sprintf(toSend, "Hi, my counter is: %d", packetCounter++);
  rf95.send(toSendRUThere, sizeof(toSendRUThere));
  rf95.waitPacketSent();
  
  if (rf95.available()){
    // Should be a message for us now
    uint8_t buf[RH_RF95_MAX_MESSAGE_LEN];
    uint8_t len = sizeof(buf);

    if (rf95.recv(buf, &len))
    {
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

      //Actualyl send the current info. This will be changed to the manifest.


/////////////////this works before I change it/////////////////////////////
      // convert curr_info struct to byte array
      //uint8_t buffer[sizeof(curr_info)];

      //memcpy(buffer, &curr_info, sizeof(curr_info));
////////////////////////////////





      // Send the manifest (megaphone style)
      // convert curr_info struct to byte array
      uint8_t buffer[sizeof(manifest)];

      memcpy(buffer, &manifest, sizeof(manifest));

      if(sizeof(buffer) < 1)
      {
        SerialUSB.println("buffer is empty"); 
      }
      else
      SerialUSB.print("buffers size:"); SerialUSB.println(sizeof(buffer));
      
      //SerialUSB.print("buffer:"); 
      //SerialUSB.print(buffer); 
      //SerialUSB.println(".");
            
      //pass buffer to radio
      rf95.send(buffer, sizeof(buffer));
      
      rf95.waitPacketSent();
      SerialUSB.println("Sent my manifest out. ");
      digitalWrite(LED, LOW); //Turn off status LED

    }
    else
      SerialUSB.println("Recieve failed");
  }
  else
  {
      SerialUSB.println("No one is out there. I am alone. :( ");
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

  //END ACTIVITY 2?

  //START ACTIVITY 3 - rec. others manifest and update mine. 

    SerialUSB.println("Sending message I am ready to RCV your manifest");// why am I doing this?????????? not jsut this fn,. but in general... I am the only benefit and even if I release it, there wont be anythign on the other side.....
  //i am doing this for the next dude. 

   
  //Send a message to the other radio
  uint8_t toSend[] = "Hi there! I am ready to recieve your manifest.";

  rf95.send(toSend, sizeof(toSend));
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[sizeof(curr_info)];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(2000))//This will change to whatever I want the timeout interval to be. TODO
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      SerialUSB.print("Got a reply of size: ");
      SerialUSB.println(sizeof(buf));

     manifestType recievedManifest[NumofNodes]; // I should have recieved a manifest of the 2 nodes. 
     
     memcpy(&recievedManifest, &buf, sizeof(buf));
     
    //reconcile my manifest with the one that I just recieved. 
    //TODOing...

    //Print out stuff for debug
    //SerialUSB.print("0sReadingID=");  SerialUSB.println(recievedManifest[0].thisNodesManifest[0].readingID);

    ///functionize this later as ReconcileManifest(recievedManifest)!!!!!!!!!!

    //the recieved manifest is depth=2 columns=10 and rows=50max

    //this is a terrible way to do this but it will work for now....
    //the current manifdest is like this in my head: [depth][cols][rows]
    //I know is should be row col height. IDFC.
    
    for(int depth = 0 ; depth < NumofNodes; depth = depth+1)
    {
        //SerialUSB.print("Pass#:"); SerialUSB.println(depth);
        //make sure we arent reading our own values. my manifest of me over theirs of me. 
        if (recievedManifest[depth].thisNodesManifest[0].readingID != curr_ID)
        {
            for (int row = 0; row < NumofReadings; row = row + 1)
            {
                /*
                SerialUSB.print("1sreadingNum=");  SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].readingID);
                SerialUSB.print("1sLAT=");  SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].curr_Lat,6);
                SerialUSB.print("1sLONG="); SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].curr_Lon,6);
                SerialUSB.print("1sALT=");  SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].curr_Alt);
                SerialUSB.print("1sTime="); SerialUSB.print(recievedManifest[depth].thisNodesManifest[row].curr_Hour); // Hour (0-23) (u8) 
                SerialUSB.print(":");     SerialUSB.print(recievedManifest[depth].thisNodesManifest[row].curr_min); // Minute (0-59) (u8)
                SerialUSB.print(":");     SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].curr_sec); // Second (0-59) (u8)
                SerialUSB.print("1sDate="); SerialUSB.println(recievedManifest[depth].thisNodesManifest[row].curr_date); // Raw date in DDMMYY format (u32) 
                SerialUSB.println();
                SerialUSB.println();
                */
                //add this  information to this nodes manifest
                //this will be done in the reconcil manifest function
                


            }
        }
    }
      
    }
    else 
    {
      SerialUSB.println("Receive failed");
    }
  }
  else 
  {
    SerialUSB.println("No reply, is the receiver running?");
  }
  delay(5);//this will go away later

  //END ACTIVITY 3

  //START ACTIVITY 4------------FUTURE DEVELOPMENT
  //END ACTIVITY 4
  
}
