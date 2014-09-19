/*
 * 
 *  >>>> THIS CODE USED TO STREAM OBCI V3 DATA TO DONGLE <<<<
 *  >>>> Includes the 60Hz notch filter by Chip Audette  <<<<
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
#define N_CHANNELS_PER_OPENBCI (8)  //number of channels on a single OpenBCI board
#define MAX_N_CHANNELS (N_CHANNELS_PER_OPENBCI)   //how many channels are available in hardware
//#define MAX_N_CHANNELS (2*N_CHANNELS_PER_OPENBCI)   //how many channels are available in hardware...use this for daisy-chained board
int nActiveChannels = MAX_N_CHANNELS;   //how many active channels would I like?
OpenBCI OBCI; //Uses SPI bus and pins to say data is ready.  Uses Pins 13,12,11,10,9,8,4
// #define MAX_N_CHANNELS (8)  //must be less than or equal to length of channelData in ADS1299 object!!
//int nActiveChannels = 8;   //how many active channels would I like?
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
//Design filters  (This BIQUAD class requires ~6K of program space!  Ouch.)
//For frequency response of these filters: http://www.earlevel.com/main/2010/12/20/biquad-calculator/
#include <Biquad_multiChan.h>   //modified from this source code:  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
#define SAMPLE_RATE_HZ (250.0)  //default setting for OpenBCI
#define FILTER_Q (0.5)        //critically damped is 0.707 (Butterworth)
#define FILTER_PEAK_GAIN_DB (0.0) //we don't want any gain in the passband
#define HP_CUTOFF_HZ (0.5)  //set the desired cutoff for the highpass filter
Biquad_multiChan stopDC_filter(MAX_N_CHANNELS,bq_type_highpass,HP_CUTOFF_HZ / SAMPLE_RATE_HZ, FILTER_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
//Biquad_multiChan stopDC_filter(MAX_N_CHANNELS,bq_type_bandpass,10.0 / SAMPLE_RATE_HZ, 6.0, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
#define NOTCH_FREQ_HZ (60.0)
#define NOTCH_Q (4.0)              //pretty sharp notch
#define NOTCH_PEAK_GAIN_DB (0.0)  //doesn't matter for this filter type
Biquad_multiChan notch_filter1(MAX_N_CHANNELS,bq_type_notch,NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, NOTCH_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan notch_filter2(MAX_N_CHANNELS,bq_type_notch,NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, NOTCH_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
boolean useFilters = false;  //enable or disable as you'd like...turn off if you're daisy chaining!
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
      if (useFilters) applyFilters();
      OBCI.writeDataToDongle(sampleCounter);
      sampleCounter++;
  
  }

} // end of loop



#define ACTIVATE_SHORTED (2)
#define ACTIVATE (1)
#define DEACTIVATE (0)
int plusCounter = 0;
char testChar;
unsigned long commandTimer;

void serialEvent(){
  while(Serial.available()){      
    char inChar = (char)Serial.read();
    
    if(plusCounter == 1){
    testChar = inChar;
    plusCounter++;
      commandTimer = millis();
    }
  
    if(inChar == '+'){
      plusCounter++;
      if(plusCounter == 3){
        if(millis() - commandTimer < 10){
          getCommand(testChar);
        }
        plusCounter = 0;
      }
    }
  }
}
    
    
void getCommand(char token){
    switch (token){
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
      case '=':
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
      case 'f':
          useFilters = true;
//         Serial.println(F("OBCI: enabaling filters"));
         break;
      case 'g':
          useFilters = false;
//         Serial.println(F("OBCI: disabling filters"));
         break;
     case '?':
        //print state of all registers
        printRegisters();
        break;
      default:
        break;
      }
  }// end of getCommand



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


long int runningAve[MAX_N_CHANNELS];
int applyFilters(void) {
  //scale factor for these coefficients was 32768 = 2^15
  const static long int a0 = 32360L; //16 bit shift?
  const static long int a1 = -2L*a0;
  const static long int a2 = a0;
  const static long int b1 = -64718L; //this is a shift of 17 bits!
  const static long int b2 = 31955L;
  static long int z1[MAX_N_CHANNELS], z2[MAX_N_CHANNELS];
  long int val_int, val_in_down9, val_out, val_out_down9;
  float val;
  for (int Ichan=0; Ichan < MAX_N_CHANNELS; Ichan++) {
    switch (1) {
      case 1:
        //use BiQuad
        val = (float) OBCI.getChannel(Ichan); //get the stored value for this sample
        val = stopDC_filter.process(val,Ichan);    //apply DC-blocking filter
        break;
      case 2:
        //do fixed point, 1st order running ave
        val_int = OBCI.getChannel(Ichan); //get the stored value for this sample
        //runningAve[Ichan]=( ((512-1)*(runningAve[Ichan]>>2)) + (val_int>>2) )>>7;  // fs/0.5Hz = ~512 points..9 bits
        //runningAve[Ichan]=( ((256-1)*(runningAve[Ichan]>>2)) + (val_int>>2) )>>6;  // fs/1.0Hz = ~256 points...8 bits
        runningAve[Ichan]=( ((128-1)*(runningAve[Ichan]>>1)) + (val_int>>1) )>>6;  // fs/2.0Hz = ~128 points...7 bits
        val = (float)(val_int - runningAve[Ichan]);  //remove the DC
        break;
//      case 3:
//        val_in_down9 = ADSManager.channelData[Ichan] >> 9; //get the stored value for this sample...bring 24-bit value down to 16-bit
//        val_out = (val_in_down9 * a0  + (z1[Ichan]>>9)) >> (16-9);  //8bits were already removed...results in 24-bit value
//        val_out_down9 = val_out >> 9;  //remove eight bits to go from 24-bit down to 16 bit
//        z1[Ichan] = (val_in_down9 * a1 + (z2[Ichan] >> 9) - b1 * val_out_down9  ) >> (16-9);  //8-bits were pre-removed..end in 24 bit number
//        z2[Ichan] = (val_in_down9 * a2  - b2 * val_out_down9) >> (16-9); //8-bits were pre-removed...end in 24-bit number
//        val = (float)val_out;
//        break;
    }
    val = notch_filter1.process(val,Ichan);     //apply 60Hz notch filter
    val = notch_filter2.process(val,Ichan);     //apply it again
    OBCI.putChannel(Ichan, (long) val);  //save the value back into the main data-holding object
  }
  OBCI.update24bitData();
  return 0;
}

// end

