/* 
 * HC12 transmitter code for the simpleHC12 library
 * Works together with the receiver code in simpleHC12Receiver.ino
 * 
 * Increases an integer counter and transmits the counter value to another HC12 module (receiver)
 * Uses the checksum functionality of the simpleHC12 library
 * Also prints some transmission statistics
 */
#include "simpleHC12.h"
#include <SoftwareSerial.h>

// Arduino pins to which the HC12 module's read/transmit/set pins are connected
const unsigned int hc12RxPin = 6;
const unsigned int hc12TxPin = 5;
const unsigned int hc12SetPin = 7;

// value to be transmitted
unsigned int counter = 0;

// variables for HC12 module setup
// these values must be the same for receiver and trasmitter
const unsigned long baudRate = 19200;
// counter is a uint value; its max value is therefore 2**16-1=65535, which can be represented by a 5 digit/char integer
// therefore, set messageLength to 5
const unsigned messageLength = 5;
const boolean useChecksum = true;

// variables for calculating transmission statistics
unsigned long sendCounter = 0;
unsigned long tStart = millis();
unsigned long estBaudRate = 0;

// create simpleHC12 object
// baudRate, messageLength, useChecksum, startChar, endChar and checksumDelim must be the same for sender and receiver
simpleHC12 HC12(hc12TxPin, hc12RxPin, hc12SetPin, baudRate, messageLength, useChecksum);

void setup() {
  Serial.begin(115200);
  HC12.safeSetBaudRate();
  
  // change channel; use same for sender and receiver
  HC12.cmd("AT+C050");  
  delay(1000);
  // standard transmission mode; use same for sender and receiver
  HC12.cmd("AT+FU3");  
  delay(1000);
  // high power transmission
  HC12.cmd("AT+P8");
  delay(1000);
}

void loop() {
  // print transmission statistics every 1000 ms
  if (millis()-tStart>=1000) {
    // number of chars sent x 8 bits per char x message length including start/end delimiters, checksum+delimiter (if selected) and \0
    estBaudRate = sendCounter * 8 * (messageLength + 2 + (useChecksum ? 6 : 0) + 1);
    
    Serial.print("Sent data: ");
    Serial.println(counter);
    
    Serial.print("Estimated baudrate: ");
    Serial.println(estBaudRate);
    
    Serial.println("-----------");
    
    // reset counter
    sendCounter = 0;
    
    tStart = millis();
  }
  // only send when HC12 module is ready
  if (HC12.isReadyToSend()) {
    // counter value will be transmitted (not reset)
    counter++;
    // sendCounter is used for calculating transmission statistics (reset every 1000ms)
    sendCounter++;
    HC12.print(counter);
  }
}
