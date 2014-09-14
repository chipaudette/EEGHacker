/* HOST
This sketch sets us a HOST device to demonstrate 
a serial link between RFDuino modules

Since the Device must initiate communication, the
device "polls" the Host evey 50mS when not sending packets.
Host is connected to PC via USB VCP (FTDI).
Device is connectedd to uC (Arduino or compatible).
The code switches between 'normal' mode and 'streamingData' mode.
Normal mode expects a call-response protocol between the Host PC and Device uC.
Normal mode supports avrdude and will allow over-air upload to Device-connected uC.

StreamingData mode expects a continuous stream of data from the uC via Device.
Host insterts a pre-fix and post-fix to the data for PC coordination.
This code tends to break in a way that point toward the Host -> PC connection.

This is using serial buffer of 128 bytes

Made by Joel Murphy with Leif Percifield and Conor Russomanno, Summer 2014
Free to use and share. This code presented for use as-is. wysiwyg.
*/

#include <RFduinoGZLL.h>

device_t role = HOST;  // This is the HOST code!
int LED = 2;           // blue LED on GPIO2

const int numBuffers = 20;              // buffer depth
char serialBuffer[numBuffers] [32];  	  // buffers to hold serial data
int bufferLevel = 0;                 	  // counts which buffer array we are using
int serialIndex[numBuffers];         	  // Buffer position counter
int serialBuffCounter = 0;
unsigned long serialTimer;              // used to time end of serial message
 boolean serialToSend = false;           // set when serial data is ready to go to serial
boolean serialTiming = false;           // set to start serial timer

char radioBuffer[300];		              // buffer to hold radio data
int radioIndex = 0;                  	  // used in sendToHost to protect len value
int packetCount = 0;                    // used to keep track of packets in received radio message
int packetsReceived = 0;                // used to count incoming packets
boolean radioToSend = false;            // set to send data from Device


int resetPin = 6;                       // GPIO6 connected to Arduino MCLR pin through 0.1uF 
int resetPinValue;                      // used to hold digitalReadin
int lastResetPinValue;                  // used to find rising/falling edge
char RFmessage[1];                      // can't get on the radio without an array
boolean sendRFmessage = false;          // flag to send radio to radio message

boolean streamingData = false;          // flag to get into streamingData mode
int numBytes;                           // counter for receiving/sending stream
int tail = 0;                           // used when streaming to make ring buffer
boolean streamToSend = false;           // send radio data to serial port using ring buffer and pre/post-fix
unsigned long totalPacketsReceived = 0; // used for verbose 


void setup(){

  RFduinoGZLL.begin(role); // start the GZLL stack
  Serial.begin(115200);  // start the serial port
  
  initBuffer();  // prime the serialBuffer
  
  pinMode(resetPin,INPUT);  // DTR from FTDI routed to GPIO6 through switcc
  lastResetPinValue = digitalRead(resetPin);  // prime lastResetPinValue
  pinMode(LED,OUTPUT);    // blue LED on GPIO2
  digitalWrite(LED,HIGH); // trun on blue LED!
  
}



void loop(){
  
  resetPinValue = digitalRead(resetPin);
  if(resetPinValue != lastResetPinValue){
    if(resetPinValue == LOW){
      RFmessage[0] = '9';  // Device will toggle target MCLR when it gets '9'
    }else{
      RFmessage[0] = '(';  // Device will toggle target MCLR when it gets '('
    }  
    sendRFmessage = true; 
    lastResetPinValue = resetPinValue;
  }

  if(serialTiming){                    // if the serial port is active
    if(millis() - serialTimer > 2){   // check for idle time out
      serialTiming = false;	       // clear serialTiming flag
      if(serialIndex[0] == 2){      // single byte messages from uC are special
        testSerialByte(serialBuffer[0][1]);  // could be command to streamData, go sniff it
      }                                                   // if we started a buffer and didn't add any bytes,
      if(serialIndex[bufferLevel] == 0){bufferLevel--;}   // don't send more buffers than we have!
      serialBuffer[0][0] = bufferLevel +1;  // drop the packet count into zero position (Device knows where to find it)
      serialBuffCounter = 0;              // get ready to count the number of packets we send
      serialToSend = true;                // set serialToSend flag. transmission starts on next packet from Device
    }
  }

  if(streamToSend){       // when we are streaming radio data
    Serial.write(0xA0);                // send the pre-fix
    for(int i=0; i<numBytes; i++){    
      Serial.write(radioBuffer[tail]);    // using ring buffer
      tail++; if(tail == 300){tail = 0;}
    }
      Serial.write(0xC0);              // send the post-fix
    streamToSend = false;
  }
  
  if(radioToSend){  // when radio data is ready to send
    for(int i=0; i<radioIndex; i++){
      Serial.write(radioBuffer[i]);  // send it out the serial port
    }
    radioIndex = 0;
    radioToSend = false;    // reset radioToSend flag
  }
  



  if(Serial.available()){   // when the serial port is acive
    while(Serial.available() > 0){    // collect the bytes in 2D array
      serialBuffer[bufferLevel][serialIndex[bufferLevel]] = Serial.read();    
      serialIndex[bufferLevel]++;           // count up the buffer size
      if(serialIndex[bufferLevel] == 32){	  // when the buffer is full,
        bufferLevel++;			  // next buffer please
      }  	
    }
    serialTiming = true;      // turn on the serial idle timer
    serialTimer = millis();   // set the serial idle clock
  }

}


boolean sendEbrake = false;
char eBrake[2];
void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len){
    
    if(sendRFmessage){       // if we have to send a private message to Device
      RFduinoGZLL.sendToDevice(device, RFmessage, 1);
      sendRFmessage = false; // put down the sendRfdMessage flag
      return;
    }
    
    if(sendEbrake){
      RFduinoGZLL.sendToDevice(device, eBrake, 2);
      sendEbrake = false;
      return;
    }
    
    if(serialToSend){
      RFduinoGZLL.sendToDevice(device,serialBuffer[serialBuffCounter], serialIndex[serialBuffCounter]);
      serialBuffCounter++;	// get ready for next buffered packet
      if(serialBuffCounter == bufferLevel +1){// when we send all the packets 
        serialToSend = false; 		    // put down bufferToSend flag
        bufferLevel = 0;			   
        initBuffer();                           // initialize serialBuffer
      }
    }
    
    
    
    if(len > 0){ 

          int startIndex = 0;	        // get ready to read this packet   
          if(packetCount == 0){	        // if this is a fresh transaction  
            packetCount = data[0];	// get the number of packets to expect in message
            startIndex = 1;		// skip the first byte of the first packet when retrieving radio data
          }		
          for(int i = startIndex; i<len; i++){
            radioBuffer[radioIndex] = data[i];  // retrieve the packet
            radioIndex++; if(radioIndex == 300){radioIndex = 0;}
          }
          packetsReceived++;
          if(packetsReceived == packetCount){		// we got all the packets
            packetsReceived = 0;
            packetCount = 0;
            // totalPacketsReceived++;     // this is used for verbose feedback
            if(streamingData){               
              numBytes = len-1;             // set the output byte count based on len
              streamToSend = true;          // set the stream packet flag
            }else{
              radioToSend = true;           // set serial pass flag
            }
          }
    }

}// end of onReceive

// sniff the serial command sent from PC to uC
void testSerialByte(char z){
  switch(z){
    case 'b':  // PC wants to stream data
      streamingData = true;  // enter streaimingData mode
      radioIndex = 0;        
      tail = 0;             
      break;
      
    case 's':  // PC sends 's' to stop streaming data
      // Serial.print(totalPacketsReceived);Serial.println(" Packets"); // verbose
      // totalPacketsReceived = 0;
      streamingData = false;  // get out of streamingData mode
      radioIndex = 0;         // reset radioBuffer index
      break;
      
    default:
      break;
  } 
}

void initBuffer(){
  serialIndex[0] = 1;     // make room for the packet checkSum!
  for(int i=1; i<numBuffers; i++){
    serialIndex[i] = 0;   // initialize indexes to 0
  }
}



// end
