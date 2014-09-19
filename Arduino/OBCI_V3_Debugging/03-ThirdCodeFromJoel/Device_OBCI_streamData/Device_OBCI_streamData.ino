/* 

Set's up RFduino Device for OpenBCI using RFduinoGZLL library

This test behaves as a serial pass thru between two RFduinos,
and will reset a target Arduino UNO with GPIO6 for over-air programming

This code uses excessive buffering of Serial and Radio packets
2D arrays, and ring buffers are used
Also uses using timout to finding end of serial data package

Made by Joel Murphy, Leif Percifield and Conor Russomanno Summer 2014
Free to use and share. This code presented as-is. wysiwyg

  RFduino modules should act as transparent serial pass thru for best effect.
  Host and Device use both serial and radio to accopmlish this.
    Serial RX in normal mode:
      2mS idle timeOut, 2D linear serialBuffer [20][32]
    Serial TX in streamData mode:
      1000us idle timeOut, 2D ring serialBuffer [20][32]
    Radio RX in normal and streamingData mode:
      1st byte is packet checkSum, radioBuffer[300]
    Radio TX:

    
  In streamData mode, Device uses a fixed packet size and also a time-out
  There is a chance of packet corruption in the Device serial RX:
    If you don't use the time-out in streamData, the data can shift phase.
    
  Single byte commands sent from the PC are prepared by the Host with '+' before and after.
  This is to avoid letting the UNO get a 'ghost' command during streamData mode

Made by Joel Murphy with Leif Percifield and Conor Russomanno, Summer 2014
Free to use and share. This code presented for use as-is. wysiwyg.

*/

#include <RFduinoGZLL.h>

device_t role = DEVICE1;  // This is the DEVICE code

const int numBuffers = 20;            // serial buffer depth
char serialBuffer[numBuffers] [32];  	// packetize serial data for the radio
int bufferLevel = 0;                 	// counts which buffer array we are using [0...19]
int serialIndex[numBuffers];         	// each buffer array needs a position counter
int serialBuffCounter = 0;            // used to count Serial buffers as they go to radio
unsigned long serialTimer;            // used to time end of serial message
boolean serialTiming = false;         // used to time end of serial message
boolean serialToSend = false;         // set when serial data is ready to go to radio

char radioBuffer[300];		            // buffer to hold radio data
int radioIndex = 0;                  	// counts position in radioBuffer
int packetCount = 0;                  // used to hold packet checkSum 
int packetsReceived = 0;              // used to count incoming packets
boolean radioToSend = false;          // set when radio data is ready to go to serial

unsigned long lastPoll;           // used to time idle message to host
unsigned int pollTime = 50;       // time between polls when idling

int resetPin = 5;               // GPIO5 is connected to Arduino UNO pin with 0.1uF cap in series

boolean firstStreamingByte = false; // used to get the checkSum (first byte in streaming serial)
boolean streamingData = false;      // streamingData flag
unsigned long streamingIdleTimer;
int streamingByteCounter;           // used to count serial bytes
char ackCounter = '0';              // keep track of the ack from Host

int head, tail;                 // ring buffer tools
char needAck = 0x00;            // manage radio fifo with this    
unsigned long packetTimer;      // anti-corruption timer
boolean packetTiming = false;  
int bytesInPacket;


void setup(){
  RFduinoGZLL.begin(role);      // start the Gazelle stack
  Serial.begin(115200,3,2);     // start the serial port, rx = GPIO3, tx = GPIO2
  
  initBuffer();                 // start serial buffer in normal mode
   
  lastPoll = millis();          // start Device poll timer
  
  pinMode(resetPin,OUTPUT);      // set direction of GPIO6
  digitalWrite(resetPin,HIGH);   // take uC out of reset
  
  
}



void loop(){
  
  if(serialTiming){                      // if the serial port is active
    if(millis() - serialTimer > 2){      // time's up! send 'em if you got'em
      if(serialIndex[bufferLevel] == 0){bufferLevel--;}  // don't send more buffers than we have data in!
      serialBuffer[0][0] = bufferLevel +1;	// drop the packet checkSum into zero position
      serialBuffCounter = 0;          		  // initialize the packet counter
      serialTiming = false;	                // clear serialTiming flag
      serialToSend = true;                  // set serialToSend flag
      sendSerialToRadio();                  // start sending packets
    }
  }
  
  if(radioToSend){                   // when radio data is ready
    for(int i=0; i<radioIndex; i++){
      Serial.write(radioBuffer[i]);  // send it out the serial port
    }
    radioIndex = 0;                  // reset radioInex counter
    radioToSend = false;             // put down radioToSend flag
  }
    
    if(streamingData){
      if(millis() - streamingIdleTimer > 1000){
        streamingData = false;
        initBuffer();
      }
    }
  
    if (millis() - lastPoll > pollTime){  // make sure to ping the host if they want to send packet
      if(!serialTiming && !serialToSend && !streamingData){  // don't poll if we are doing something important!
        RFduinoGZLL.sendToHost(NULL,0);
      }
      lastPoll = millis();          // reset timer for next poll time
    }
  

if(Serial.available()){
  if(streamingData){    // if we are streaming data
    streamingIdleTimer = millis();
    // if the packet we were building is short by any amount
    if(packetTiming && (micros() - packetTimer > 1000)){
      firstStreamingByte = true;  // delete the lossy packet
      packetTiming = false;      // turn off the packetTimer
    }
    if(firstStreamingByte){     // if we expect a fresh packet
      serialIndex[head] = 0;    // set pointer at [head][0]
      serialBuffer[head][serialIndex[head]] = 0x01;   // stage packet checkSum at [head][0]
      serialIndex[head]++;        // increment pointer
      streamingByteCounter = 0;   // reset incoming byte counter
      firstStreamingByte = false; // clear flag
    }
    while(Serial.available() > 0){
      streamSerialBytes();        // go get 'em!
    }
    packetTimer = micros();       // start streaming idle time-out clock 
    packetTiming = true;         // remember to check timer
  }else{                                // if we're not streaming data
    while(Serial.available() > 0){      // while the serial is active
      serialTimeOut();                  // go get 'em!
    }
    serialTimer = millis();             // start idle time-out clock
    serialTiming = true;                // remember to check timer
  }
}

}// end of loop



void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len)
{

  if(ackCounter > '0'){ackCounter--;} // if we ever needed an ack, now's the time!
    
  if(serialToSend){	// send buffer to host in normal mode on ack so as not to clog the radio
    sendSerialToRadio();
  }
  
  if(len == 1){    // Radios communicate to eachother in 1 byte packets
    testFirstRadioByte(data[0]);       // check for special character 
    return;        // don't send radio message along to uC, get outa here!
  }      
  
  if(len == 4){   // single byte messages from PC is special for uC
    if((data[1] == '+') && (data[3] == '+')){
      testSecondRadioByte(data[2]);    // sniff special character for uC state
    }
  }
  
  if(len > 0){                    // we got a packet!!
    int startIndex = 0;	          // get ready to read this packet from 0
    if(packetCount == 0){	        // if this is the first packet in transaction       
      packetCount = data[0];	    // get the packet checkSum from first position
      startIndex = 1;		          // skip the index[0] when retrieving first radio packet
    }		
    for(int i = startIndex; i < len; i++){
      radioBuffer[radioIndex] = data[i];  // read packet contents into radioBuffer
      radioIndex++;                       // increment the radioBuffer index counter
    }
    packetsReceived++;                    // increment the packet counter
    if(packetsReceived == packetCount){   // when we get all the packets
      packetsReceived = 0;                // reset packetsReceived counter for next time
      packetCount = 0;                    // reset packetCount for next time
      radioToSend = true;	                // radioToSend flag read in loop
    }else{                                // if we're still expecting packets,
      RFduinoGZLL.sendToHost(NULL,0);     // poll host for next packet
    }
  }
  
}

void sendSerialToRadio(){  // put a <=32 byte buffer on the radio in normal mode
  RFduinoGZLL.sendToHost(serialBuffer[serialBuffCounter], serialIndex[serialBuffCounter]);
  serialBuffCounter++;        // get ready for next buffered packet
  if(serialBuffCounter == bufferLevel +1){  // when we send all the packets
    serialToSend = false;                   // put down bufferToSend flag
    bufferLevel = 0;                        // initialize bufferLevel
    initBuffer();                           // initialize bufffer
  }
}
    
void serialTimeOut(){     // used in normal mode to receive serial bytes
  serialBuffer[bufferLevel][serialIndex[bufferLevel]] = Serial.read();    
  serialIndex[bufferLevel]++;           // count up the buffer size
  if(serialIndex[bufferLevel] == 32){	  // when this buffer is full,
    bufferLevel++;			                // next buffer please
  }  // if we just got the last byte, and advanced the bufferLevel, the serialTimeout will catch it 
}

  
void streamSerialBytes(){   // used in streamingData mode to receive serial bytes
  
  serialBuffer[head][serialIndex[head]] = Serial.read();    // read serial into the ring buffer
  streamingByteCounter++;               // count up the streaming bytes
  serialIndex[head]++;                  // increment this buffer index

  if(streamingByteCounter == bytesInPacket){ // when we get all the bytes
    head++; if(head == 20){head = 0;}        // next head please!
    serialIndex[head] = 0;                   // set pointer to [0]   
    serialBuffer[head][serialIndex[head]] = 0x01;  // place the 0x01 packet checkSum
    serialIndex[head]++;                     // increment pointer
    streamingByteCounter = 0;                // initialize streamingByteCounter
    packetTiming = false;                   // turn off streaming idle timer
    if((ackCounter < '3') && (tail != head)){     // watch out for the Gazelle FIFO limit
      RFduinoGZLL.sendToHost(serialBuffer[tail],serialIndex[tail]); // send 'em if you got 'em
      ackCounter++;                               // keep track of FIFO
      tail++; if(tail == 20){tail = 0;}           // advance the tail
    }
  }
}

  
boolean testFirstRadioByte(char z){  // test the first byte of a new radio packet for reset msg
  boolean r;
 switch(z){
    case '9':                 // HOST sends '9' when its GPIO6 goes LOW
      digitalWrite(resetPin,LOW);  // clear RESET pin
      r = true;  
      break;
    case '(':                 // HOST sends '(' when its GPIO6 goes HIGH
      digitalWrite(resetPin,HIGH); // set RESET pin
      r = true;
      break;   
    default:
      r = false;
      break;
  }
  return r;     // this might be useful
}

boolean testSecondRadioByte(char z){  // test the second byte of a new radio packet
  boolean r = false;
  switch(z){
    case 'b':  // PC wants to stream data
      streamingData = true;       // set flags and initialize variables
      streamingByteCounter = 0;
      head = 0;
      tail = 0;
      firstStreamingByte = true;
      bytesInPacket = 31;
      streamingIdleTimer = millis();
      r = true;
      break;
      
    case 's':  // PC sends 's' to stop streaming data
      streamingData = false;
      initBuffer();   // prep 2D buffer for normal mode
      r = true;
      break;
      
    default:
      r = false;
      break;
  }
  return r; // this might be useful
}
  


void initBuffer(){             // initialize 2D serial buffer in normal mode
  serialIndex[0] = 1;          // save buffer[0][0] to hold number packet checkSum!
  for(int i=1; i<numBuffers; i++){
    serialIndex[i] = 0;        // initialize indexes to 0
  }
}

