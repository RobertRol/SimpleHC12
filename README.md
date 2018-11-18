# simpleHC12

An Arduino library for reliable data transfer between HC12 RF modules. It provides (optional) checksum functionality as well as a few convenience functions for HC12 module setup.

## Overview
1. [Installation](#installation)
2. [Usage](#usage)
3. [Comments and Restrictions](#comments-and-restrictions)
4. [Public Methods Description](#public-methods-description)

## Installation

Copy `simpleHC12.cpp` and `simpleHC12.h` into a new folder `simpleHC12` in your libraries directory. In my case, the files are located in `/home/robert/sketchbook/libraries/simpleHC12`.

Then include the line
```cpp
#include "simpleHC12.h"
```
in your project.

You must have the `SoftwareSerial` library installed in order to be able to use `simpleHC12`.

## Usage

See files [simpleHC12Sender.ino](https://github.com/RobertRol/SimpleHC12/blob/master/examples/simpleHC12Sender/simpleHC12Sender.ino)
and [simpleHC12Receiver.ino](https://github.com/RobertRol/SimpleHC12/blob/master/examples/simpleHC12Receiver/simpleHC12Receiver.ino)
for an example on how to set up a simple communication between two HC12 modules.

Since the `simpleHC12` library prints information to the serial port, the user has to add
```cpp
Serial.begin(115200); //or any other baudrate
```
to the `setup` routine.

The basic **transmitter** code looks like
```cpp
#include "simpleHC12.h"
#include <SoftwareSerial.h>

// Arduino pins to which the HC12 module's read/transmit/set pins are connected
// change accordingly
const unsigned int hc12RxPin = 4;
const unsigned int hc12TxPin = 3;
const unsigned int hc12SetPin = 2;

// variables for HC12 module setup
// these values must be the same for receiver and transmitter
const unsigned long baudRate = 19200;
const unsigned messageLength = 5;
boolean useChecksum = true;

// set up simpleHC12 object
simpleHC12 HC12(hc12TxPin, hc12RxPin, hc12SetPin, baudRate, messageLength, useChecksum);

void setup() {
  Serial.begin(115200);
  HC12.safeSetBaudRate();
}

void loop() {
  // only send when HC12 module is ready
  // the message length must match messageLength
  if (HC12.isReadyToSend()) HC12.print("12345");
}
```

**Receiver** code
```cpp
// same as transmitter code up to loop function

// holds received data; messageLength + \0
char data[messageLength + 1];

void loop() {
  // read from HC12 module
  HC12.read();
  // process data as soon as HC12 has finished reading
  if (HC12.dataIsReady()) {
    // does the received checksum value match the one calculated from the received data?
    // checksumOk() will always return true if useChecksum is set to false
    if (HC12.checksumOk()) {
      // this is how received data should be copied from the simpleHC12 buffer into other arrays
      strncpy(data, HC12.getRcvData(), messageLength);
      // add \0 to the end of data
      data[messageLength]='\0';
    } else {
      // checksum is not ok
      // handle error
      // ...
    }
    // reset flag
    HC12.setReadyToReceive();
  }
}
```

The serial output of the sender and receiver modules can then be monitored via
```
screen /dev/xyz 115200
```

In my case, `xyz` is `ttyACM0` for the sender and `ttyUSB0` for the receiver.
Don't forget to terminate the screens via the keyboard shortcut `CTRL+A SHIFT+K y` before uploading new code to your Arduinos.

## Comments and Restrictions

Please note that I was not able to get a working connection between my HC12 modules when setting the baudrate above 19200.
This might be due to the dependecy on `SoftwareSerial`, which seems to have problems at baudrates above 19200, see http://forum.arduino.cc/index.php?topic=180844.0.
Possible work-arounds for this are given in https://github.com/SlashDevin/NeoGPS/blob/master/extras/doc/Installing.md#2-choose-a-serial-port.

Using simple checksums to ensure data integrity does not capture all transmission errors. For instance, the messages "12345" and "12354" would produce the same checksum although the last two integers are flipped.

## Public Methods Description

### The Constructor
```cpp
// simpleHC12 constructor
simpleHC12 (const unsigned int txPin,
            const unsigned int rxPin,
            const unsigned int setPin,
            const unsigned long baudRate,
            const size_t messageLen,
            const boolean useChecksum=false,
            const unsigned int transferDelay=0,
            char startChar='<',
            char endChar='>',
            char checksumDelim=',')
```

* `txPin`, `rxPin` and `setPin` are the Arduino pins to which the HC12 module's read/transmit/set pins are connected.
* `baudRate` is the baudrate at which the HC12 modules are communicating (I did not manage to get it working above 19200)
* `messageLen` length of the message to be transmitted. For instance, assume the user wants to transmit `"abcde"`, then the message lenght would be `5` because the message has 5 `chars`.
* `useChecksum` boolean flag to indicate whether the user wants to use the checksum functionality
* `transferDelay` delay between sending messages. See [simpleHC12.h](https://github.com/RobertRol/SimpleHC12/blob/master/simpleHC12.h) and https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf for more details. The user might have to change this value to match the selected HC12 transmission mode. The default value is `0ms`.
* `startChar` and `endChar` characters that indicate the start/end of a data package. Cannot be the same. Also the message itself should not include any of these two.
* `checksumDelim` if the user decides to use the checksum functionality, the checksum integer will be appended to the original message and delimited by this character

### safeSetBaudRate
```cpp
void safeSetBaudRate();
```
Convenience function to set baudrate (as given in constructor). Also works if the user does not know the _current_ HC12 baudrate setting.


### Changing the HC12 module configuration
```cpp
boolean cmd(const char[], boolean printSerial=false);
```

Let's say the user wants to change to high power transmission mode, then he would execute
```cpp
HC12.cmd("AT+P8"); // with HC12 being the simpleHC12 object
```
Setting `printSerial` to `true` will print the response of the HC12 module to the serial port.

A list of configuration options for the HC12 module is given in https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf.


### Print routine
```cpp
// print routine for char arrays
void print(char[], boolean printSerial=false);
// print routine for ints
void print(int, boolean printSerial=false);
```

Transmits a char array or an integer via the HC12 module.
Setting `printSerial` to `true` will also print the transmitted message to the serial port.


### Reading data
```cpp
void read();
```
Reads RF data and writes it to the `rcvData` buffer. The user cannot directly access this buffer but has to use the following accessor routine instead

```cpp
char* getRcvData();
```

The data can then be copied into a user-specified array like that
```cpp
// +1 for \0 termination
char data[messageLength + 1];
strncpy(data, HC12.getRcvData(), messageLength); // with HC12 being the simpleHC12 object
```

### Checksum
```cpp
boolean checksumOk();
```

Compares the received checksum value to the one calculated from the received data.
`checksumOk` will always return `true` if `useChecksum` is set to `false`.

### isReadyToSend
```cpp
boolean isReadyToSend();
```
Indicates when the HC12 module is ready to send a new data package.


### dataIsReady
```cpp
boolean dataIsReady();
```
Indicates when the HC12 module has finished reading a data package.

### setReadyToReceive
```cpp
void setReadyToReceive();
```

Resets the `finishedReading` flag to `false`. Must be used after data is read and processed. See receiver code example above.


### Detect baudrate
```cpp
void baudDetector();
```
Detects baudrate setting of the HC12 module and prints it to the serial port.


### begin and end routines
```cpp
void begin();
void end();
```
These are basically just wrappers around SoftwareSerial's `begin` and `end` methods.
Not needed if user calls `safeSetBaudRate()`.


### Reset module to defaults
```cpp
void bruteSetDefault();
```
Sets HC12 module to default state across all baudrates. Therefore also works if user does not know the current HC12 baudrate setting.
