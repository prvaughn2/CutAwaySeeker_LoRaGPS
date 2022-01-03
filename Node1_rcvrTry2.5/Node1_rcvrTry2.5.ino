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



///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*
  Both the TX and RX ProRF boards will need a wire antenna. We recommend a 3" piece of wire.
  This example is a modified version of the example provided by the Radio Head
  Library which can be found here:
  www.github.com/PaulStoffregen/RadioHeadd
*/

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

  SerialUSB.begin(9600);
  // It may be difficult to read serial messages on startup. The following line
  // will wait for serial to be ready before continuing. Comment out if not needed.
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
  //////////////////////////////////////////////////////////////////////////////////////Step 1/////////////////////////////////////////////////////////////////////////////////////////////////
delay(2000);


  SerialUSB.println("STEP 0: Test output");
  if (SerialUSB.available()) {          // If anything comes in Serial (USB),
    Serial1.write(SerialUSB.read());     // read it and send it out Serial1 (pins 0 & 1)
  }

//supressing for debugging. This is where the LAT/LON is coming in.  
  if (Serial1.available()) // If anything comes in Serial1 (pins 0 & 1)
  {
    ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 1 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
    //Get my Position: process the NMEA input
    SerialUSB.println("STEP 1: Reading and recording GPS data...........................................................................! ");
    gps.encode(Serial1.read());

    

    curr_info.readingID = readingCounter;
    curr_info.nodeID    = curr_ID;
    curr_info.curr_Lat  = gps.location.lat() + DebugOffset; //+10 for debug
    curr_info.curr_Lon  = gps.location.lng();
    curr_info.curr_Alt  = gps.altitude.meters();
    //curr_info.curr_Hour = gps.time.hour();
    //curr_info.curr_min  = gps.time.minute();
    //curr_info.curr_sec  = gps.time.second();

    enqueue(aBookCalledManifest[0],curr_info);

    SerialUSB.print("TEST!!!!!!!!!!!!!Last LAT="); SerialUSB.println(front(aBookCalledManifest[0]).curr_Lat);

    /*
    //Update my manifest: Add the above information to the current Node's manifest
    manifest[0].thisNodesManifest[posofManifestDataWriter].readingID = readingCounter;
    manifest[0].thisNodesManifest[posofManifestDataWriter].nodeID    = curr_ID ;
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Lat  = gps.location.lat() + 10; //+10 for debug
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Lon  = gps.location.lng();
    manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Alt  = gps.altitude.meters();
    //manifest[0].thisNodesManifest[posofManifestDataWriter].curr_Hour = gps.time.hour();
    //manifest[0].thisNodesManifest[posofManifestDataWriter].curr_min  = gps.time.minute();
    //manifest[0].thisNodesManifest[posofManifestDataWriter].curr_sec  = gps.time.second();

    SerialUSB.print("TEST!!!!!!!!!!!!!LAT=");  SerialUSB.println(thisNodesManifest[posofManifestDataWriter].curr_Lat);
    SerialUSB.print("TEST!!!!!!!!!!!!!inside the manifest LAT=");  SerialUSB.println(manifest[0].thisNodesManifest[0].curr_Lat);
     */
    //Update counters

    //THESE WLL BE EDFUNCT AS SOON AS I FINISH THE QUEUE
    readingCounter = readingCounter + 1;
    posofManifestDataWriter = posofManifestDataWriter + 1;

    //If manifest is full, reset the counter to the top to delete the oldest reading NOTE: this list is now not sorted....
    if (posofManifestDataWriter > NumofReadings) {
      posofManifestDataWriter = 0;
    }
  }
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 2 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  // START ACTIVITY 2

  SerialUSB.println("Begin Step2: Send currentinfo.............. ");

  // Send the manifest. This needs to be done "page by page" as the packet can become large.
  //This is still in DEV so I am only sending 1 page  to start

  SerialUSB.println("Sending -curr_info- to anyone who will listen");
  // convert the page struct to byte array
  uint8_t buffer2[sizeof(curr_info)];

  memcpy(buffer2, &curr_info, sizeof(curr_info));

  if (sizeof(buffer2) < 1)
  {
    SerialUSB.println("buffer is empty...:(");
  }
  else
    SerialUSB.print("the sending buffer's size:"); SerialUSB.println(sizeof(buffer2));


  //pass buffer to radio
  rf95.send(buffer2, sizeof(buffer2));

  rf95.waitPacketSent();
  SerialUSB.println("Sent my manifest out successfully. ");
  digitalWrite(LED, LOW); //Turn off status LED

  //END ACTIVITY 2?
  /////////////////////////////////////////////////////////////////////////////////////////End Step 2//////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 3 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //START ACTIVITY 3 - rec. others manifest and update mine.

  SerialUSB.println("Begining Step 3: listen for an incoming packet for 2 seconds");
  //wait for packet to comlpete
  rf95.waitPacketSent();

  // Now wait for a reply
  uint8_t buf[sizeof(curr_info)];
  byte len = sizeof(buf);

  if (rf95.waitAvailableTimeout(50000))//This will change to whatever I want the timeout interval to be. TODO
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
      print1PageofManifest(incoming_curr_info);

      SerialUSB.println("End of Sanity Check #2................................");

      //reconcile my manifest with the one that I just recieved.
      //TODO Sanity checks on the incoming packet i.e. size, node id is a POS num etc. 
      addIncomingCurrInfoToMyBookCalledManifest(incoming_curr_info);
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
}


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/*




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

  //END ACTIVITY 1


  

  //START ACTIVITY 4------------FUTURE DEVELOPMENT
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Step 4 /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
  //END ACTIVITY 4

  //Start sanity check
  SerialUSB.println("Sanity Check3- Entire manifest");
  printEntireManifest(manifest);
  SerialUSB.println("End of Sanity Check3..................END OF PROGRAM.......................................................................................");
  //end sanity check
}


*/
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////// Functions /////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void addIncomingCurrInfoToMyBookCalledManifest(information someNodesCurrInfo)
{
  SerialUSB.println("Adding incming nodes curr info to my Book called manifest");

  //First check to see if I already have a page for the incoming node's info
  bool foundPageInMyManifest = false;
  int counter = 0, IndexofNextQueue = 1;
  for ( ; counter < NumofReadings; counter = counter + 1 )
  //while(counter < MAX_NUM_NODES)
  {
    //NOTE - doing anything in this function produceas a warning to change the starty of the .bss files to ofset by 4.... not sure whuy
    ////////////////////////////////////////////////////////////THIS IS THE PROBLEM!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

        SerialUSB.print("seeing if the array entry has a queue. size of the queue im checking: "); 
    SerialUSB.print(aBookCalledManifest[counter]->size); SerialUSB.println("");
    //SerialUSB.print((aBookCalledManifest[counter+1])->size); SerialUSB.println(".");

    if ((aBookCalledManifest[counter])->size == 0) // this checks if the next queue has a 
    {
       SerialUSB.print("the array I am now checking is empty, I will not check nothing further ");
        IndexofNextQueue = counter;
        counter = NumofReadings - 1;
    }

    else
    {
    
    SerialUSB.print("ID check. Checking the incoming ID of:"); 
    SerialUSB.print(someNodesCurrInfo.nodeID); SerialUSB.println(".");
    SerialUSB.print("Against the stored ID of:");
    SerialUSB.print(front(aBookCalledManifest[counter]).nodeID);SerialUSB.println(".");

    SerialUSB.println("Actually doing the check now:");

    if (someNodesCurrInfo.nodeID == front(aBookCalledManifest[counter]).nodeID)
    {
      SerialUSB.println("Adding incoming nodes curr info my Manifest as an exsisting page");
      foundPageInMyManifest = true;
      enqueue(aBookCalledManifest[counter],someNodesCurrInfo); // is this right?
      
      SerialUSB.println("Incoming Nodes info successfully enqueued to exsisting page!");
      return;// is this right?
    }
    }
  }

  // If we are here, that means this is a new node's data
  if (foundPageInMyManifest == false) // this is now a redundanty check... leave in for now
  {
    SerialUSB.print("The incoming page's ID is not in my manifest. Adding a new page to my manifest. Sanity checking ID:");
    SerialUSB.println(someNodesCurrInfo.nodeID);

    SerialUSB.print("IndexofNextQueue:");
    SerialUSB.println(IndexofNextQueue);
    

    numDiscoveredNodes = numDiscoveredNodes + 1;

    enqueue(aBookCalledManifest[IndexofNextQueue],someNodesCurrInfo); // is this right? 
    SerialUSB.println("Incoming Nodes info successfully enqueued to new page!");
   
  }

  return;
}

/*void addIncomingCurrInfoToMyManifest(information someNodesCurrInfo)
{
  SerialUSB.print("Adding incming nodes curr info to my manifest");


  // We just got a page of a manifest from an outside node. We need to combine/update this new information with our manifest.

  //First check to see if I already have a page for the incoming node's info

  bool foundPageInMyManifest = false;

  for (int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {

    SerialUSB.print("check="); SerialUSB.println(someNodesCurrInfo.nodeID);
    SerialUSB.print("againstmanifest="); SerialUSB.println(manifest[counter].thisNodesManifest[0].nodeID);
    
    if (someNodesCurrInfo.nodeID == manifest[counter].thisNodesManifest[0].nodeID)
    {
      SerialUSB.println("Found the page associated with the incoming pages ID.");
      foundPageInMyManifest = true;
      // Found the page. Update my manifest with the new information.

      // go line by line of my manifest and see if there is new information (somewhat bubble sort like?)
      int incomingRowCounter = 0;
      int myManifestRowCounter = 0;
      for (int readingsCounter = 0 ; readingsCounter < NumofNodes ; readingsCounter = readingsCounter + 1) //better, but not great code
      {
        //We already have an entry for this node.


        
        //if the incoming nodes reading ID is greater than ours (aka that reading is newer than ours) overwrite that reading in my book) // THIS WILL ALWAYS HAPPEN////
        if (someNodesCurrInfo.readingID > manifest[counter].thisNodesManifest[myManifestRowCounter].readingID )
        {
          //take that reading over mine

          manifest[counter].thisNodesManifest[myManifestRowCounter].readingID = someNodesCurrInfo.readingID;
          manifest[counter].thisNodesManifest[myManifestRowCounter].nodeID    = someNodesCurrInfo.nodeID;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lat  = someNodesCurrInfo.curr_Lat;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lon  = someNodesCurrInfo.curr_Lon;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Alt  = someNodesCurrInfo.curr_Alt;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Hour = someNodesCurrInfo.curr_Hour;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_min  = someNodesCurrInfo.curr_min;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_sec  = someNodesCurrInfo.curr_sec;

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

    for (int i = 0; i < (NumofNodes - 1) ; i = i + 1)
    {
      BiggerManifest[i] = manifest[i];
    }
    free(manifest); //Not sure if this will work.........
    //manifestType manifest[NumofNodes];

    for (int i = 0; i < (NumofNodes - 1) ; i = i + 1)
    {
      manifest[i] = BiggerManifest[i];
    }

    //Add the last page to the end

    for (int i = 0; i < NumofReadings ; i = i + 1)
    {
      manifest[NumofNodes].thisNodesManifest[i].readingID = someNodesCurrInfo.readingID;
      manifest[NumofNodes].thisNodesManifest[i].nodeID    = someNodesCurrInfo.nodeID;
      manifest[NumofNodes].thisNodesManifest[i].curr_Lat  = someNodesCurrInfo.curr_Lat;
      manifest[NumofNodes].thisNodesManifest[i].curr_Lon  = someNodesCurrInfo.curr_Lon;
      manifest[NumofNodes].thisNodesManifest[i].curr_Alt  = someNodesCurrInfo.curr_Alt;
      //manifest[NumofNodes].thisNodesManifest[i].curr_Hour = someNodesCurrInfo.curr_Hour;
      //manifest[NumofNodes].thisNodesManifest[i].curr_min  = someNodesCurrInfo.curr_min;
      //manifest[NumofNodes].thisNodesManifest[i].curr_sec  = someNodesCurrInfo.curr_sec;
    }
    SerialUSB.print("finally, checking what the nodeid is of the last page:");
    SerialUSB.println(manifest[NumofNodes].thisNodesManifest[0].nodeID);

    SerialUSB.print("It should be:");
    SerialUSB.println(someNodesCurrInfo.nodeID);

  }



  SerialUSB.println("END addingtomymanifest ;;;;;;;;;;;;;");
  ///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


}
*/

String converter(uint8_t *str) {
  return String((char *)str);
}
void printEntireManifest(manifestType Manifest[]) {
  for (int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {
    SerialUSB.print("Manifest of Node "); SerialUSB.println(counter);

    //print1PageofManifest(Manifest[counter].thisNodesManifest);
    SerialUSB.println("TODO PRINT ENTIRE MANIFEST");

  }

}
void print1PageofManifest(information aNodesManifest) { // print the short version for debugging

  //for (int row = 0; row < NumofReadings ; row = row + 1)
  {
    //SerialUSB.print("Manifest Page # "); SerialUSB.println(row);
    SerialUSB.print("ID=");  SerialUSB.println(aNodesManifest.nodeID);
    SerialUSB.print("LAT=");  SerialUSB.println(aNodesManifest.curr_Lat, 6);
    SerialUSB.print("LONG="); SerialUSB.println(aNodesManifest.curr_Lon, 6);
  }
}
/*void ReconcileManifest(information incomingNodesManifest[])
{

  // We just got a page of a manifest from an outside node. We need to combine/update this new information with ours.

  //First check to see if I already have a page for the incoming node's info

  bool foundPageInMyManifest = false;

  for (int counter = 0; counter < NumofNodes; counter = counter + 1 )
  {

    SerialUSB.print("check="); SerialUSB.println(incomingNodesManifest[0].nodeID);
    SerialUSB.print("againstmanifest="); SerialUSB.println(manifest[counter].thisNodesManifest[0].nodeID);
    if (incomingNodesManifest[0].nodeID == manifest[counter].thisNodesManifest[0].nodeID)
    {
      SerialUSB.println("Found the page associated with the incoming pages ID.");
      foundPageInMyManifest = true;
      // Found the page. Update my manifest with the new information.

      // go line by line of my manifest and see if there is new information (somewhat bubble sort like?)
      int incomingRowCounter = 0;
      int myManifestRowCounter = 0;
      for (int readingsCounter = 0 ; readingsCounter < NumofNodes ; readingsCounter = readingsCounter + 1) //better, but not great code
      {
        //if the incoming nodes reading ID is greater than ours (aka that reading is newer than ours) overwrite that reading in my book)
        if (incomingNodesManifest[incomingRowCounter].readingID > manifest[counter].thisNodesManifest[myManifestRowCounter].readingID )
        {
          //take that reading over mine

          manifest[counter].thisNodesManifest[myManifestRowCounter].readingID = incomingNodesManifest[incomingRowCounter].readingID;
          manifest[counter].thisNodesManifest[myManifestRowCounter].nodeID    = incomingNodesManifest[incomingRowCounter].nodeID;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lat  = incomingNodesManifest[incomingRowCounter].curr_Lat;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Lon  = incomingNodesManifest[incomingRowCounter].curr_Lon;
          manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Alt  = incomingNodesManifest[incomingRowCounter].curr_Alt;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_Hour = incomingNodesManifest[incomingRowCounter].curr_Hour;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_min  = incomingNodesManifest[incomingRowCounter].curr_min;
          //manifest[counter].thisNodesManifest[myManifestRowCounter].curr_sec  = incomingNodesManifest[incomingRowCounter].curr_sec;

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

    for (int i = 0; i < (NumofNodes - 1) ; i = i + 1)
    {
      BiggerManifest[i] = manifest[i];
    }
    free(manifest); //Not sure if this will work.........
    //manifestType manifest[NumofNodes];

    for (int i = 0; i < (NumofNodes - 1) ; i = i + 1)
    {
      manifest[i] = BiggerManifest[i];
    }

    //Add the last page to the end

    for (int i = 0; i < NumofReadings ; i = i + 1)
    {
      manifest[NumofNodes].thisNodesManifest[i].readingID = incomingNodesManifest[i].readingID;
      manifest[NumofNodes].thisNodesManifest[i].nodeID    = incomingNodesManifest[i].nodeID;
      manifest[NumofNodes].thisNodesManifest[i].curr_Lat  = incomingNodesManifest[i].curr_Lat;
      manifest[NumofNodes].thisNodesManifest[i].curr_Lon  = incomingNodesManifest[i].curr_Lon;
      manifest[NumofNodes].thisNodesManifest[i].curr_Alt  = incomingNodesManifest[i].curr_Alt;
      //manifest[NumofNodes].thisNodesManifest[i].curr_Hour = incomingNodesManifest[i].curr_Hour;
      //manifest[NumofNodes].thisNodesManifest[i].curr_min  = incomingNodesManifest[i].curr_min;
      //manifest[NumofNodes].thisNodesManifest[i].curr_sec  = incomingNodesManifest[i].curr_sec;
    }
    SerialUSB.print("finally, checking what the nodeid is of the last page:");
    SerialUSB.println(manifest[NumofNodes].thisNodesManifest[0].nodeID);

    SerialUSB.print("It should be:");
    SerialUSB.println(incomingNodesManifest[0].nodeID);

  }



  SerialUSB.println("END reconcile manifest ;;;;;;;;;;;;;");
}
*/
