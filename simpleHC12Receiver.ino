/*
 * HC12 receiver code for the simpleHC12 library
 * Works together with the transmitter code of simpleHC12Sender.ino
 * 
 * Receives and prints an integer value as sent from another HC12 sender
 * Also prints some transmission statistics
 */

#include "simpleHC12.h"
#include <SoftwareSerial.h>

// Arduino pins to which the HC12 module's read/transmit/set pins are connected
const unsigned int hc12RxPin = 4;
const unsigned int hc12TxPin = 3;
const unsigned int hc12SetPin = 2;

// variables for HC12 module setup
const unsigned long baudRate = 19200;
// the transmitted value is an unsigned integer; its max value is therefore 2**16-1=65535, which can be represented by a 5 digit/char integer
// therefore, set messageLength to 5
const unsigned messageLength = 5;
boolean useChecksum = true;

// create simpleHC12 object
// baudRate, messageLength, useChecksum, startChar, endChar and checksumDelim must be the same for sender and receiver
simpleHC12 HC12(hc12TxPin, hc12RxPin, hc12SetPin, baudRate, messageLength, useChecksum);

// holds received data; messageLength + \0
char data[messageLength + 1];

// variables for calculating transmission statistics
unsigned long okCounter = 0;
unsigned long errCounter = 0;
unsigned long ttlCounter = 0;
unsigned long estBaudRate = 0;
double okRatio = 0;
unsigned long tStart = millis();

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
}

void loop() {
  // print stats every 100 milliseconds
  if (millis()-tStart >= 1000) {
    // sum of OK and error data
    ttlCounter = okCounter + errCounter;
    okRatio = (double)okCounter / (double)ttlCounter;
    
    // print last received data
    Serial.print("Received data: ");
    Serial.println(data);
    
    Serial.print("OK ratio: ");
    Serial.println(okRatio, 2);
        
    // number of chars sent x 8 bits per char x message length including start/end delimiters, checksum+delimiter (if selected) and \0
    estBaudRate = ttlCounter * 8 * (messageLength + 2 + (useChecksum ? 6 : 0) + 1);
    Serial.print("Estimated baudrate: ");
    Serial.println(estBaudRate);
    
    Serial.print("OK counter: ");
    Serial.println(okCounter);
    
    Serial.println("-----------");
    
    // reset counters
    okCounter = 0;
    errCounter = 0;
    
    tStart = millis();
  }
  
  // read from HC12 module
  HC12.read();
  // process data as soon as HC12 has finished reading
  if (HC12.hasFinishedReading()) {
    // does the received checksum value match the one calculated from the received data?
    // checksumOk() will always return true if useChecksum is set to false
    if (HC12.checksumOk()) {
      // checksum is ok --> increase okCounter
      okCounter++;
      // this is how received data should be copied from the simpleHC12 buffer into other arrays
      strncpy(data,HC12.getRcvData(),messageLength);
      // add \0 to the end of data
      data[messageLength]='\0';
    } else {
      // checksum is not ok --> increase errCounter
      errCounter++;
    }
    // reset flag
    HC12.resetFinishedReading();
  }
}

