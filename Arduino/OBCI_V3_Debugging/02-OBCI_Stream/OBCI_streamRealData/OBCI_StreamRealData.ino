

/*
 *
 *  >>>> THIS CODE USED TO TEST OBCI V3 PROTOTYPES <<<<
 *
 * testing BLOCK WRITE using REAL OBCI data
 *
 * This code is written to run on the OpenBCI V3 board.
 * Adjust as needed if you are testing on different hardware.
 *
 * By Chip Audette August, 2014
 * Based on Skeleton Code from Joel Muprhy, Spring/Summer, 2014.
 *
 */

#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>      // from https://github.com/greiman/SdFat
#include <SdFatUtil.h>  // from https://github.com/greiman/SdFat
#include "OpenBCI_04.h"


//-----------------------------------------------------------------
// Data info
const int dataLength = 24;  // size (bytes) of payload in data packet
char serialCheckSum;            // holds the byte count for streaming
int sampleCounter = 0;         // sample counter
word packetCounter = 0;         // used to limit the number of packets during tests
boolean streamingData = false;  // streamingData flag is set when 'b' is received

//------------------------------------------------------------------------------
//  << OpenBCI BUSINESS >>
OpenBCI OBCI; //Uses SPI bus and pins to say data is ready.  Uses Pins 13,12,11,10,9,8,4
//#define MAX_N_CHANNELS (8)  //must be less than or equal to length of channelData in ADS1299 object!!
int nActiveChannels = 8;   //how many active channels would I like?
byte gainCode = ADS_GAIN24;   //how much gain do I want
//byte inputType = ADSINPUT_NORMAL;   //here's the normal way to setup the channels
//byte inputType = ADSINPUT_SHORTED;   //here's the normal way to setup the channels
byte inputType = ADSINPUT_TESTSIG;   //here's the normal way to setup the channels
boolean is_running = false;    // is the ADS1299 running?
boolean startBecauseOfSerial = false;
char leadingChar;
int outputType;


// use cout to save memory use pstr to store strings in flash to save RAM
ArduinoOutStream cout(Serial);

void startData(void) {
  //cout << pstr("Starting OpenBCI data log to ") << currentFileName << pstr("\n");
  //SPI.setDataMode(SPI_MODE1);
  OBCI.start_ads();
  is_running = true;
}
void stopData(void) {
  is_running = false;
  //SPI.setDataMode(SPI_MODE1);
  OBCI.stop_ads();
  //SPI.setDataMode(SPI_MODE0);
  //OBCI.disable_accel();
  delay(100);
}
void startRFDuinoStreaming(void) {
  Serial.print('B');   //RFDuino is looking for this character, I guess
  delay(20);           // give Device time to catch up
  streamingData = true;
}
void stopRFDuinoStreaming(void) {
  Serial.write(0x01); //code to tell RFDuino to look for the 'S'
  Serial.print('S');  //RFDuino is looking for this character, I guess
  delay(20);           // give Device time to catch up
  streamingData = false;
}

void setup(void) {

  Serial.begin(115200);
  stopRFDuinoStreaming();  //ensure we're in STANDARD mode

  // pinMode(LIS3DH_SS,OUTPUT); digitalWrite(LIS3DH_SS,HIGH);   // de-select the LIS3DH
  pinMode(SD_SS, OUTPUT); digitalWrite(SD_SS, HIGH);         // de-select the SD card
  //  pinMode(ADS_SS,OUTPUT); digitalWrite(ADS_SS,HIGH);         // de-select the ADS
  //  pinMode(DAISY_SS,OUTPUT); digitalWrite(DAISY_SS,HIGH);         // de-select the Daisy Module


  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  delay(3000); //was 4000
  cout << pstr("\nOBCI_StreamOneChannel\n");  delay(1000);

  //initialize the ADS Hardware
  SPI.setDataMode(SPI_MODE1); delay(20);
  OBCI.initialize_ads();
  OBCI.ads.configureInternalTestSignal(ADSTESTSIG_AMP_1X, ADSTESTSIG_PULSE_FAST); //only needed if we ever want to use test signals
  for (int chan = 1; chan <= nActiveChannels; chan++) {
    OBCI.activateChannel(chan, gainCode, inputType);
  }
  delay(500);

  //send instructions to user
  cout << pstr("'b' to start\n"); delay(500);// prompt user to begin test
  cout << pstr("'s' to stop\n"); delay(500);// prompt user to begin test
  //benchWriteTime = 0;           // used to benchmark transmission time
  serialCheckSum = dataLength + 1;  // serialCheckSum includes sampleCounter
  sampleCounter = 0;

}



void loop() {

  if (is_running) {

    //wait until data is available
    //SPI.setDataMode(SPI_MODE1);
    while (!(OBCI.isDataAvailable())) { // watch the DRDY pin
      delayMicroseconds(10);
    }

    //get the data
    //SPI.setDataMode(SPI_MODE1);
    OBCI.updateChannelData();

    if (streamingData) {            // receive 'b' on serial to set this
      sampleCounter++;  //count how many samples we've taken
      
      //benchWriteTime = micros();                 // BENCHMARK SAMPLE WRITE TIME
      Serial.write(serialCheckSum);              // send the number of bytes to follow
      //Serial.write(0xA0);                        // send the start byte
      Serial.write(lowByte(packetCounter));         // send the sampleCounter
      
       //write OpenBCI's raw bytes
       //Serial.write(OBCI.ads.rawChannelData, min(dataLength, (nActiveChannels * 3))); // send one byte of data
       for (int i=0; i < 24; i++) {
         Serial.write(OBCI.ads.rawChannelData[i]); // send one byte of data
       }
      
       
       //increment the packet counter
       packetCounter++;  if (packetCounter > 255) packetCounter = 0; //constrain
    }

  //is it time to stop?
//  if (sampleCounter >= 120*250) {  //at 250 Hz
//      stopRFDuinoStreaming();
//      //cout << pstr("\nStopping\n");
//      delay(1000);
//      stopData(); //stop ADS1299
//    }
  }
}

void serialEvent() { //done at end of every loop(), when serial data has been detected

  while (Serial.available()) { //loop here until the receive buffer is empty!
    char token = Serial.read();
    //Serial.print("got ");  Serial.write(token);  Serial.print('\n'); delay(400);

    switch (token) {
      case 'b':
        //cout << pstr("\nStarting\n");
        delay(1000);
        startRFDuinoStreaming();        //put RFDuino into STREAMING mode
        startData();        //start ADS1299
        sampleCounter = 0;
        break;

      case 's':
        //put RFDuino back into STANDARD mode
        stopRFDuinoStreaming();

        //stop the ADS1299
        stopData();
        //cout << pstr("\nStopping\n");
        delay(1000);
        break;

      default:
        break;
    }
    //     if(!streamingData){  // send checkSum for the verbose to follow
    //       Serial.print("got the ");
    //       Serial.write(token);
    //       Serial.print('\n');
    //     }
    delay(20);
  }
}
