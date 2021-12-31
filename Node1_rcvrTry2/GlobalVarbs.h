

// The broadcast frequency is set to 921.2, but the SADM21 ProRf operates
// anywhere in the range of 902-928MHz in the Americas.
// Europe operates in the frequencies 863-870, center frequency at
// 868MHz.This works but it is unknown how well the radio configures to this frequency:
//float frequency = 864.1;
float frequency = 904.6;

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
  //uint8_t curr_Hour;
  //uint8_t curr_min;
  //uint8_t curr_sec;
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
} manifest[initialNumofNodes];

// We need to provide the RFM95 module's chip select and interrupt pins to the
// rf95 instance below.On the SparkFun ProRF those pins are 12 and 6 respectively.
RH_RF95 rf95(12, 6);

//Status LED on pin 13
int LED = 13; 

//Counts the number of packets sent
int packetCounter = 0; 

//Tracks the time stamp of last packet received
long timeSinceLastPacket = 0; 
