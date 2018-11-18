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
    // module finihsed sending message
    boolean finishedSending;
    // module currently sending message
    boolean isSending;
    // module is ready to receive
    boolean readyToReceive;
    // module has finished reading
    boolean finishedReading;
    
    // iterator variables for message and checksum
    size_t messageIter;
    size_t checksumIter;
    
    // time counter that is updated after a message was sent
    unsigned long endSendMillis;
    // minumum time between sending messages
    unsigned int transferDelay;
    
    // buffer for message
    char* message;
    // buffer for data to be sent (including start/endChar, checksum and delimiter)
    char* sendData;
    // buffer for received data
    char* rcvData;
    // buffer for calculated checksum
    char* checksumBuffer;
    // buffer for response of cmds
    char* cmdResBuff;
    char* allData;
    // holds all valid baudRates
    static const long baudArray[];
    
    // arduino's snprintf does not allow to use printf's asterisk notation for setting fixed widths
    // therefore, we prepare our own format string
    char stringFormatBuffer[6];
    char intFormatBuffer[6];
    char uintFormatBuffer[6];
    
    // length of message
    const size_t messageLen;
    // length of data to be sent
    const size_t sendDataLen;
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
            const unsigned int transferDelay=0,
            char startChar='<',
            char endChar='>',
            char checksumDelim=','
           ):
           startChar{startChar},
           endChar{endChar},
           checksumDelim{checksumDelim},
           
           // iterator for received message buffer
           messageIter{0},
           // iterator for received checksum
           checksumIter{0},
           
           checksum{0},
           useChecksum{useChecksum},
           
           isStarted{false},
           isReadingData{false},
           isReadingChecksum{false},
           finishedSending{false},
           isSending{false},
           readyToReceive{true},
           finishedReading{false},
           
           endSendMillis{0},
           // the documentation is hard to read due to the bad English but ...
           // ... a delay between sending packages might be necessary for some transmission modes
           // it's 0 by default
           transferDelay{transferDelay},
           
           // allocate messageLen chars + 2 for start/endChar + 5 chars for checksum (if selected) + 1 for checksum delimiter + 1 for \0
           sendDataLen{messageLen + 2 + (useChecksum ? 6 : 0) + 1},
           // allocate messageLen chars + 1 for \0
           messageLen{messageLen + 1 },
           // checksum is a uint value; its max value is therefore 2**16-1=65535, which can be represented by a 5 digit integer
           // note: add one char for \0 --> 6 chars in total
           // this also means that the maximal message length is 255 when using the checksum functionality
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
               sendData=new char[sendDataLen];
               clearBuffer(sendData,sendDataLen);
               
               message=new char[messageLen];
               clearBuffer(message,messageLen);
               
               if (useChecksum) {
                   checksumBuffer=new char[checksumLen];
                   clearBuffer(checksumBuffer,checksumLen);
               }
               rcvData=new char[messageLen];
               clearBuffer(rcvData,messageLen);
               
               cmdResBuff=new char[cmdResBuffLen];
               clearBuffer(cmdResBuff,cmdResBuffLen);
               
               snprintf(stringFormatBuffer,6,"%%%ds",messageLen);
               snprintf(intFormatBuffer,6,"%%%dd",messageLen);
               snprintf(uintFormatBuffer,6,"%%%du",messageLen);
    };
        
    //destructor
    ~simpleHC12() {
        if (isStarted) iHC12.end();
        isStarted=false;
        if (useChecksum) delete[] checksumBuffer;
        delete[] message;
        delete[] sendData;
        delete[] rcvData;
        delete[] cmdResBuff;
    };
    
    // wrappers around SoftwareSerial's begin and end methods
    void begin();
    void end();
        
    // sends cmd to HC12 module
    boolean cmd(const char[], boolean printSerial=false);
    
    // print routine for char arrays
    void print(char[], boolean printSerial=false);
    // print routine for ints
    void print(int, boolean printSerial=false);
    // print routine for uints
    void print(unsigned int, boolean printSerial=false);
    
    // reads data from HC12 module
    void read();
    // returns pointer to rcvData buffer
    char* getRcvData();
    
    // checks whether checksum of message is OK
    boolean checksumOk();
    // module is ready to send data
    boolean isReadyToSend();
    // has module finished receiving data
    boolean dataIsReady();
    
    void setReadyToReceive();
    void setNotReadyToReceive();
    
    // detects baudrate setting of the HC12 module
    void baudDetector();
    
    // set HC12 to default state across all baudrates
    void bruteSetDefault();
    
    // convenience function to set baudRate
    void safeSetBaudRate();
    
    // change transferDelay on-the-fly
    void setTransferDelay(unsigned int newTransferDelay);
private:
    // clears buffer
    void clearBuffer(char*, const size_t);
    
    // send cmds across all baudrates given in baudArray
    loopCmdRes loopCmd(char cmdChar[]);
    
    // prints buffer overflow message
    void bufferOverflowMsg();
};
 
#endif
