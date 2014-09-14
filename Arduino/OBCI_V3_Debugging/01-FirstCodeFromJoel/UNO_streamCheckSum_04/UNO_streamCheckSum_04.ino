/*
  This sketch tests OBCI_V3 prototype using dummy data.
  
  USE THIS WITH THE LATEST DEVICE AND HOST CODE TO TEST STREAMING DATA
  
  OBCI onboard UNO waits to receive a 'b' 
  then sends a dummy sample counter followed by a string of dummy data
  When OBCI onboard UNO receives a 's' the dummy data transfer is stopped.
  
  If no 's' is received, the dummy transfer stops after 256 transmissions
  
  RFduino modules act as wireless serial pass thru.
  RFduinos use a time-out on the serial port to determine the end of data transmission.
  This code tests the time-out in data stream mode. May in fact be better to just count bytes...
  
  NEEDS TO LOAD 3 BYTE LONG DUMMY DATA PACKET
  
*/

const int dataLength = 24;  // size of 
  // load dummyData with ascii if you want to see it on the serial monitor
// char dummyData[dataLength] = {
//   '0','1','2','3','4','5','6','7','8','9','A','B','C','D','E','F',
//   '0','1','2','3','4','5','6','\n'
// }
  // load byte values for testing OpenBCI data conversion
char dummyData[dataLength] = {
  0xFF,0xFF,0x00,  // -256
  0x00,0x00,0xFF,  // 255
  0xFF,0xFF,0x00,
  0x00,0x00,0xFF,
  0xFF,0xFF,0x00,
  0x00,0x00,0xFF,
  0xFF,0xFF,0x00,
  0x00,0x00,0xFF
};

char serialCheckSum;            // holds the byte count for streaming 
char sampleCounter = 0;         // sample counter
word packetCounter = 0;         // used to limit the number of packets during tests
boolean streamingData = false;  // streamingData flag is set when 'b' is received

unsigned int timeBetweenSamples;  // used to generate sample freqeuncy
unsigned long benchWriteTime = 100;

const int ADS_SS = 10;    // all the slave selects
const int DAISY_SS = 7;
const int SD_SS = 6;
const int LIS3DH_SS = 5;
const int ADS_RST = 9;

void setup() {
  Serial.begin(115200); 
  // disable the devices
  pinMode(SD_SS, OUTPUT); digitalWrite(SD_SS, HIGH);
  pinMode(DAISY_SS,OUTPUT); digitalWrite(DAISY_SS,HIGH);
  pinMode(ADS_SS,OUTPUT); digitalWrite(ADS_SS,HIGH);
  pinMode(LIS3DH_SS,OUTPUT); digitalWrite(LIS3DH_SS,HIGH);
  pinMode(ADS_RST,OUTPUT); digitalWrite(ADS_RST,LOW);    // put the ADS in reset for fun
   
  delay(2000);
  Serial.print("send 'b' to start\n");
  Serial.print("send 's' to stop\n");
  benchWriteTime = 0;           // used to benchmark transmission time
  serialCheckSum = dataLength + 1;  // serialCheckSum includes sampleCounter
}

void loop() {
  
  if(streamingData){              // receive 'b' on serial to set this
    benchWriteTime = micros();                 // BENCHMARK SAMPLE WRITE TIME
    Serial.write(serialCheckSum);              // send the number of bytes to follow
    Serial.write(sampleCounter);               // send the sampleCounter
    for (int i=0; i<dataLength; i++){
      Serial.write(dummyData[i]);  // send a data sample
      
//      while(!bitRead(UCSR0A,UDRE0)){  // wait for RX/TX register empty flag!
//      }
      
    }
    benchWriteTime = micros() - benchWriteTime; // BENCHMARK SAMPLE WRITE TIME
    packetCounter++;    // count the number of times we send a sample   
    sampleCounter++;
    if (packetCounter == 1024){        // if we've sent enough samples
      packetCounter = 0;
      delay(50); 
      Serial.write(0x01);
      Serial.print('S');
      delay(20);
//      Serial.print(benchWriteTime); Serial.print(' ');  // BENCHMARK TIME TO WRITE SAMPLE PACKET 
//      Serial.println(benchWriteTime/dataLength);           // BENCHMARK TIME TO WRITE ONE BYTE  
      Serial.print("SEND 'b' TO START\n");
      Serial.print("SEND 's' TO STOP\n");
      streamingData = false;        // stop sending samples
    }
  
    timeBetweenSamples = 4000 - benchWriteTime;      // set sample frequency
    delayMicroseconds(timeBetweenSamples);           // time between sample writes
  }
  
}

void serialEvent(){
  char token = Serial.read();
  
  switch (token) {
    case 'b':
      streamingData = true;
      Serial.print('B');
      delay(20);        // give Device time to catch up
      break;
      
    case 's':
      if(streamingData){Serial.write(0x01);} // send checkSum for the command to follow
      streamingData = false;
      Serial.print('S');
      delay(20);        // give Device time to catch up
      break;
      
    default:
      break;
   }
   if(!streamingData){  // send checkSum for the verbose to follow
     Serial.print("got the ");
     Serial.write(token);
     Serial.print('\n');
   }
   delay(20);
}
