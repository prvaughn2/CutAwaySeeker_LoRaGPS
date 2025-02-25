#include <SPI.h>
#include <RH_RF95.h>
#include <TinyGPS++.h>

// Define the pins for the LoRa module
#define RFM95_CS     12  // Chip select
#define RFM95_RST    9   // Reset pin
#define RFM95_INT    6   // Interrupt pin

// GPS setup
TinyGPSPlus gps;

// Node UID (based on MAC address or serial number)
uint32_t nodeUID;  // Dynamic UID based on device serial number

// Manifest to store locations and timestamps (only last 50 transmissions)
struct Location {
  float lat;
  float lng;
  int hour;
  int minute;
  int second;
};

Location manifest[50];  // Circular buffer for the last 50 transmissions
int nodeCount = 0;  // Track how many transmissions we've recorded

// Create the RF95 object for LoRa communication
RH_RF95 rf95(RFM95_CS, RFM95_INT);

unsigned long previousMillis = 0;  // Store the last time the debug message was displayed
const long debugInterval = 5000;  // Interval for displaying the debug message (5 seconds)
const long listenInterval = 10000; // Interval for listening to nodes (10 seconds)
unsigned long listenStartMillis = 0; // Timer for listening

bool listening = false; // Flag to track whether we are in the listening window

// Function to retrieve the MAC address or serial number for UID
uint32_t getUID() {
  // Use the serial number stored in the SAMD21 device registers for a unique ID
  uint32_t uid = (*(uint32_t*)0x0080A00C); // Device serial number from SAMD21 unique ID
  return uid;
}

void setup() {
  // Start Serial for debugging via SerialUSB
  SerialUSB.begin(9600);
  while (!SerialUSB);  // Wait for USB serial to be ready

  SerialUSB.println("Initializing GPS...");

  // Initialize LoRa module
  if (!rf95.init()) {
    SerialUSB.println("Radio initialization failed!");
    while (1);
  }

  // Set the frequency to 904.6 MHz
  float frequency = 904.6;
  if (!rf95.setFrequency(frequency)) {
    SerialUSB.println("Frequency set failed!");
    while (1);
  }

  // Set the transmission power (optional)
  rf95.setTxPower(13);  // Max power

  SerialUSB.println("LoRa Initialized!");

  // Initialize GPS
  SerialUSB.println("GPS initialized.");

  // Initialize the random number generator with a better seed
  randomSeed(analogRead(A1));  // Use an unused analog pin (e.g., A1)

  // Generate a dynamic UID (32-bit based on device serial number)
  nodeUID = getUID();  // Retrieve the device's unique serial number
  SerialUSB.print("This Node UID: ");
  SerialUSB.println(nodeUID, DEC);  // Display the UID for debugging

  // Initialize Serial1 for GPS communication
  Serial1.begin(9600);  // Ensure that the GPS is using the correct baud rate
}

void loop() {
  // Get the current time
  unsigned long currentMillis = millis();

  // Check if it's time to display the debug message every 5 seconds
  if (currentMillis - previousMillis >= debugInterval) {
    previousMillis = currentMillis;
    SerialUSB.println("Entering loop...");
  }

  // Start listening for nodes for 10 seconds
  if (!listening && currentMillis - listenStartMillis >= listenInterval) {
    // Start a new listening period
    listening = true;
    listenStartMillis = currentMillis;
    SerialUSB.println("Listening for 10 seconds...");
  }

  // If we're in the listening period (10 seconds), listen for incoming messages
  if (listening) {
    // Listen for other nodes' messages
    listenForNodes();

    // Check if we've been listening for 10 seconds
    if (currentMillis - listenStartMillis >= listenInterval) {
      listening = false; // Stop listening after 10 seconds
      SerialUSB.println("Finished listening for nodes.");
    }
  }

  // Read GPS data from Serial1 (not SerialUSB)
  while (Serial1.available() > 0) {
    char incomingByte = Serial1.read();
    //SerialUSB.print("Raw GPS Byte: ");
    //SerialUSB.println(incomingByte, DEC); // Print the raw byte for debugging

    gps.encode(incomingByte);  // Encode the byte in the GPS object
    //if (gps.location.isUpdated()) {
      float latitude = gps.location.lat();
      float longitude = gps.location.lng();
      
      // Get current time from GPS
      int hour = gps.time.hour();
      int minute = gps.time.minute();
      int second = gps.time.second();

      // Display the current GPS info
      SerialUSB.print("Latitude= "); 
      SerialUSB.print(latitude, 6);
      SerialUSB.print(" Longitude= "); 
      SerialUSB.println(longitude, 6);
      SerialUSB.print("Time= "); 
      SerialUSB.print(hour); 
      SerialUSB.print(":");
      SerialUSB.print(minute); 
      SerialUSB.print(":");
      SerialUSB.println(second);

      // Send location with timestamp and UID to other nodes
      sendLocation(latitude, longitude, hour, minute, second);
    //}
  }

  // Listen for other nodes
  listenForNodes();
}

void sendLocation(float lat, float lon, int hour, int minute, int second) {
  // Prepare the message to send
  String message = String("UID:") + nodeUID + String(" LOC:") + lat + String(",") + lon + String(",") + hour + String(":") + minute + String(":") + second;
  
  // Convert the message to a char array
  char msg[message.length() + 1];
  message.toCharArray(msg, message.length() + 1);

  // Send the message over LoRa
  rf95.send((uint8_t *)msg, message.length());
  rf95.waitPacketSent();  // Wait for the packet to be sent
  SerialUSB.println("Location and timestamp sent with UID.");
}

void listenForNodes() {
  // Check if there's a message available
  uint8_t buf[255];
  uint8_t len = sizeof(buf);

  SerialUSB.println("Listening for others.");

  if (rf95.recv(buf, &len)) {
    // Successfully received a message
    String received = String((char*)buf);

    if (received.startsWith("UID:")) {
      uint32_t receivedUID;
      float lat, lon;
      int hour, minute, second;
      sscanf(received.c_str() + 4, "%lu LOC:%f,%f,%d:%d:%d", &receivedUID, &lat, &lon, &hour, &minute, &second);
      
      // Add to manifest (circular buffer for last 50 transmissions)
      addToManifest(receivedUID, lat, lon, hour, minute, second);
    }
  }
  else
    SerialUSB.println("Nothing heard.");
}

void addToManifest(uint32_t uid, float lat, float lon, int hour, int minute, int second) {
  // Add a new entry to the manifest
  if (nodeCount < 50) {
    // Store the transmission in the manifest
    manifest[nodeCount].lat = lat;
    manifest[nodeCount].lng = lon;
    manifest[nodeCount].hour = hour;
    manifest[nodeCount].minute = minute;
    manifest[nodeCount].second = second;
    nodeCount++;
  } else {
    // Circular buffer: shift entries and add the new one
    for (int i = 1; i < 50; i++) {
      manifest[i - 1] = manifest[i];
    }
    manifest[49].lat = lat;
    manifest[49].lng = lon;
    manifest[49].hour = hour;
    manifest[49].minute = minute;
    manifest[49].second = second;
  }

  // Print the added entry for debugging
  SerialUSB.print("Node UID: ");
  SerialUSB.print(uid);
  SerialUSB.print(" Location: ");
  SerialUSB.print(lat, 6);
  SerialUSB.print(", ");
  SerialUSB.print(lon, 6);
  SerialUSB.print(" Time: ");
  SerialUSB.print(hour);
  SerialUSB.print(":");
  SerialUSB.print(minute);
  SerialUSB.print(":");
  SerialUSB.println(second);
}
