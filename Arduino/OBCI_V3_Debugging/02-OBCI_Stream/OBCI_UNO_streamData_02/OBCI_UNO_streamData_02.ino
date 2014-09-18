/*
 * 
 *  >>>> THIS CODE USED TO STREAM OBCI V3 DATA TO DONGLE <<<<
 *
 *
 * This code is written to run on the OpenBCI V3 board. 
 * Adjust as needed if you are testing on different hardware.
 *
 *
 * Made by Joel Murphy, Luke Travis, Conor Russomanno Summer, 2014. 
 * Based on RawWrite example in SDFat library 
 * This software is provided as-is with no promise of workability
 * Use at your own risk, wysiwyg.

 TO DO
  port the serialEvent command handler
  check that all functions are built correctly
  verify, simplify for testing with GUI

  Don't send serial unless the command to stop has passed down the radio chain!
 */

#include <EEPROM.h>
#include <SPI.h>
#include <SdFat.h>   // not using SD. could be an option later
#include <SdFatUtil.h>
#include "OpenBCI_04.h"  


//------------------------------------------------------------------------------
//  << SD CARD BUSINESS >> has bee taken out. See OBCI_SD_LOG_CMRR 
//  SD_SS on pin 7 defined in OpenBCI library
//------------------------------------------------------------------------------
//  << OpenBCI BUSINESS >>
OpenBCI OBCI; //Uses SPI bus and pins to say data is ready.  Uses Pins 13,12,11,10,9,8,4
//#define MAX_N_CHANNELS (8)  //must be less than or equal to length of channelData in ADS1299 object!!
int nActiveChannels = 8;   //how many active channels would I like?
byte gainCode = ADS_GAIN24;   //how much gain do I want. adjustable
byte inputType = ADSINPUT_NORMAL;   //here's the normal way to setup the channels. adjustable
boolean is_running = false;    // this flag is set in serialEvent on reciept of ascii prompt
boolean startBecauseOfSerial = false; // not sure this is needed?
byte sampleCounter = 0;
char leadingChar;


//------------------------------------------------------------------------------
//  << LIS3DH Accelerometer Business >>
//  LIS3DH_SS on pin 5 defined in OpenBCI library
// int axisData[3];  // holds X, Y, Z accelerometer data MOVED TO LIBRARY-JOEL
boolean xyzAvailable = false;
boolean useAccel = false;
//------------------------------------------------------------------------------
//  << PUT BIQUAD FILTER BUSNIESS HERE >>
//------------------------------------------------------------------------------
#define OUTPUT_NOTHING (0)
//#define OUTPUT_TEXT (1)
#define OUTPUT_BINARY (2)
#define OUTPUT_BINARY_SYNTHETIC (3)
#define OUTPUT_BINARY_4CHAN (4)
//#define OUTPUT_BINARY_OPENEEG (6)
//#define OUTPUT_BINARY_OPENEEG_SYNTHETIC (7)
#define OUTPUT_BINARY_WITH_ACCEL (8)
int outputType;
//------------------------------------------------------------------------------
  
//------------------------------------------------------------------------------

void setup(void) {

  Serial.begin(115200);
  
  
  SPI.begin();
  SPI.setClockDivider(SPI_CLOCK_DIV2);
  
  delay(1000);
 
  OBCI.initialize();  // ADD OPTION FOR DAISY
  Serial.print(F("OpenBCI V3 Stream Data To Dongle\nSetting ADS1299 Channel Values\n"));
//  setup channels on the ADS as desired. specify gain and input type for each
 for (int chan=1; chan <= nActiveChannels; chan++) {
   OBCI.activateChannel(chan, gainCode, inputType); // add option to include in bias
   // add SRB2 inclusion here?
 }


  
  Serial.print(F("ADS1299 Device ID: 0x")); Serial.println(OBCI.getADS_ID(),HEX);
  Serial.print(F("LIS3DH Device ID: 0x")); Serial.println(OBCI.getAccelID(),HEX);
  OBCI.printAllRegisters(); //print state of all registers ADS and LIS3DH

  // tell the controlling program that we're ready to start!
  Serial.println(F("Press '?' to query and print ADS1299 register settings again")); //read it straight from flash
  Serial.println(F("Press 1-8 to disable EEG Channels, q-i to enable (all enabled by default)"));
  Serial.println(F("Press 'f' to enable filters.  'g' to disable filters"));
  Serial.println(F("Press 'b' (binary) to begin streaming data..."));  

}



void loop() {
  

  
  if(is_running){
    
      while(!(OBCI.isDataAvailable())){   // watch the DRDY pin
        // delayMicroseconds(10); // don't delay!
      }

      OBCI.updateChannelData(); // retrieve the ADS channel data 8x3 bytes
      //Apply  filers to the data here if desired. FILTERS NEEDS int CONVERSION
      if(OBCI.useAccel && OBCI.LIS3DH_DataReady()){
        OBCI.getAccelData();    // fresh axis data goes into the X Y Z 
      }
      
      OBCI.writeDataToDongle(sampleCounter);
      sampleCounter++;
  
  }

} // end of loop



#define ACTIVATE_SHORTED (2)
#define ACTIVATE (1)
#define DEACTIVATE (0)

void serialEvent(){
  while(Serial.available()){      
    char inChar = (char)Serial.read();
    switch (inChar){

//TURN CHANNELS ON/OFF COMMANDS
      case '1':
        changeChannelState_maintainRunningState(1,DEACTIVATE); break;
      case '2':
        changeChannelState_maintainRunningState(2,DEACTIVATE); break;
      case '3':
        changeChannelState_maintainRunningState(3,DEACTIVATE); break;
      case '4':
        changeChannelState_maintainRunningState(4,DEACTIVATE); break;
      case '5':
        changeChannelState_maintainRunningState(5,DEACTIVATE); break;
      case '6':
        changeChannelState_maintainRunningState(6,DEACTIVATE); break;
      case '7':
        changeChannelState_maintainRunningState(7,DEACTIVATE); break;
      case '8':
        changeChannelState_maintainRunningState(8,DEACTIVATE); break;
      case 'q':
        changeChannelState_maintainRunningState(1,ACTIVATE); break;
      case 'w':
        changeChannelState_maintainRunningState(2,ACTIVATE); break;
      case 'e':
        changeChannelState_maintainRunningState(3,ACTIVATE); break;
      case 'r':
        changeChannelState_maintainRunningState(4,ACTIVATE); break;
      case 't':
        changeChannelState_maintainRunningState(5,ACTIVATE); break;
      case 'y':
        changeChannelState_maintainRunningState(6,ACTIVATE); break;
      case 'u':
        changeChannelState_maintainRunningState(7,ACTIVATE); break;
      case 'i':
        changeChannelState_maintainRunningState(8,ACTIVATE); break;
     
//TEST SIGNAL CONTROL COMMANDS
      case '0':
        activateAllChannelsToTestCondition(ADSINPUT_SHORTED,ADSTESTSIG_NOCHANGE,ADSTESTSIG_NOCHANGE); break;
      case '-':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_1X,ADSTESTSIG_PULSE_SLOW); break;
      case '+':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_1X,ADSTESTSIG_PULSE_FAST); break;
      case '=':
        //repeat the line above...just for human convenience
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_1X,ADSTESTSIG_PULSE_FAST); break;
      case 'p':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_DCSIG); break;
      case '[':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_PULSE_SLOW); break;
      case ']':
        activateAllChannelsToTestCondition(ADSINPUT_TESTSIG,ADSTESTSIG_AMP_2X,ADSTESTSIG_PULSE_FAST); break;
           
//BIAS GENERATION COMMANDS
      // case '`':
      //   ADSManager.setAutoBiasGeneration(true); break;
      // case '~': 
      //   ADSManager.setAutoBiasGeneration(false); break; 

//OUTPUT SELECT AND FILTER COMMANDS
      case 'n':
        startRunning(OUTPUT_BINARY_WITH_ACCEL);
        useAccel = true;
        startBecauseOfSerial = is_running;
        break;
      case 'b':
        startRunning(OUTPUT_BINARY);
        OBCI.useAccel = false;
        startBecauseOfSerial = is_running;
        break;
      // case 'v':
      //   toggleRunState(OUTPUT_BINARY_4CHAN);
      //   useAccel = false;
      //   startBecauseOfSerial = is_running;
      //   if (is_running) Serial.println(F("OBCI: Starting binary 4-chan..."));
      //   break;
     case 's':
        stopRunning();
        OBCI.useAccel = false;
        startBecauseOfSerial = is_running;
        break;
     // case 'x':
     //    toggleRunState(OUTPUT_BINARY_SYNTHETIC);
     //    useAccel = false;
     //    startBecauseOfSerial = is_running;
     //    if (is_running) Serial.println(F("OBCI: Starting synthetic..."));
     //    break;
     // case 'f':
     //    // useFilters = true;
     //    Serial.println(F("OBCI: enabaling filters"));
     //    break;
     // case 'g':
     //    // useFilters = false;
     //    Serial.println(F("OBCI: disabling filters"));
     //    break;
     case '?':
        //print state of all registers
        printRegisters();
        break;
      default:
        break;
      }
  }// end of while

}// end of serialEvent



boolean stopRunning(void) {
  if(is_running == true){
    OBCI.stopStreaming();                    // stop the data acquisition  //
    is_running = false;
    return is_running;
  }
}

boolean startRunning(int OUT_TYPE) {
  if(is_running == false){
    outputType = OUT_TYPE;
    OBCI.startStreaming();    //start the data acquisition NOT BUILT include accel if needed
    is_running = true;
  }
    return is_running;
}


int changeChannelState_maintainRunningState(int chan, int start)
{
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();
  if (start == true) {
    if(is_running_when_called == false){
      Serial.print(F("Activating channel "));
      Serial.println(chan);
    }
    OBCI.activateChannel(chan,gainCode,inputType);
  } else {
    if(is_running_when_called == false){
      Serial.print(F("Deactivating channel "));
      Serial.println(chan);
    }
    OBCI.deactivateChannel(chan);
  }
  
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

// CALLED WHEN COMMAND CHARACTER IS SEEN ON THE SERIAL PORT
int activateAllChannelsToTestCondition(int testInputCode, byte amplitudeCode, byte freqCode)
{
  boolean is_running_when_called = is_running;
  int cur_outputType = outputType;
  
  //must stop running to change channel settings
  stopRunning();
  //set the test signal to the desired state
  OBCI.configureInternalTestSignal(amplitudeCode,freqCode);    
  //loop over all channels to change their state
  for (int Ichan=1; Ichan <= 8; Ichan++) {
    OBCI.activateChannel(Ichan,gainCode,testInputCode);  //Ichan must be [1 8]...it does not start counting from zero
  }
  //restart, if it was running before
  if (is_running_when_called == true) {
    startRunning(cur_outputType);
  }
}

void printRegisters(){
  if(is_running == false){
    // print the ADS and LIS3DH registers
    OBCI.printAllRegisters();
  }
}




// end

