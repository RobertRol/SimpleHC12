#include "Arduino.h"
#include "simpleHC12.h"
#include <util/atomic.h>

// array of all valid baudrates
const long simpleHC12::baudArray[] = {1200,2400,4800,9600,19200,38400,57600,115200};

// wrapper around SoftwareSerial's begin function
void simpleHC12::begin() {
    Serial.print("Manually starting HC-12 module with ");
    Serial.print(baudRate);
    Serial.println(" baudrate");
    iHC12.begin(baudRate);
    isStarted=true;
}

// wrapper around SoftwareSerial's end function
void simpleHC12::end() {
    iHC12.end();
    isStarted=false;
}

// send cmd to HC12 module
boolean simpleHC12::cmd(const char cmd[], boolean printSerial) {
    // clear cmd response buffer first
    clearBuffer(cmdResBuff,cmdResBuffLen);
    
    if (printSerial) {
        Serial.print("Sending Cmd: ");
        Serial.println(cmd);
    }
    
    // put HC12 module into cmd mode
    digitalWrite(setPin,LOW);
    delay(setLowTime);
    
    // send the cmd to the module
    iHC12.print(cmd);
    delay(cmdTime);
    
    // write response into buffer
    size_t i = 0;
    char input;
    
    while (iHC12.available() && i<cmdResBuffLen-1) {
        input = iHC12.read();
        // no interrupts allowed while writing to buffer
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            cmdResBuff[i] = input;
            i++;
        }
    }
    
    if (printSerial) {
        Serial.println(cmdResBuff);
    }
        
    // put HC12 mdoule back into read/write mode
    digitalWrite(setPin,HIGH);
    delay(setHighTime);
    
    // in case of buffer overflow return false
    if (i==cmdResBuffLen-1) {
        return false;
    } else {
        return true;
    }
}

// clears buffers
void simpleHC12::clearBuffer(char* buff, const size_t buffLen) {
    // do not allow interrupts while writing to the buffer
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (size_t i=0;i<buffLen-1;++i) buff[i]=' ';
        // always terminate buffers with \0
        buff[buffLen-1]='\0';
    }
}

// main function for writing
void simpleHC12::printCore(boolean printSerial) {
    // indicate HC12 module is sending data
    isSending=true;
    
    // clear checksum buffer if user selected checksum mode
    if (useChecksum) clearBuffer(checksumBuffer,checksumLen);
    if (printSerial) Serial.print(startChar);
    
    // send char indicating start of message
    iHC12.print(startChar);
    
    // reset checksum var
    checksum=0;
    
    // calculate checksum
    if (useChecksum) {
        // no interrupts while calculating checksum
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            // skip last char = \0
            for (size_t i=0;i<messageLen-1;++i) {
                checksum = checksum + (unsigned int)(sendData[i]);
            }
        }
    }
    
    // send sendData buffer to HC12 module
    // but skip last char, i.e., do not send \0
    // no interrupts while sending -- not sure whether ATOMIC_BLOCK is necessary here
    ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        for (size_t i=0;i<messageLen-1;++i) {
            if (printSerial) Serial.print(sendData[i]);
            iHC12.print(sendData[i]);
        }
    }
    
    if (useChecksum) {
        checksum = ~checksum;
        checksum++;
        // checksum is a uint value; its max value is therefore 2**16-1=65535, which can be represented by a 5 digit integer
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            snprintf(checksumBuffer,checksumLen,"%5u",checksum);
        }
        
        // send delimiter between data and checksum
        iHC12.print(checksumDelim);
        
        // send checksum buffer to HC12 module
        // but skip last char, i.e., do not send \0
        // no interrupts while sending -- not sure whether ATOMIC_BLOCK is necessary here
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            for (size_t j=0;j<checksumLen-1;++j) iHC12.print(checksumBuffer[j]);
        }
        
        if (printSerial) {
            Serial.print(checksumDelim);
            Serial.print(checksumBuffer);
        }
    }
    
    // send char indicating end of message
    iHC12.print(endChar);
    
    if (printSerial) Serial.println(endChar);

    // update flags
    isSending=false;
    finishedSending=true;
    // store millis at the end of the send function
    endSendMillis=millis();
}

// HC12 print method for char arrays
void simpleHC12::print(char data[], boolean printSerial) {
    // clear sendData buffer
    clearBuffer(sendData,messageLen);
    
    // arduino's snprintf does not allow to use printf's asterisk notation for setting fixed widths
    // therefore, we prepare our own format string
    char formatBuffer[6];
    snprintf(formatBuffer,6,"%%%ds",messageLen-1);
    
    // format input to have the correct length
    snprintf(sendData,messageLen,formatBuffer,data);
    // send data to HC12 module
    printCore(printSerial);
}

// HC12 print method for integers
void simpleHC12::print(int data, boolean printSerial) {
    // clear sendData buffer
    clearBuffer(sendData,messageLen);
    
    // arduino's snprintf does not allow to use printf's asterisk notation for setting fixed widths
    // therefore, we prepare our own format string
    char formatBuffer[6];
    snprintf(formatBuffer,6,"%%%dd",messageLen-1);
    
    // format input to have the correct length
    snprintf(sendData,messageLen,formatBuffer,data);
    // send data to HC12 module
    printCore(printSerial);
}

// read data from HC12 module
void simpleHC12::read() {
    size_t i = 0;
    char input;
    
    // clear buffer for received data
    clearBuffer(rcvData,messageLen);
    
    while (iHC12.available()) {
        // call to SoftwareSerial's read function
        input = iHC12.read();
        
        // no interrupts while we store the read data in the rcvData buffer
        // ATOMIC_BLOCK very important here
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
            if (input == endChar) {
                isReadingData = false;
                isReadingChecksum = false;
                finishedReading = true;
                break;
            }
            else if (input == startChar) {
                i=0;
                isReadingData = true;
                isReadingChecksum = false;
            }
            else if (input == checksumDelim & useChecksum) {
                i=0;
                isReadingData = false;
                isReadingChecksum = true;
            }
            else if (isReadingData) {           
                rcvData[i] = input;
                if (i < messageLen - 1) {
                    i++;
                }
                else if (!useChecksum) {
                    isReadingData = false;
                    isReadingChecksum = false;
                    finishedReading = true;
                    break;
                }
            }
            else if (isReadingChecksum) {
                checksumBuffer[i] = input;
                if (i < checksumLen - 1) {
                    i++;
                }
                else {
                    isReadingData = false;
                    isReadingChecksum = false;
                    finishedReading = true;
                    break;
                }
            }
        }
        
	}
}

// return pointer to buffer of received data
char* simpleHC12::getRcvData() {
    return rcvData;
}

// check whether checksum is OK
boolean simpleHC12::checksumOk() {
    if (!useChecksum) return true;
    checksum=0;
    for (size_t i=0;i<messageLen-1;++i) checksum = checksum + (unsigned int)(rcvData[i]);
    unsigned checksumRcvd=atoi(checksumBuffer);
    return checksumRcvd+checksum == 0;
}

// marks that data can be read from the module
void simpleHC12::setReadyToRead() {
    finishedReading=false;
}

// module has finished reading data
boolean simpleHC12::hasFinishedReading() {
    return finishedReading;
}

// module can send data
boolean simpleHC12::isReadyToSend() {
    // module must have stopped sending
    // plus: we have to wait for transferDelay milliseconds
    return !isSending && (millis()-endSendMillis>=transferDelay);
}

// sends cmd "cmdChar" across all baudrates given in baudArray
// returns doStop if any cmd returned "OK.*"; also returns idx of baudArray where the HC12 module returns "OK"
// returns bufferOK=true if no buffer overflow occured in cmd method
loopCmdRes simpleHC12::loopCmd(char cmdChar[]) {
    boolean doStop=false;
    size_t i=0;
    boolean bufferOK;
    
    while (i<baudArrayLen && !doStop) {               
        iHC12.end();
        delay(500);
        iHC12.begin(baudArray[i]);
        delay(500);
        
        bufferOK=cmd(cmdChar);
        if (!bufferOK) break;
        doStop=(cmdResBuff[0]=='O'&&cmdResBuff[1]=='K');
        delay(500);
        if (!doStop) i++;
    }
    
    return loopCmdRes(doStop,bufferOK, i);
}

// detects baudrate setting of the HC12 module
void simpleHC12::baudDetector() {
    Serial.println("***Detecting baudRate***");
    
    // send "AT" cmd
    // will return "OK" if successful
    loopCmdRes tmp=loopCmd("AT");
    
    // buffer overflow
    if (!tmp.bufferOK) {
        Serial.println("- Buffer overflow in baudDetector                     -");
        Serial.println("- This might be due to an interferring sending module -");
        Serial.println("- Turn off and try again                              -");
    } else {
        if (tmp.doStop) {
            Serial.print("Detected baudRate at: ");
            Serial.println(baudArray[tmp.idx]);
            Serial.print("\n");
        } else {
            Serial.println("Could not detect baudRate. \n\rMaybe try to run bruteSetDefault to reset to 9600.");
            Serial.print("\n");
        }
    }
}

// loops through all valid baudrates and sends "AT+DEFAULT" cmd
void simpleHC12::bruteSetDefault() {
    Serial.println("***Resetting to defaults***");
    char baudChar[11];
    
    boolean doStop=false;
    
    for (size_t i=0;i<baudArrayLen;i++) {
        sprintf(baudChar,"AT+B%ld",baudArray[i]);
                
        iHC12.end();
        delay(500);
        iHC12.begin(baudArray[i]);
        delay(500);
        
        // discard boolean loopCmdRes
        cmd("AT+DEFAULT",false);
        delay(500);
    }
    iHC12.end();
}

// convenience function to set baudrate
// also works if the user does not know the current HC12 baudrate setting
void simpleHC12::safeSetBaudRate() {
    Serial.println("***Safe-setting baudRate***");
    char baudChar[11];
    char baudCharSet[11];
    
    sprintf(baudCharSet,"AT+B%ld",baudRate);
    
    loopCmdRes tmp=loopCmd(baudCharSet);
    
    if (!tmp.bufferOK) {
        Serial.println("- Buffer overflow in safeSetBaudRate                  -");
        Serial.println("- This might be due to an interferring sending module -");
        Serial.println("- Turn off and try again                              -");
    } else {
        if (tmp.doStop) {
            Serial.println(cmdResBuff);
        } else {
            Serial.println("Could not set baudRate");
        }
    }
}
