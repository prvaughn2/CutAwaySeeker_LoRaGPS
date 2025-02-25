/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3" piece of wire.
  This example is a modified version of the example provided by the Radio Head
  Library which can be found here: 
  www.github.com/PaulStoffregen/RadioHeadd


Definitions:
manifest - the total of everything I know: my location present and past, and the recorded locations of everything I have heard.
information - a struct containing Lat Lon, Alt, Time, etc.
thisNodesManifest - my page of the manifest

  
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

//Counter used to say what reading we are on. This will exceed the NumofReadings and overwrite the oldest reading. 0 becomes 51 etc. 
int readingCounter = 0; 
int posofManifestDataWriter = 0;

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
  //uint32_t curr_date;
  //int curr_battery;
} curr_info, incoming_curr_info;


//3d table for all the information that I know (AKA Manifest) tablenamedefinedas[rows][col][depth]
#define initialNumofNodes 2
int NumofNodes = initialNumofNodes;     //Starting with 2 nodes aka pages for manifest aka devices. this can be a variable for later development. 
#define NumofReadings 3 //Start with last 10 readings
#define NumofCol 9      //See struct above

//Only this nodes readings. AKA my page of my manifest. 
information thisNodesManifest[NumofReadings];

//I see this as a "book" titled Manifest. Each page represents a node, and each page contains that node's data
struct manifestType
{
  information thisNodesManifest[NumofReadings];
}manifest[initialNumofNodes];

 //ID for this node
int curr_ID = 1;

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

//Setup

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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// MAIN LOOP /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void loop()
{
  // MAIN LOOP
  // There are 4 activities to this project:
  //1. Get my position and update my manifest.
  //2. Relay my current POSITION to anyone who will listen (UDP style).
  //3. Listen for anyone's transmitting data. If so, receive it and update my manifest. 
  //4. Relay my updated manifest to the gateway, and in turn the server. (FUTURE DEVELOPMENT)

  //For now, I am doing the first 3 activities with the hope that the gateway will be for future development.
  
  //FUTURE DEVELOPMENT TO GET BATTERY STATUS:
  /*
  // read the input on analog pin 0:
  int sensorValue = analogRead(A0);

  // Convert the analog reading (which goes from 0 - 1023) to a voltage (0 - 5V):
  float voltage = sensorValue * (3.3 / 1023.0);

  //SerialUSB.println(voltage);
  */
  //END BATTERY CHECK
  SerialUSB.println("STEP 0: Test output");
  //ACTIVITY 1:
   if (SerialUSB.available()) {          // If anything comes in Serial (USB),
    Serial1.write(SerialUSB.read());     // read it and send it out Serial1 (pins 0 & 1) 
    
   }

   //supressing for debugging. This is where the LAT/LON is coming in.  if (Serial1.available()) // If anything comes in Serial1 (pins 0 & 1)
   {            
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 1 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Get my Position: process the NMEA input
    SerialUSB.println("STEP 1: Reading and recording GPS data...........................................................................! ");
    gps.encode(Serial1.read());
    
    //SerialUSB.print("readingCounter=");  SerialUSB.println(readingCounter);
    //SerialUSB.print("TEST!!!!!!!!!!!!!LAT=");  SerialUSB.println(gps.location.lat(), 6);
    /*
    SerialUSB.print("LONG="); SerialUSB.println(gps.location.lng(), 6);
    SerialUSB.print("ALT=");  SerialUSB.println(gps.altitude.meters());
    SerialUSB.print("Time="); SerialUSB.print(gps.time.hour()); // Hour (0-23) (u8) 
    SerialUSB.print(":");     SerialUSB.print(gps.time.minute()); // Minute (0-59) (u8)
    SerialUSB.print(":");     SerialUSB.println(gps.time.second()); // Second (0-59) (u8)
    SerialUSB.print("Date="); SerialUSB.println(gps.date.value()); // Raw date in DDMMYY format (u32)
    */
    curr_info.readingID = readingCounter;
    curr_info.nodeID    = curr_ID ;
    curr_info.curr_Lat  = gps.location.lat()+10; //+10 for debug
    curr_info.curr_Lon  = gps.location.lng();
    curr_info.curr_Alt  = gps.altitude.meters();
    curr_info.curr_Hour = gps.time.hour();
    curr_info.curr_min  = gps.time.minute();
    curr_info.curr_sec  = gps.time.second();
    //curr_info.curr_date = gps.date.value();

    //Update my manifest: Add the above information to the current Node's manifest

    manifest[0].thisNodesManifest[posofManifestDataWriter].readingID = readingCounter;
    manifest[0].thisNodesManifest[posofManifestDataWriter].nodeID    = curr_ID ;
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Lat  = gps.location.lat()+10; //+10 for debug
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Lon  = gps.location.lng();
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Alt  = gps.altitude.meters();
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Hour = gps.time.hour();
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_min  = gps.time.minute();
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_sec  = gps.time.second();
    //thisNodesManifest[posofManifestDataWriter].curr_date = gps.date.value();
    
 

    SerialUSB.print("TEST!!!!!!!!!!!!!LAT=");  SerialUSB.println(thisNodesManifest[posofManifestDataWriter].curr_Lat);
    SerialUSB.print("TEST!!!!!!!!!!!!!inside the manifest LAT=");  SerialUSB.println(manifest[0].thisNodesManifest[0].curr_Lat);
    
    
    
    //Update counters
    readingCounter = readingCounter + 1;
    posofManifestDataWriter = posofManifestDataWriter + 1;

    //If manifest is full, reset the counter to the top to delete the oldest reading NOTE: this list is now not sorted....
    if (posofManifestDataWriter > NumofReadings){
      posofManifestDataWriter = 0; 
    }
  



      //FUTURE DEVELOPMENT
        /*
      A0adc = analogRead(A0); // dummy reading
      A0adc = analogRead(A0); // actual reading
      battVolts = A0adc * 0.004567; // calibrate here
      Serial.print("Volts:");
      Serial.println(battVolts);
      //curr_info.curr_battery = getBandgap();
      */
  
      //SANITY CHECK
      /*
      SerialUSB.println("Sanity Check................................");
      print1PageofManifest(thisNodesManifest);
      SerialUSB.println("End of Sanity Check................................");
      */
  }
  
  //END ACTIVITY 1
  
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 2 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////  
  // START ACTIVITY 2
  // Lets do this UDP style - blast it out and cross your fingers that someone hears you. X)
  
  SerialUSB.println("Begin Step2: Send currentinfo.............. ");

  //Send manifest a message to the other radio
   // rf95.send(curr_info, sizeof(curr_info));

  // I think this waits for a packet to come in. If there is something, send out your stuff... This is incorrect... 
  //SerialUSB.println("Waiting for packet to complete......................");
  //rf95.waitPacketSent();
  //delay(5);
  
  //SerialUSB.println("Sent. awaiting reply. ");
  
  //if (rf95.available()){
    /*
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

*/

/////////////////this works before I change it/////////////////////////////
      // convert curr_info struct to byte array
      //uint8_t buffer[sizeof(curr_info)];

      //memcpy(buffer, &curr_info, sizeof(curr_info));
////////////////////////////////



      // Send the manifest. This needs to be done "page by page" as the packet can become large.
      //This is still in DEV so I am only sending 1 page  to start
      
      SerialUSB.println("Sending -curr_info- to anyone who will listen");
      // convert the page struct to byte array
      uint8_t buffer[sizeof(curr_info)];

      memcpy(buffer, &curr_info, sizeof(curr_info));

      //SerialUSB.println("max len:"+RH_RF95_MAX_MESSAGE_LEN);



      
      // convert curr_info struct to byte array
      //uint8_t buffer[sizeof(manifest)];

      //memcpy(buffer, &manifest, sizeof(manifest));

      if(sizeof(buffer) < 1)
      {
        SerialUSB.println("buffer is empty...:("); 
      }
      else
      SerialUSB.print("the sending buffer's size:"); SerialUSB.println(sizeof(buffer));
      
      //SerialUSB.print("buffer:"); 
      //SerialUSB.print(buffer); 
      //SerialUSB.println(".");
            
      //pass buffer to radio
      rf95.send(buffer, sizeof(buffer));
      
      rf95.waitPacketSent();
      SerialUSB.println("Sent my manifest out successfully. ");
      digitalWrite(LED, LOW); //Turn off status LED

    //}
    //else
    //  SerialUSB.println("Receive failed. Are you sure you are not alone?");
  
  //END ACTIVITY 2?
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 3 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //START ACTIVITY 3 - rec. others manifest and update mine. 

 /*
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
*/
  


/*

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

      SerialUSB.print("buffer is: ");
      SerialUSB.println(converter(buf));

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
*/


/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////begin copy paste area


  //Start Activity 3

  SerialUSB.println("Begining Step 3: listen for an incoming packet for 2 seconds");
  //wait for packet to comlpete
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

      SerialUSB.print("buffer should be of curr_info. This is the actual packet: ");
      SerialUSB.println(converter(buf));

     //currently this is 1 page of manifest. this will be bigger later? or will i only ever recieve 1 page at a time?
     information incomingNodesManifest[NumofReadings];

     information incomingNodesCurrInfo;
     //manifestType recievedManifest[NumofNodes]; // I should have recieved a manifest of the 2 nodes. 
     
     memcpy(&incoming_curr_info, &buf, sizeof(buf));

     

    //SANITY CHECK #2
    SerialUSB.println("Sanity Check #2................................");
    SerialUSB.print("incoming Nodes curr info LAT------: "); SerialUSB.println(incoming_curr_info.curr_Lat);
    //print1PageofManifest(incomingNodesManifest); 
    
    SerialUSB.println("End of Sanity Check #2................................");
     
     
    //reconcile my manifest with the one that I just recieved. 
    //TODOing...
    addIncomingCurrInfoToMyManifest(incoming_curr_info);

    
    //ReconcileManifest(incomingNodesManifest);
    //SerialUSB.print("checking what the nodeid is of the last page:");
    //SerialUSB.println(manifest[NumofNodes].thisNodesManifest[0].nodeID);

    //Print out stuff for debug
    //SerialUSB.print("0sReadingID=");  SerialUSB.println(recievedManifest[0].thisNodesManifest[0].readingID);

    ///functionize this later as ReconcileManifest(recievedManifest)!!!!!!!!!!

    //the recieved manifest is depth=2 columns=10 and rows=50max

    //this is a terrible way to do this but it will work for now....
    //the current manifdest is like this in my head: [depth][cols][rows]
    //I know is should be row col height. IDFC.

    /*
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
                
                //add this  information to this nodes manifest
                //this will be done in the reconcil manifest function
                
              

            }
        }
    }
    */
     
    }
    else 
    {
      SerialUSB.println("Receive failed");
    }
  }
  else
  {
    SerialUSB.println("Didn't get any packets. Is anyone sending?");
  }

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////end copy paste area

  //END ACTIVITY 3

  //START ACTIVITY 4------------FUTURE DEVELOPMENT
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 4 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //END ACTIVITY 4

  //Start sanity check 
  SerialUSB.println("Sanity Check3- Entire manifest");
  printEntireManifest(manifest);
  SerialUSB.println("End of Sanity Check3..................END OF PROGRAM.......................................................................................");
  //end sanity check
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addIncomingCurrInfoToMyManifest(information someNodesCurrInfo)
{
  SerialUSB.print("Adding incming nodes curr info to my manifest");

  
  // We just got a page of a manifest from an outside node. We need to combine/update this new information with ours.

  //First check to see if I already have a page for the incoming node's info

  bool foundPageInMyManifest = false;
  
  for(int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {
    
    SerialUSB.print("check="); SerialUSB.println(someNodesCurrInfo.nodeID);
    SerialUSB.print("againstmanifest="); SerialUSB.println(manifest[counter].thisNodesManifest[0].nodeID);
    if(someNodesCurrInfo.nodeID == manifest[counter].thisNodesManifest[0].nodeID)
    {
        SerialUSB.println("Found the page associated with the incoming pages ID.");
        foundPageInMyManifest = true;
        // Found the page. Update my manifest with the new information.

        // go line by line of my manifest and see if there is new information (somewhat bubble sort like?)
        int incomingRowCounter = 0;
        int myManifestRowCounter = 0;
        for(int readingsCounter = 0 ; readingsCounter < NumofNodes ; readingsCounter = readingsCounter + 1)//better, but not great code
        {
           //if the incoming nodes reading ID is greater than ours (aka that reading is newer than ours) overwrite that reading in my book)
           if(someNodesCurrInfo.readingID > manifest[counter].thisNodesManifest[myManifestRowCounter].readingID )
           {
                //take that reading over mine
            
                manifest[counter].thisNodesManifest[myManifestRowCounter].readingID = someNodesCurrInfo.readingID;
                manifest[counter].thisNodesManifest[myManifestRowCounter].nodeID    = someNodesCurrInfo.nodeID;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lat  = someNodesCurrInfo.curr_Lat;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lon  = someNodesCurrInfo.curr_Lon;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Alt  = someNodesCurrInfo.curr_Alt;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Hour = someNodesCurrInfo.curr_Hour;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_min  = someNodesCurrInfo.curr_min;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_sec  = someNodesCurrInfo.curr_sec;
                
            //Update counters
            myManifestRowCounter = myManifestRowCounter + 1;
           }
           
           incomingRowCounter = incomingRowCounter + 1;
           
        }
        
    }
    
  }
  if (foundPageInMyManifest == false)
  {
      SerialUSB.println("The incoming page's ID is not in my manifest. Adding a new page to my manifest. ShortSanity checking ID:");
      SerialUSB.println(someNodesCurrInfo.nodeID);

      
      NumofNodes = NumofNodes + 1;

      manifestType BiggerManifest[NumofNodes];

      //copy info to new struct
      
      for(int i = 0; i < (NumofNodes - 1) ; i = i + 1)
      {
        BiggerManifest[i] = manifest[i];
      }
      free(manifest); //Not sure if this will work.........
      //manifestType manifest[NumofNodes];

      for(int i = 0; i < (NumofNodes - 1) ; i = i + 1)
      {
        manifest[i] = BiggerManifest[i];
      }

      //Add the last page to the end
      
      for(int i = 0; i < NumofReadings ; i = i + 1)
      {
          manifest[NumofNodes].thisNodesManifest[i].readingID = someNodesCurrInfo.readingID;
          manifest[NumofNodes].thisNodesManifest[i].nodeID    = someNodesCurrInfo.nodeID;
          manifest[NumofNodes].thisNodesManifest[i].curr_Lat  = someNodesCurrInfo.curr_Lat;
          manifest[NumofNodes].thisNodesManifest[i].curr_Lon  = someNodesCurrInfo.curr_Lon;
          manifest[NumofNodes].thisNodesManifest[i].curr_Alt  = someNodesCurrInfo.curr_Alt;
          manifest[NumofNodes].thisNodesManifest[i].curr_Hour = someNodesCurrInfo.curr_Hour;
          manifest[NumofNodes].thisNodesManifest[i].curr_min  = someNodesCurrInfo.curr_min;
          manifest[NumofNodes].thisNodesManifest[i].curr_sec  = someNodesCurrInfo.curr_sec;
      }
      SerialUSB.print("finally, checking what the nodeid is of the last page:");
      SerialUSB.println(manifest[NumofNodes].thisNodesManifest[0].nodeID);

      SerialUSB.print("It should be:");
      SerialUSB.println(someNodesCurrInfo.nodeID);
      
  }


  
  SerialUSB.println("END addingtomymanifest ;;;;;;;;;;;;;");
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  
  
}

String converter(uint8_t *str){
    return String((char *)str);
}
void printEntireManifest(manifestType Manifest[]){
  for(int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {
      SerialUSB.print("Manifest of Node ");SerialUSB.println(counter);
      
      print1PageofManifest(Manifest[counter].thisNodesManifest);
      SerialUSB.println();
     
  }
  
}
void print1PageofManifest(information* aNodesManifest){

   for (int row = 0; row < NumofReadings ; row = row + 1)
     {
        SerialUSB.print("Manifest Page # "); SerialUSB.println(row);
        SerialUSB.print("ID=");  SerialUSB.println(aNodesManifest[row].nodeID);
        SerialUSB.print("LAT=");  SerialUSB.println(aNodesManifest[row].curr_Lat,6);
        SerialUSB.print("LONG="); SerialUSB.println(aNodesManifest[row].curr_Lon,6);
     }
}
void ReconcileManifest(information incomingNodesManifest[])
{

  // We just got a page of a manifest from an outside node. We need to combine/update this new information with ours.

  //First check to see if I already have a page for the incoming node's info

  bool foundPageInMyManifest = false;
  
  for(int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {
    
    SerialUSB.print("check="); SerialUSB.println(incomingNodesManifest[0].nodeID);
    SerialUSB.print("againstmanifest="); SerialUSB.println(manifest[counter].thisNodesManifest[0].nodeID);
    if(incomingNodesManifest[0].nodeID == manifest[counter].thisNodesManifest[0].nodeID)
    {
        SerialUSB.println("Found the page associated with the incoming pages ID.");
        foundPageInMyManifest = true;
        // Found the page. Update my manifest with the new information.

        // go line by line of my manifest and see if there is new information (somewhat bubble sort like?)
        int incomingRowCounter = 0;
        int myManifestRowCounter = 0;
        for(int readingsCounter = 0 ; readingsCounter < NumofNodes ; readingsCounter = readingsCounter + 1)//better, but not great code
        {
           //if the incoming nodes reading ID is greater than ours (aka that reading is newer than ours) overwrite that reading in my book)
           if(incomingNodesManifest[incomingRowCounter].readingID > manifest[counter].thisNodesManifest[myManifestRowCounter].readingID )
           {
                //take that reading over mine
            
                manifest[counter].thisNodesManifest[myManifestRowCounter].readingID = incomingNodesManifest[incomingRowCounter].readingID;
                manifest[counter].thisNodesManifest[myManifestRowCounter].nodeID    = incomingNodesManifest[incomingRowCounter].nodeID;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lat  = incomingNodesManifest[incomingRowCounter].curr_Lat;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lon  = incomingNodesManifest[incomingRowCounter].curr_Lon;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Alt  = incomingNodesManifest[incomingRowCounter].curr_Alt;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Hour = incomingNodesManifest[incomingRowCounter].curr_Hour;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_min  = incomingNodesManifest[incomingRowCounter].curr_min;
                manifest[counter].thisNodesManifest[myManifestRowCounter].curr_sec  = incomingNodesManifest[incomingRowCounter].curr_sec;
                
            //Update counters
            myManifestRowCounter = myManifestRowCounter + 1;
           }
           
           incomingRowCounter = incomingRowCounter + 1;
           
        }
        
    }
    
  }
  if (foundPageInMyManifest == false)
  {
      SerialUSB.println("The incoming page's ID is not in my manifest. Adding a new page to my manifest TODO. ShortSanity checking ID:");
      SerialUSB.println(incomingNodesManifest[0].nodeID);

      
      NumofNodes = NumofNodes + 1;

      manifestType BiggerManifest[NumofNodes];

      //copy info to new struct
      
      for(int i = 0; i < (NumofNodes - 1) ; i = i + 1)
      {
        BiggerManifest[i] = manifest[i];
      }
      free(manifest); //Not sure if this will work.........
      //manifestType manifest[NumofNodes];

      for(int i = 0; i < (NumofNodes - 1) ; i = i + 1)
      {
        manifest[i] = BiggerManifest[i];
      }

      //Add the last page to the end
      
      for(int i = 0; i < NumofReadings ; i = i + 1)
      {
          manifest[NumofNodes].thisNodesManifest[i].readingID = incomingNodesManifest[i].readingID;
          manifest[NumofNodes].thisNodesManifest[i].nodeID    = incomingNodesManifest[i].nodeID;
          manifest[NumofNodes].thisNodesManifest[i].curr_Lat  = incomingNodesManifest[i].curr_Lat;
          manifest[NumofNodes].thisNodesManifest[i].curr_Lon  = incomingNodesManifest[i].curr_Lon;
          manifest[NumofNodes].thisNodesManifest[i].curr_Alt  = incomingNodesManifest[i].curr_Alt;
          manifest[NumofNodes].thisNodesManifest[i].curr_Hour = incomingNodesManifest[i].curr_Hour;
          manifest[NumofNodes].thisNodesManifest[i].curr_min  = incomingNodesManifest[i].curr_min;
          manifest[NumofNodes].thisNodesManifest[i].curr_sec  = incomingNodesManifest[i].curr_sec;
      }
      SerialUSB.print("finally, checking what the nodeid is of the last page:");
      SerialUSB.println(manifest[NumofNodes].thisNodesManifest[0].nodeID);

      SerialUSB.print("It should be:");
      SerialUSB.println(incomingNodesManifest[0].nodeID);
      
  }


  
  SerialUSB.println("END reconcile manifest ;;;;;;;;;;;;;");
}
