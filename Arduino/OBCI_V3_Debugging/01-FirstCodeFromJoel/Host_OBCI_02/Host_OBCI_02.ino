/* HOST
This sketch sets us a HOST device to demonstrate 
a serial link between RFDuino modules

Since the Device must initiate communication, the
device "polls" the Host evey 100mS when not sending packets


Made by Joel Murphy and Leif Percifield, Summer 2014
Free to use and share. This code presented for use as-is. No promises!
*/

#include <RFduinoGZLL.h>

device_t role = HOST;  // This is the HOST code!
int LED = 2;  // blue LED on GPIO2

const int numBuffers = 20;            // buffer depth
char serialBuffer[numBuffers] [32];  	// buffers to hold serial data
int bufferLevel = 0;                 	// counts which buffer array we are using
int serialIndex[numBuffers];         	// Buffer position counter
int numPackets = 0;                  	// number of packets to send/receive on radio
int serialBuffCounter = 0;
unsigned long serialTimer;              // used to time end of serial message

char radioBuffer[300];		        // buffer to hold radio data
int radioIndex = 0;                  	// used in sendToHost to protect len value
int packetCount = 0;                    // used to keep track of packets in received radio message
int packetsReceived = 0;                // used to count incoming packets

boolean serialToSend = false;     // set when serial data is ready to go to serial
boolean radioToSend = false;     
boolean serialTiming = false;

int resetPin = 6;
int resetPinValue;
int lastResetPinValue;
char RFmessage[1];
boolean sendRFmessage = false;

//unsigned long zeroSpaceTimer;
//boolean testForZeroSpace = false;

boolean firstStreamingByte = true;
boolean initiatingDataStream = false;
#define FORMAT_NORMAL  (0)
#define FORMAT_OpenBCI  (1)
#define FORMAT_OpenBCI_AUX (2)
#define FORMAT_OpenBCI_4chan  (3)
#define FORMAT_OpenEEG  (4)
#define FORMAT_OpenVIBE  (5)
#define FORMAT_TEXT  (6)
int outputType;
int lastOutputType = FORMAT_NORMAL;

char sampleCounter;
long channelData[8];
int lisData[3];
boolean sendLIS3DH = false;

void setup(){

  RFduinoGZLL.begin(role); // start the GZLL stack
  Serial.begin(115200);  // start the serial port
  
  initBuffer();
  outputType = FORMAT_NORMAL;
  
  pinMode(resetPin,INPUT);
  lastResetPinValue = digitalRead(resetPin);
  pinMode(LED,OUTPUT);
  digitalWrite(LED,HIGH);
  
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
      if(serialIndex[0] == 2){      // single byte messages are special
        testSerialByte(serialBuffer[0][1]);  // sniff PC commands here
      }
      if(serialIndex[bufferLevel] == 0){bufferLevel--;}  // don't send more buffers than we have!
      serialBuffer[0][0] = bufferLevel +1;  // drop the packet count into zero position (packet checkSum)
      serialBuffCounter = 0;              // keep track of how many packets we send
      serialToSend = true;             // set serialToSend flag. transmission starts on next packet from Device
    }
  }

  
  if(radioToSend){  // when data comes in on the radio
    sendRadioData();
    radioToSend = false;    // reset radioToSend flag
  }
  



if(Serial.available()){
    while(Serial.available() > 0){
      serialBuffer[bufferLevel][serialIndex[bufferLevel]] = Serial.read();    
      serialIndex[bufferLevel]++;           // count up the buffer size
      if(serialIndex[bufferLevel] == 32){	  // when the buffer is full,
        bufferLevel++;			  // next buffer please
      }  	
    }
    serialTiming = true; 
    serialTimer = millis();  
}

}


void RFduinoGZLL_onReceive(device_t device, int rssi, char *data, int len){
  if(device == 1){
    
    if(sendRFmessage){       // if we have to send a private message to Device
      RFduinoGZLL.sendToDevice(device, RFmessage, 1);
      sendRFmessage = false; // put down the sendRfdMessage flag
      return;
    }
    
    
    if(serialToSend){
      RFduinoGZLL.sendToDevice(device,serialBuffer[serialBuffCounter], serialIndex[serialBuffCounter]);
      serialBuffCounter++;	// get ready for next buffered packet
      if(serialBuffCounter == bufferLevel +1){// when we send all the packets
        serialToSend = false; 		    // put down bufferToSend flag
        bufferLevel = 0;			    // initialize bufferLevel
        initBuffer();
      }
    }
    
    if(len == 2){testRadioByte(data[1]);}
    
    if(len > 0){   
        int startIndex = 0;	        // get ready to read this packet   
        if(packetCount == 0){	// if this is a fresh transaction  
          packetCount = data[0];	// get the number of packets to expect in message
          startIndex = 1;		// skip the first byte when retrieving radio data
        }		
        for(int i = startIndex; i < len; i++){
          radioBuffer[radioIndex] = data[i];
          radioIndex++;
        }
        packetsReceived++;
        if(packetsReceived == packetCount){		// we got all the packets
          packetsReceived = 0;
          packetCount = 0;
          radioToSend = true;			// set serial pass flag
        }
    }
  }
}// end of onReceive



boolean testSerialByte(char z){
  boolean r = false;

  switch(z){
    
    default:
      r = false;
      break;
  }

  return r;
}



void initBuffer(){
  serialIndex[0] = 1;     // make room for the number of packets!
  for(int i=1; i<numBuffers; i++){
    serialIndex[i] = 0;   // initialize indexes to 0
  }
}


boolean testRadioByte(char z){
  boolean r = false;
  switch(z){
    case 'S':                       // 'S' comes from Arduino to say it stops streaming
      lastOutputType = outputType;  // keep track of how we were formatting data before
      outputType = FORMAT_NORMAL;   // go back to normal serial pass thru
      r = false;
      break;
    case 'B':                       // 'B' comes from Arduino to say it starts streaming agian
      outputType = FORMAT_OpenBCI;  // go back to the last formatting method
      r = true;
      break;
    case 'N':
     lastOutputType = FORMAT_OpenBCI_AUX;
     r = true;  
      break;
    case 'V':  
     lastOutputType = FORMAT_OpenBCI_4chan;
     r = true;  
      break;
    case 'X':  
     lastOutputType = FORMAT_TEXT;
     r = true;  
      break;
    default:
      r = false;
      break;
  }
  initiatingDataStream = r;  // this flag allows (z) to pass thru. might not need?
  return r;
}


void sendRadioData(){
  // if this is the first time we transition from FORMAT_NORMAL to another format
  // this test will allow the command 'B' to get passed over serial. Might not need it.
  if(initiatingDataStream){ 
    sendDataNormal();
    initiatingDataStream = false;
    return;
  }
  
  switch(outputType){
    
    case FORMAT_NORMAL:
      sendDataNormal();
      break;
      
    case FORMAT_OpenBCI:
      sendLIS3DH = false;
      convertRawDataToInts(8);
      sendOpenBCI_Binary(8);
      break;
      
    case FORMAT_OpenBCI_AUX:
      sendLIS3DH = true;
      convertRawDataToInts(8);
      sendOpenBCI_Binary(8);
      break;

    case FORMAT_OpenBCI_4chan:
      sendLIS3DH = false;
      convertRawDataToInts(4);
      sendOpenBCI_Binary(4);
      
    case FORMAT_OpenEEG:
      sendLIS3DH = false;
      sendOpenEEG();
      break;
      
    case FORMAT_TEXT:
      sendText();
      break;
      
    default:
      break; 
  }
  
}

void convertRawDataToInts(int N){
  
  int packetIndexCounter = 0;
  sampleCounter = radioBuffer[packetIndexCounter];
  packetIndexCounter++;
  
  for(int i=0; i<N; i++){      // collect the channel data from the radio packet
    for(int j=0; j<3; j++){
      channelData[i] = (channelData[i] << 8) | radioBuffer[packetIndexCounter];
      packetIndexCounter++;
    }
  }
  for(int i=0; i<N; i++){	// convert 3 byte 2's compliment to 4 byte 2's compliment	
    if(bitRead(channelData[i],23) == 1){	
      channelData[i] |= 0xFF000000;
      }else{
      channelData[i] &= 0x00FFFFFF;
      }
   }
   
   if(sendLIS3DH){                // collect the accelerometer data from the radio packet
     for(int i=0; i<3; i++){
       for(int j=0; j<2; j++){
         lisData[i] = (lisData[i] << 8) | radioBuffer[packetIndexCounter];
         packetIndexCounter++;     
       } 
     }
   }
   radioIndex = 0;
}


void sendOpenBCI_Binary(int N){
  char payloadBytes = N*4 + 1; 
  if(sendLIS3DH){payloadBytes += 6;}
  
  Serial.write(0xA0);
  Serial.write(payloadBytes);
  Serial.write(sampleCounter);
  for (int chan=0; chan<8; chan++){
    int temp = channelData[chan];
    for(int b=3; b>=0; b--){
      char outByte = temp >> (8*b);  
      Serial.write(outByte);
    }
  }
  Serial.write(0xC0);
}

void sendDataNormal(){
  for(int i=0; i<radioIndex; i++){
    Serial.write(radioBuffer[i]);  // send it out the serial port
  }
  radioIndex = 0;
}

void sendText(){

}

void sendOpenEEG(){

}







// end
