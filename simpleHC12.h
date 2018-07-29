#ifndef simpleHC12_h
#define simpleHC12_h
 
#include "Arduino.h"
#include <SoftwareSerial.h>

// return object for method simpleHC12::loopCmd
struct loopCmdRes {
    boolean doStop;
    boolean bufferOK;
    size_t idx;
    loopCmdRes (boolean doStop, boolean bufferOK, size_t idx): doStop{doStop}, bufferOK{bufferOK}, idx{idx} {};
};

class simpleHC12 {
private:
    // char that indicates start of message
    const char startChar;
    // char that indicates end of message (including checksum, if present)
    const char endChar;
    // delimiter char between message and checksum
    const char checksumDelim;
    
    // calculated checksum of message
    unsigned int checksum;
    // indicates whether to add checksum or send plain message
    const boolean useChecksum;
    
    // inidicates whether software serial communication to HC12 module was started
    boolean isStarted;
    // module is reading message
    boolean isReadingData;
    // module is reading checksum (if present)
    boolean isReadingChecksum;
    // module finished reading message
    boolean finishedReading;
    // module finihsed sending message
    boolean finishedSending;
    // module currently sending message
    boolean isSending;
    
    // time counter that is updated after a message was sent
    unsigned long endSendMillis;
    // minumum time between sending messages
    const unsigned int transferDelay;
    
    // buffer for data to be sent
    char* sendData;
    // buffer for received data
    char* rcvData;
    // buffer for calculated checksum
    char* checksumBuffer;
    // buffer for response of cmds
    char* cmdResBuff;
    // holds all valid baudRates
    static const long baudArray[];
    
    // length of message
    const size_t messageLen;
    // length of checksum
    const size_t checksumLen;
    // length of cmdResBuff
    const size_t cmdResBuffLen;
    // length of baudRate array
    const size_t baudArrayLen;
    
    // sends sendData buffer via HC12 module
    void printCore(boolean);
    
    // HC12 specific data
    // baudrate to be used for communication
    const unsigned long baudRate;
    // pin used to switch between cmd and send/read mode of HC12 module
    const unsigned int setPin;
    // after setting the set pin to LOW, wait for setLowTime
    const unsigned int setLowTime;
    // after setting the set pin to HIGH, wait for setHighTime
    const unsigned int setHighTime;
    // after sending a cmd to the HC12 module, wait for cmdTime
    const unsigned int cmdTime;
    
    // a SoftwareSerial object for communication with the HC12 module
    SoftwareSerial iHC12;
public:
    // simpleHC12 constructor
    simpleHC12 (const unsigned int txPin,
            const unsigned int rxPin,
            const unsigned int setPin,
            const unsigned long baudRate,
            const size_t messageLen,
            const boolean useChecksum=false,
            const unsigned int transferDelay=9,
            char startChar='<',
            char endChar='>',
            char checksumDelim=','
           ):
           startChar{startChar},
           endChar{endChar},
           checksumDelim{checksumDelim},
           
           checksum{0},
           useChecksum{useChecksum},
           
           isStarted{false},
           isReadingData{false},
           isReadingChecksum{false},
           finishedReading{false},
           finishedSending{false},
           isSending{false},
           
           endSendMillis(0),
           // the default value for transferDelay is 9ms; I have found this value via trial-and-error
           // the necessity for for this delay is indicated in https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf
           // the optimal value for transferDelay may be different across different HC12 modules
           transferDelay(transferDelay),
           
           // allocate messageLen chars + 1 for \0
           messageLen{messageLen+1},
           // checksum is a uint value; its max value is therefore 2**16-1=65535, which can be represented by a 5 digit integer
           // note: add one char for \0 --> 6 chars in total
           checksumLen{useChecksum ? 6 : 0},
           // 20 chars should be enough for most HC12 cmds; might not work for "AT+RX"
           cmdResBuffLen{20},
           baudArrayLen{8},
           
           baudRate{baudRate},
           setPin{setPin},
           // times in milliseconds; taken from v2.4 documentation
           // https://statics3.seeedstudio.com/assets/file/bazaar/product/HC-12_english_datasheets.pdf
           setLowTime{50},
           setHighTime{90},
           cmdTime{100},
           
           // SoftwareSerial constructor for iHC12 
           iHC12(txPin, rxPin) {
               // rest of simpleHC12 constructor
               
               // setPin must be set to output
               pinMode(setPin, OUTPUT);
               
               // allocate arrays and clear data
               sendData=new char[messageLen];
               clearBuffer(sendData,messageLen);
               
               if (useChecksum) {
                   checksumBuffer=new char[checksumLen];
                   clearBuffer(checksumBuffer,checksumLen);
               }
               rcvData=new char[messageLen];
               clearBuffer(rcvData,messageLen);
               
               cmdResBuff=new char[cmdResBuffLen];
               clearBuffer(cmdResBuff,cmdResBuffLen);
    };
        
    //destructor
    ~simpleHC12() {
        if (isStarted) iHC12.end();
        isStarted=false;
        if (useChecksum) delete[] checksumBuffer;
        delete[] sendData;
        delete[] rcvData;
        delete[] cmdResBuff;
    };
    
    // wrappers around SoftwareSerial's begin and end methods
    void begin();
    void end();
    
    // clears buffer
    void clearBuffer(char*, const size_t);
    
    // sends cmd to HC12 module
    boolean cmd(const char[], boolean printSerial=false);
    
    // print routine for char arrays
    void print(char[], boolean printSerial=false);
    // print routine for ints
    void print(int, boolean printSerial=false);
    
    // reads data from HC12 module
    void read();
    // returns pointer to rcvData buffer
    char* getRcvData();
    
    // checks whether checksum of message is OK
    boolean checksumOk();
    // module can send data
    boolean isReadyToSend();
    // module has finished reading data
    boolean hasFinishedReading();
    
    // marks that data can be read from the module
    void setReadyToRead();
    
    // send cmds across all baudrates given in baudArray
    loopCmdRes loopCmd(char cmdChar[]);
    
    // detects baudrate setting of the HC12 module
    void baudDetector();
    
    // set HC12 to default state across all baudrates
    void bruteSetDefault();
    
    // convenience function to set baudRate
    void safeSetBaudRate();
};
 
#endif
