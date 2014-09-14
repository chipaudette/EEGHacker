/*
 * 
 *  >>>> THIS CODE USED TO TEST OBCI V3 PROTOTYPES <<<<
 *
 * testing BLOCK WRITE using REAL OBCI data  
 * logging 3 byte OBCI data in HEX format using ',' separator and '\n' terminator
 * 
 * This sketch sets the ADS inputs to GND and logs 20 seconds of data
 * Data can be used to measure input referred noise
 *
 * This code is written to run on the OpenBCI V3 board. 
 * Adjust as needed if you are testing on different hardware.
 *
 * If a high quality SanDisk card is used with this sketch
 * no overruns occur and the maximum block write time is
 * ~ 700 micros.
 *
 * Made by Joel Murphy, Spring, 2014. 
 * Based on RawWrite example in SDFat library 
 */

#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>
#include <SdFatUtil.h>
#include "OpenBCI_04.h"  


//------------------------------------------------------------------------------
//  << SD CARD BUSINESS >>
//  SD_SS on pin 7 defined in OpenBCI library
// number of blocks in the contiguous file
const uint32_t BLOCK_COUNT = 100UL;  // this times 512 = size of file to allocate
// time to produce a block of data not critical. used for benchmarking only
const uint32_t MICROS_PER_BLOCK = 2000; // minimum time that won't trigger overrun error
SdFat sd; // file system
SdFile file; // test file
uint32_t bgnBlock, endBlock; // file extent
#define error(s) sd.errorHalt_P(PSTR(s)) // store error strings in flash to save RAM
uint32_t b = 0;  // used to count blocks
uint8_t* pCache;  // used to cache the data before writing to SD
// benchmark stats
  uint16_t overruns = 0;
  uint32_t maxWriteTime = 0;
  uint16_t minWriteTime = 65000;
  uint32_t t = micros();
// log of first overruns
#define OVER_DIM 20
struct {
  uint32_t block;   // holds block number that over-wrote
  uint32_t micros;  // holds the length of this of over-write
} over[OVER_DIM];

int byteCounter = 0;     // used to hold position in cache
boolean logging = false;
unsigned int timeBetweenFiles;
boolean betweenFiles = false;

//------------------------------------------------------------------------------
//  << OpenBCI BUSINESS >>
OpenBCI OBCI; //Uses SPI bus and pins to say data is ready.  Uses Pins 13,12,11,10,9,8,4
//#define MAX_N_CHANNELS (8)  //must be less than or equal to length of channelData in ADS1299 object!!
int nActiveChannels = 8;   //how many active channels would I like?
byte gainCode = ADS_GAIN24;   //how much gain do I want
byte inputType = ADSINPUT_SHORTED;   //here's the normal way to setup the channels
unsigned int sampleCounter = 0;      // used to time the tesing loop
boolean is_running = false;    // this flag is set in serialEvent on reciept of prompt
boolean startBecauseOfSerial = false;
char leadingChar;
int outputType;

//------------------------------------------------------------------------------
//  << LIS3DH Accelerometer Business >>
//  LIS3DH_SS on pin 5 defined in OpenBCI library
int axisData[3];  // holds X, Y, Z accelerometer data
boolean xyzAvailable = false;
//------------------------------------------------------------------------------
byte fileTens, fileOnes;  // enumerate succesive files on card and store number in EEPROM 
char currentFileName[] = "OBCI_00.TXT";

// use cout to save memory use pstr to store strings in flash to save RAM
ArduinoOutStream cout(Serial); 
//------------------------------------------------------------------------------
  
//------------------------------------------------------------------------------

void setup(void) {

  Serial.begin(115200);
//  pinMode(LIS3DH_SS,OUTPUT); digitalWrite(LIS3DH_SS,HIGH);   // de-select the LIS3DH
  pinMode(SD_SS,OUTPUT); digitalWrite(SD_SS,HIGH);           // de-select the SD card
//  pinMode(ADS_SS,OUTPUT); digitalWrite(ADS_SS,HIGH);         // de-select the ADS
//  pinMode(DAISY_SS,OUTPUT); digitalWrite(DAISY_SS,HIGH);         // de-select the Daisy Module
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  
  incrementFileCounter();
  
  delay(4000);
 
  OBCI.initialize_ads();
  cout << ("OpenBCI Log Data To SD card\nSetting ADS1299 Channel Values\n");

//  SPI.setDataMode(SPI_MODE1);
  for (int chan=1; chan <= nActiveChannels; chan++) {
    OBCI.activateChannel(chan, gainCode, inputType);
  }
  
  cout << ("Setting Up Accelerometer\n");
//  SPI.setDataMode(SPI_MODE3);  
  OBCI.initialize_accel();
  OBCI.disable_accel();
  Serial.print("LIS3DH Device ID: 0x"); Serial.println(OBCI.getAccelID(),HEX);Serial.println();
  OBCI.printAllRegisters(); //print state of all registers ADS and LIS3DH
//  SPI.setDataMode(SPI_MODE0);

  while (!sd.begin(SD_SS, SPI_FULL_SPEED)){ 
    Serial.println(F("SD begin fail... Card Insterted?"));
    sd.begin(SD_SS, SPI_FULL_SPEED);
  }

}



void loop() {
  while (Serial.read() >= 0) {}  // clear out the serial buffer
  
  cout << pstr("Type any character to start logging\n");  // prompt user to begin test
  while (Serial.read() <= 0) {}  // wait here for serial input

//  cout << pstr("Free RAM: ") << FreeRam() << endl; // how much RAM?
  sampleCounter = 0;
  // initialize the SD card at SPI_FULL_SPEED for best performance.
  // try SPI_HALF_SPEED if bus errors occur.
//  while (!sd.begin(SD_SS, SPI_FULL_SPEED)){ 
//    Serial.println(F("SD begin fail... Card Insterted?"));
//    sd.begin(SD_SS, SPI_FULL_SPEED);
//  }
  sd.remove(currentFileName);  // delete possible existing file. name must be 8.3 format
  // create a contiguous file
  if (!file.createContiguous(sd.vwd(), currentFileName, 512UL*BLOCK_COUNT)) {
    Serial.print(F("createContiguous fail"));
  }
  // get the location of the file's blocks
  if (!file.contiguousRange(&bgnBlock, &endBlock)) {
    
  }
  //*********************NOTE**************************************
  // NO SdFile calls are allowed while cache is used for raw writes
  //***************************************************************
  // clear the cache and use it as a 512 byte buffer
  pCache = (uint8_t*)sd.vol()->cacheClear();
  // tell card to setup for multiple block write with pre-erase
  if (!sd.card()->erase(bgnBlock, endBlock)) Serial.println(F("erase block fail"));
  if (!sd.card()->writeStart(bgnBlock, BLOCK_COUNT)) {
    Serial.println(F("writeStart fail"));
  }
  
  
  digitalWrite(SD_SS,HIGH);  // de-select SD card
  // initialize benchmark stats for write time test
    overruns = 0;  // number of overruns
    maxWriteTime = 0;  // longest block write time
    minWriteTime = 65000;  // shortest block write time
    b = 0;   // block counter
    t = micros();  // note the time
    byteCounter = 0;
    
    if (betweenFiles){
      timeBetweenFiles = millis() - timeBetweenFiles;
      cout << pstr("time between files ") << timeBetweenFiles;
      
      for (int currentNibble = 3; currentNibble >= 0; currentNibble--){
          byte nibble = (timeBetweenFiles >> currentNibble*4) & 0x0F;
          if (nibble > 9){
            nibble += 55;  // convert to ASCII A-F
          }else{
            nibble += 48;  // convert to ASCII 0-9
          }
          pCache[byteCounter] = nibble;
          byteCounter++;
          if(byteCounter == 512){
            overRun();
          }
        }
        pCache[byteCounter] = '\n';  // put a carriage return after the time between files
          byteCounter++; 
          if(byteCounter == 512){
            overRun();
          }
    }
    cout << pstr("Enable Accelerometer\n");
    OBCI.enable_accel();
    
    cout << pstr("Starting OpenBCI data log to ") << currentFileName << pstr("\n"); 
    SPI.setDataMode(SPI_MODE1);
    OBCI.start_ads();
    
    is_running = true;
    
    while(is_running){
      while(!(OBCI.isDataAvailable())){   // watch the DRDY pin
        delayMicroseconds(10);
      }

      SPI.setDataMode(SPI_MODE1);
      OBCI.updateChannelData();
      
      if(OBCI.LIS3DH_DataReady()){
        SPI.setDataMode(SPI_MODE3);
        axisData[0] = OBCI.getX();
        axisData[1] = OBCI.getY();
        axisData[2] = OBCI.getZ();
        xyzAvailable = true;
      }
      SPI.setDataMode(SPI_MODE0);
      
    // convert 16 bit sampleCounter into HEX. 0000|0000 0000|0000 
    for (int currentNibble = 3; currentNibble >= 0; currentNibble--){
          byte nibble = (sampleCounter >> currentNibble*4) & 0x0F;
          if (nibble > 9){
            nibble += 55;  // convert to ASCII A-F
          }else{
            nibble += 48;  // convert to ASCII 0-9
          }
          pCache[byteCounter] = nibble;
          byteCounter++;
          if(byteCounter == 512){
            overRun();
          }
        }
        sampleCounter++;
        pCache[byteCounter] = ',';  // place the comma between the sample number and the data
          byteCounter++; 
          if(byteCounter == 512){
            overRun();
          }
      
    // convert 24 bit channelData into HEX. 0000|0000 0000|0000 0000|0000
    for (int currentChannel = 0; currentChannel < 8; currentChannel++){
       
        for (int currentNibble = 5; currentNibble >= 0; currentNibble--){
          byte nibble = (OBCI.ads.channelData[currentChannel] >> currentNibble*4) & 0x0F;
          if (nibble > 9){
            nibble += 55;  // convert to ASCII A-F
          }else{
            nibble += 48;  // convert to ASCII 0-9
          }
          pCache[byteCounter] = nibble;
          byteCounter++;
          if(byteCounter == 512){
            overRun();
          }
        }
        
        if(currentChannel < 7){
          pCache[byteCounter] = ',';
          byteCounter++; 
          if(byteCounter == 512){
            overRun();
          }
        }
      }// end of ADS data log loop
      
      if(xyzAvailable){  // if we have accelerometer data to log
        xyzAvailable = false;  // reset accelerometer data flag
        pCache[byteCounter] = ',';  // add a comma separator
          byteCounter++; 
          if(byteCounter == 512){
            overRun();
          }
        // convert 16 bit accelerometer data into HEX 0000|0000 0000|0000  
        for (int currentChannel = 0; currentChannel < 3; currentChannel++){
       
        for (int currentNibble = 3; currentNibble >= 0; currentNibble--){
          byte nibble = (axisData[currentChannel] >> currentNibble*4) & 0x0F;
          if (nibble > 9){
            nibble += 55;  // convert to ASCII A-F
          }else{
            nibble += 48;  // convert to ASCII 0-9
          }
          pCache[byteCounter] = nibble;
          byteCounter++;
          if(byteCounter == 512){
            overRun();
          }
        }
        
        if(currentChannel < 2){
          pCache[byteCounter] = ',';
          byteCounter++; 
          if(byteCounter == 512){
            overRun();
          }
        }
      }// end of accelerometer data log loop
      }// end of xyzAvailable
      
      // end of nested loops
    pCache[byteCounter] = '\n';
    byteCounter++; 
    if(byteCounter == 512){
        overRun();
    }
  }
  
  t = micros() - t;  // measure total write time

  SPI.setDataMode(SPI_MODE1);
  OBCI.stop_ads();
  SPI.setDataMode(SPI_MODE0);

  OBCI.disable_accel(); 


  // end multiple block write mode
  if (!sd.card()->writeStop()) Serial.println(F("writeStop fail"));
  // close file for next pass of loop
  file.close();
// increment the file name number for logging to a new file  
  incrementFileCounter();
  
  cout << pstr("Done\n");
  cout << pstr("Elapsed time: ") << setprecision(3)<< 1.e-6*t;
  cout << pstr(" seconds\n");
  cout << pstr("Max write time: ") << maxWriteTime << pstr(" micros\n");
  cout << pstr("Min write time: ") << minWriteTime << pstr(" micros\n");
  cout << pstr("Overruns: ") << overruns << endl;
  if (overruns) {
    uint8_t n = overruns > OVER_DIM ? OVER_DIM : overruns;
    cout << pstr("fileBlock,micros") << endl;
    for (uint8_t i = 0; i < n; i++) {
      cout << over[i].block << ',' << over[i].micros << endl;
    }
  }
  
  Serial.println();
}

void overRun(){
    // write the 512 byte block
    byteCounter = 0; // reset 512 byte counter
    uint32_t tw = micros();  // time the write
    if (!sd.card()->writeData(pCache)){ 
      Serial.print("writeData failed"); 
      Serial.println(b);
      b = BLOCK_COUNT-1;
    }
    tw = micros() - tw;
    
    if (tw > maxWriteTime) {  // check for max write time
      maxWriteTime = tw;
    }
    if (tw < minWriteTime){   // check for min write time
      minWriteTime = tw;
    }
    // check for overrun
    if (tw > MICROS_PER_BLOCK) {
      if (overruns < OVER_DIM) {
        over[overruns].block = b;
        over[overruns].micros = tw;
      }
      overruns++;
    }
    
    b++;    // increment BLOCK counter
    if(b == BLOCK_COUNT){
      is_running = false;
      timeBetweenFiles = millis(); 
      betweenFiles = true;   
    }  // we did it!
    // write must be done by this time
    
}


void incrementFileCounter(){
  fileTens = EEPROM.read(0);
  fileOnes = EEPROM.read(1);
  // if it's the first time writing to EEPROM, seed the file number to '00'  
  if(fileTens == 0xFF | fileOnes == 0xFF){
    fileTens = fileOnes = '0';
  }
  fileOnes++;   // enumerate the file name in HEX
  if (fileOnes == ':'){fileOnes = 'A';}
  if (fileOnes > 'F'){
    fileOnes = '0';
    fileTens++; 
    if(fileTens == ':'){fileTens = 'A';} 
    if(fileTens > 'F'){fileTens = '0';}
  }
  EEPROM.write(0,fileTens);
  EEPROM.write(1,fileOnes);
  currentFileName[5] = fileTens;
  currentFileName[6] = fileOnes;
  Serial.print("fileNumber ");Serial.println(currentFileName); //Serial0.write(fileTens);Serial0.write(fileOnes);Serial0.println();
  
}

