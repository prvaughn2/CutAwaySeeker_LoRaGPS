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
  uint16_t nodeID;
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
#define NumofReadings 10 //Start with last 10 readings
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


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// C program for array implementation of queue
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
 
// A structure to represent a queue
struct Queue {
    int front, rear, size;
    unsigned capacity;
    information* array;
};
 
// function to create a queue
// of given capacity.
// It initializes size of queue as 0
struct Queue* createQueue(unsigned capacity)
{
    struct Queue* queue = (struct Queue*)malloc(
        sizeof(struct Queue));
    queue->capacity = capacity;
    queue->front = queue->size = 0;
 
    // This is important, see the enqueue
    queue->rear = capacity - 1;
    queue->array = (information*)malloc(
        queue->capacity * sizeof(information));
    return queue;
}
 
// Queue is full when size becomes
// equal to the capacity
int isFull(struct Queue* queue)
{
    return (queue->size == queue->capacity);
}
 
// Queue is empty when size is 0
int isEmpty(struct Queue* queue)
{
    return (queue->size == 0);
}

// Function to remove an item from queue.
// It changes front and size
information dequeue(struct Queue* queue)
{
    if (isEmpty(queue))
    {
        information blankarray;
        return blankarray;
    }
    information item = queue->array[queue->front];
    queue->front = (queue->front + 1)
                   % queue->capacity;
    queue->size = queue->size - 1;
    return item;
}

// Function to add an item to the queue.
// It changes rear and size
void enqueue(struct Queue* queue, information item)
{
    if (isFull(queue)) // MODIFICATION MADE: if we get a reading, but our queue is full, drop the last one. 
        dequeue(queue);
    queue->rear = (queue->rear + 1)
                  % queue->capacity;
    queue->array[queue->rear] = item;
    queue->size = queue->size + 1;
    printf("%d enqueued to queue\n", item);
}
 
// Function to get front of queue
information front(struct Queue* queue)
{
    if (isEmpty(queue))
    {
        information blankarray;
        return blankarray;
    }
    return queue->array[queue->front];
}
 
// Function to get rear of queue
information rear(struct Queue* queue)
{
    if (isEmpty(queue))
    {
        information blankarray;
        return blankarray;
    }
    return queue->array[queue->rear];
}
////////////////////////////////////////////////////////

//The manifest book is an array of queues. Since we start with only our self, we can zero the rest and as we get packets, we can add them to the manifest.

//declare a queue of readings. this is a "tab" or 1 page of manifest. 
struct Queue* aPageofManifest = createQueue(NumofReadings);

//The is the array of queues aka book/manifest
#define MAX_NUM_NODES 100


struct Queue* aBookCalledManifest[MAX_NUM_NODES] = {aPageofManifest};
