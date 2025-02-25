#include <SPI.h>
#include <RH_RF95.h> //Radio Head Library
#include "TinyGPS++.h" //NMEA Parsing
#include "Battery.h" // Battery info
#include "GlobalVarbs.h" // Config file for this node
#include "ThisNodesConfig.h" // node specific varbs
//#include "Functions.h" // Global Functions for this program

//Globals
TinyGPSPlus gps; //GPS info
int numDiscoveredNodes;


void setup()
{

  for(int i=0 ; i < MAX_NUM_NODES ; i = i +1)
{
  //declare a queue of readings. this is a "tab" or 1 page of manifest. 
  //struct Queue* aPageofManifest = createQueue(NumofReadings);

  aBookCalledManifest[i] = createQueue(NumofReadings);
}


  numDiscoveredNodes = 1; 
  pinMode(LED, OUTPUT);

  Serial.begin(9600);
  // It may be difficult to read serial messages on startup. The following line
  // will wait for serial to be ready before continuing. Comment out if not needed.
  //while(!Serial); 
  Serial.println("RFM Client!"); 

  //Initialize the Radio.
  if (rf95.init() == false){
    Serial.println("Radio Init Failed - Freezing");
    while (1);
  }
  else{
    //An LED inidicator to let us know radio initialization has completed. 
    Serial.println("Transmitter up!"); 
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

  Serial.println("STEP 0: Test output");
  if (Serial.available()) {          // If anything comes in Serial (USB),
    Serial1.write(Serial.read());     // read it and send it out Serial1 (pins 0 & 1)
  }

  //START ACTIVITY 3 - rec. others manifest and update mine.

  Serial.println("Begining Step 3: listen for an incoming packet for 2 seconds");
  //wait for packet to comlpete
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[sizeof(curr_info)];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(5000))//This will change to whatever I want the timeout interval to be. TODO
  {
    // Should be a reply message for us now
    if (rf95.recv(buf, &len)) {
      Serial.print("Got a reply of size: ");
      Serial.println(sizeof(buf));

      Serial.print("buffer should be of curr_info. This is the actual packet: ");
      Serial.println(converter(buf));

      //currently this is 1 page of manifest. this will be bigger later? or will i only ever recieve 1 page at a time?
      information incomingNodesManifest[NumofReadings];

      information incomingNodesCurrInfo;
      //manifestType recievedManifest[NumofNodes]; // I should have recieved a manifest of the 2 nodes.

      memcpy(&incoming_curr_info, &buf, sizeof(buf));


      //SANITY CHECK #2
      Serial.println("Sanity Check #2................................");
      Serial.print("incoming Nodes curr info LAT------: "); Serial.println(incoming_curr_info.curr_Lat);
      print1PageofManifest(incoming_curr_info);

      Serial.println("End of Sanity Check #2................................");

      //reconcile my manifest with the one that I just recieved.
      //TODO Sanity checks on the incoming packet i.e. size, node id is a POS num etc. 
      //addIncomingCurrInfoToMyBookCalledManifest(incoming_curr_info);
    }
    else
    {
      Serial.println("Receive failed");
    }
  }
  else
  {
    Serial.println("Didn't get any packets. Is anyone sending?");
  }



  //END ACTIVITY 3
}

String converter(uint8_t *str) {
  return String((char *)str);
}

void print1PageofManifest(information aNodesManifest) { // print the short version for debugging

  //for (int row = 0; row < NumofReadings ; row = row + 1)
  {
    //Serial.print("Manifest Page # "); Serial.println(row);
    Serial.print("ID=");  Serial.println(aNodesManifest.nodeID);
    Serial.print("LAT=");  Serial.println(aNodesManifest.curr_Lat, 6);
    Serial.print("LONG="); Serial.println(aNodesManifest.curr_Lon, 6);
  }
}

