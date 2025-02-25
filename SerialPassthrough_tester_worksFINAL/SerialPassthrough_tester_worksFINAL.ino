/*
  Use this to forward data to and from the ucenter applcaiton 
  
  
  SerialPassthrough sketch

  Some boards, like the Arduino 101, the MKR1000, Zero, or the Micro, have one
  hardware serial port attached to Digital pins 0-1, and a separate USB serial
  port attached to the IDE Serial Monitor. This means that the "serial
  passthrough" which is possible with the Arduino UNO (commonly used to interact
  with devices/shields that require configuration via serial AT commands) will
  not work by default.

  This sketch allows you to emulate the serial passthrough behaviour. Any text
  you type in the IDE Serial monitor will be written out to the serial port on
  Digital pins 0 and 1, and vice-versa.

  On the 101, MKR1000, Zero, and Micro, "Serial" refers to the USB Serial port
  attached to the Serial Monitor, and "Serial1" refers to the hardware serial
  port attached to pins 0 and 1. This sketch will emulate Serial passthrough
  using those two Serial ports on the boards mentioned above, but you can change
  these names to connect any two serial ports on a board that has multiple ports.

  created 23 May 2016
  by Erik Nyquist

  https://www.arduino.cc/en/Tutorial/BuiltInExamples/SerialPassthrough
*/
#include <NMEAGPS.h>
NMEAGPS gps;
gps_fix fix;

const unsigned long interval = 1000; // 1 seconds interval
unsigned long previousMillis = 0;


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
}

void loop() {
  // Whatever is coming in on the USB and forward it to RX/TX
  if (SerialUSB.available()) {        // If anything comes in Serial (USB),
    Serial1.write(SerialUSB.read());  // read it and send it out Serial1 (pins 0 & 1)
  }

  // Whatever is coming in on the RX/TX and forward it to USB

  if (Serial1.available()) {       // If anything comes in Serial1 (pins 0 & 1)
    SerialUSB.write(Serial1.read());  // read it and send it out Serial (USB)
  }

  //if (Serial1.available()) {       // If anything comes in Serial1 (pins 0 & 1)
  //  gps.encode(Serial1.read());
  //}
//delay(500);
//SerialUSB.print("TEST!!!!!!!!!!!!!gps.location.lat()="); SerialUSB.println(gps.location.lat());

unsigned long currentMillis = millis();
// Check if 5 seconds have passed
    if (currentMillis - previousMillis >= interval) {
        previousMillis = currentMillis; // Reset the timer
//SerialUSB.print("TEST!!!!HERE___");

    //while (Serial1.available()) {
        //fix = gps.read(Serial1);
        //SerialUSB.print("TEST!!!!fix.valid.status="); SerialUSB.println(fix.valid.status);
    //}
    }


}
