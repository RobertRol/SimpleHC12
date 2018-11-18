/*
 * HC12 receiver code for the simpleHC12 library
 * Works together with the transmitter code of simpleHC12Sender.ino
 * 
 * Receives and prints an integer value as sent from another HC12 sender
 * Also prints some transmission statistics
 */

#include <simpleHC12.h>
#include <SoftwareSerial.h>

// Arduino pins to which the HC12 module's read/transmit/set pins are connected
const unsigned int hc12RxPin = 4;
const unsigned int hc12TxPin = 3;
const unsigned int hc12SetPin = 2;

// variables for HC12 module setup
const unsigned long baudRate = 19200;
const unsigned messageLength = 16;
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
double estBaudRate = 0;
double okRatio = 0;
unsigned long tStart = millis();
unsigned long deltaTime = 0;

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
  // print stats every 1000 milliseconds
  // for some reason this does not really print statistics every 1000ms but less frequently
  // estBaudRate is also affected
  deltaTime = millis()-tStart;
  if (deltaTime >= 1000) {    
    // sum of OK and error data
    ttlCounter = okCounter + errCounter;
    okRatio = (double)okCounter / (double)ttlCounter;
        
    // number of packages sent x 
    //    10 bits per char (8 bits for char + start/stop bit) x
    //    [ message length including start/end delimiters + 
    //      checksum + delimiter (if selected)]
    estBaudRate = (double)(ttlCounter * 10 * (messageLength + 2 + (useChecksum ? 6 : 0)))/(1e-3 * (double)deltaTime);
    
    // print last received data
    Serial.print("Received data: ");
    Serial.println(data);
    
    Serial.print("OK ratio: ");
    Serial.println(okRatio, 2);
    
    Serial.print("Estimated baudrate: ");
    Serial.println(estBaudRate, 0);
    
    Serial.print("DeltaT: ");
    Serial.println(deltaTime);
    
    Serial.print("OK counter: ");
    Serial.println(okCounter);
    
    Serial.print("TTL counter: ");
    Serial.println(ttlCounter);
    
    Serial.println("-----------");
    
    // reset counters
    okCounter = 0;
    errCounter = 0;
    
    tStart = millis();
  }
  
  // read from HC12 module
  HC12.read();
  // process data as soon as HC12 has finished reading
  if (HC12.dataIsReady()) {
    // this is how received data should be copied from the simpleHC12 buffer into other arrays
    // add \0 to the end of data
    strncpy(data,HC12.getRcvData(),messageLength);
    data[messageLength]='\0';
    // does the received checksum value match the one calculated from the received data?
    // checksumOk() will always return true if useChecksum is set to false
    if (HC12.checksumOk()) {
      // checksum is ok --> increase okCounter
      okCounter++;
      //Serial.println(data);
    } else {
      // checksum is not ok --> increase errCounter
      errCounter++;
      //Serial.println("Err");
    }
    // reset flag
    HC12.setReadyToReceive();
  }
}

