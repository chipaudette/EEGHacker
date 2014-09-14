/*
  This sketch is written for OBCI_V3 hardware. It sends dummy data for testing through-put.
  
  UNO waits to receive 'b' then sends a string of dummy data 
  sampleCounter: 1 byte
  dummy ADS data: 8x3 bytes
  dummy accel data 3x2 bytes
  31 bytes all day
  
  Dummy data transfer is stopped when UNO receives 's'
  
  When not streamingData, UNO echos bytes received
  
  
    
*/

const int dataLength = 31; 
char dummyData[dataLength] = {  // use ascii hex for verbosity
  'a','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',  
  '1','2','3','4','5','6','7','8','9','A','B','C','D','E','F'  
};

char sampleCounter = 'a';         // sample counter
boolean streamingData = false;  // streamingData flag is set when 'b' is received

unsigned long timeBetweenSamples;  // used to generate sample freqeuncy
unsigned long benchWriteTime = 0;  // ditto

const int ADS_SS = 10;    // all the slave selects on 8bit OBCI board
const int DAISY_SS = 7;
const int SD_SS = 6;
const int LIS3DH_SS = 5;
const int ADS_RST = 9;

void setup() {
  Serial.begin(115200);   
  
  pinMode(SD_SS, OUTPUT); digitalWrite(SD_SS, HIGH);       // disable the slaves
  pinMode(DAISY_SS,OUTPUT); digitalWrite(DAISY_SS,HIGH);
  pinMode(ADS_SS,OUTPUT); digitalWrite(ADS_SS,HIGH);
  pinMode(LIS3DH_SS,OUTPUT); digitalWrite(LIS3DH_SS,HIGH);
  pinMode(ADS_RST,OUTPUT); digitalWrite(ADS_RST,LOW);      // put the ADS in reset for fun
  delay(500);      
  Serial.println("send 'b' to start    send 's' to stop");
}

void loop() {
  
  if(streamingData){              // receive 'b' on serial to set this
    benchWriteTime = millis();      // start bench mark timer
    dummyData[0] = sampleCounter;   // update sampleCounter to send
    for (int i=0; i<dataLength; i++){
      Serial.write(dummyData[i]);  // send a 31 byte data packet  
    }
    sampleCounter++;                // increment sampleCounter
    if(sampleCounter > 'z'){sampleCounter = 'a';}    // roll-over sampleCounter
    benchWriteTime = millis() - benchWriteTime;      // stop bench mark timer
    timeBetweenSamples = 4000 - benchWriteTime;      // set sample frequency
    delayMicroseconds(timeBetweenSamples);           
  } 
 
}

void serialEvent(){
  char token = Serial.read();
  
  switch (token) {
    case 'b':
      streamingData = true;
      break;
    case 's':
      streamingData = false;
      break;
      
    default:
      break;
   }
   
   if(!streamingData){ // echo serial
     Serial.print("got the ");
     Serial.println(token);
    }
    
   
}

