/*

 Developed by Chip Audette (Nov 2013) for use with OpenBCI
 Processes EEG data on the Arduino itself.
 
 *** WHAT DOES IT DO ***
 
 This sketch sets up the OpenBCI board to use just channel 1.  It then starts the system and continually
 retrieves the EEG data from the OpenBCI board.  It does all the processing on the Arduino itself...
       * Filtering to block DC
       * Filtering to notch out 60Hz noise
       * Bandpass filtering...centered on Alpha waves (or change to Beta or whatever)
       * RMS calculation of the EEG signal amplitude in that frequency band
       * Depending on the RMS of the in-band EEG, light an LED! (pin 5) and sound a buzzer (pin 6)
       
 
 *** DEPENDENCIES ***
 
 This example uses the ADS1299 Arduino Library, a software bridge between the ADS1299 TI chip and 
 Arduino. See http://www.ti.com/product/ads1299 for more information about the device and the README
 folder in the ADS1299 directory for more information about the library.
 
 This example uses a Biquad filter library that I downloaded and modified.  You can get the original
 code at: http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
 
 No warranty.  Use at your own risk.  Use it for whatever you would like.
 
*/
typedef long int int32;

//set the OpenBCI Board
#include <ADS1299Manager.h>
ADS1299Manager ADSManager; //Uses SPI bus and pins to say data is ready.  Uses Pins 13,12,11,10,9,8,4

//define how I'd like my channels setup
#define ADS1299_CHANNELS (8)  //how many channels are in the hardware (must be <= channels supported by ADS1299_Manager)
#define N_EEG_CHANNELS (1)    //how many channels do you actually want to use?  CHANGE THIS TO SUIT YOUR NEEDS
byte gainCode = ADS_GAIN24;         //how much gain do I want  see ADS1299Manager.h for other options
byte inputType = ADSINPUT_NORMAL;   //normal operation.  see ADS1299Manager.h for other options

//define how raw data is sent out over serial
#define OUTPUT_NOTHING (0)
#define OUTPUT_TEXT (1)
#define OUTPUT_BINARY (2)
#define OUTPUT_BINARY_4CHAN (4)
#define OUTPUT_BINARY_OPENEEG (6)
#define OUTPUT_BINARY_OPENEEG_SYNTHETIC (7)
int outputFormat_rawData = OUTPUT_NOTHING; //how to send the raw data back to the PC?
#define SEND_DEBUG (false)   //flip this to 'true' to see CPU and RAM utilization
#define SEND_OTHER_TEXT (false)  //flip this to 'true' to see results of calculations in text form

//Design basic filters  (This BIQUAD class requires ~6K of program space!  Ouch.)
//For frequency response of these filters: http://www.earlevel.com/main/2010/12/20/biquad-calculator/
#include <Biquad_multiChan.h>   //modified from this source code:  http://www.earlevel.com/main/2012/11/26/biquad-c-source-code/
#define SAMPLE_RATE_HZ (250.0)  //default setting for OpenBCI
#define FILTER_Q (0.5)        //critically damped is 0.707 (Butterworth)
#define FILTER_PEAK_GAIN_DB (0.0) //doesn't matter for Lowpass, highpass, notch, or bandpass
#define HP_CUTOFF_HZ (0.5)  //set the desired cutoff for the highpass filter
Biquad_multiChan stopDC_filter(N_EEG_CHANNELS,bq_type_highpass,HP_CUTOFF_HZ / SAMPLE_RATE_HZ, FILTER_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
//Biquad_multiChan stopDC_filter(MAX_N_CHANNELS,bq_type_bandpass,10.0 / SAMPLE_RATE_HZ, 6.0, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
#define NOTCH_FREQ_HZ (60.0)
#define NOTCH_Q (4.0)              //pretty shap notch
Biquad_multiChan notch_filter1(N_EEG_CHANNELS,bq_type_notch,NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan notch_filter2(N_EEG_CHANNELS,bq_type_notch,NOTCH_FREQ_HZ / SAMPLE_RATE_HZ, NOTCH_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states

//Design signal detection filters
#define BP_FREQ_HZ (10.0f)  //focus on Alpha waves
//#define BP_FREQ_HZ (20.0f)  //focus on Beta waves
#define BP_Q (2.0f)         //gives somewhat steeply sloped sides
Biquad_multiChan bandpass_filter1(N_EEG_CHANNELS,bq_type_bandpass,BP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan bandpass_filter2(N_EEG_CHANNELS,bq_type_bandpass,BP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
#define RMS_LP_PERIOD_SEC  (1.0f)  //how long do we want to do the average (seconds)
#define RMS_LP_FREQ_HZ  (1.0f/RMS_LP_PERIOD_SEC)
#define RMS_LP_Q (1.0f)  //keep below 1.0 to avoid a peak at the cuttof frequency
Biquad_multiChan rms_lowpass_filter1(N_EEG_CHANNELS,bq_type_lowpass,RMS_LP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan rms_lowpass_filter2(N_EEG_CHANNELS,bq_type_lowpass,RMS_LP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states

//for guard filtering
#define BP_GUARD_HZ (25.0)
Biquad_multiChan bandpass_filter3(N_EEG_CHANNELS,bq_type_bandpass,BP_GUARD_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan bandpass_filter4(N_EEG_CHANNELS,bq_type_bandpass,BP_GUARD_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan rms_lowpass_filter3(N_EEG_CHANNELS,bq_type_lowpass,RMS_LP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan rms_lowpass_filter4(N_EEG_CHANNELS,bq_type_lowpass,RMS_LP_FREQ_HZ / SAMPLE_RATE_HZ, BP_Q, FILTER_PEAK_GAIN_DB); //one for each channel because the object maintains the filter states
Biquad_multiChan *bp1, *bp2, *lp1, *lp2;


//define some data variables
float EEG_inband_rms[N_EEG_CHANNELS];
float EEG_guard_rms[N_EEG_CHANNELS];
#define MICROVOLTS_PER_COUNT (0.02235174f)  //Nov 10,2013...assumes gain of 24, includes mystery factor of 2... = 4.5/24/(2^24) *  2
//#define MICROVOLTS_PER_COUNT (1.0f)  //don't scale, just operate as counts

//define some output pins
#define LED_OUTPUT_PIN  5     //choose a PWM pin
#define MIN_LED_VAL  (2)      //set the minimum LED brightness
#define BUZZER_OUTPUT_PIN  6  //choose a PWM pin
//#define TONE_MIN_HZ  (400)    //minimum frequency of tone from buzzer
//#define TONE_MAX_HZ  (1200)   //maximum frequency of tone from buzzer
#define TONE_MIN_HZ  (1000)    //minimum frequency of tone from buzzer
#define TONE_MAX_HZ  (3000)   //maximum frequency of tone from buzzer


void setup() {
  //detect which version of OpenBCI we're using (is Pin2 jumped to Pin3?)
  int OpenBCI_version = OPENBCI_V2;  //assume V2
  pinMode(2,INPUT);  digitalWrite(2,HIGH); //activate pullup...for detecting which version of OpenBCI PCB
  pinMode(3,OUTPUT); digitalWrite(3,LOW);  //act as a ground pin...for detecting which version of OpenBCI PCB
  if (digitalRead(2) == LOW) OpenBCI_version = OPENBCI_V1; //check pins to see if there is a jumper.  if so, it is the older board
  ADSManager.initialize(OpenBCI_version);  //must do this VERY early in the setup...preferably first

  // setup the serial link to the PC
  Serial.begin(115200);
  Serial.println(F("ADS1299-Arduino UNO - ProcessDataOnUno")); //read the string from Flash to save RAM
  Serial.print(F("Configured as OpenBCI_Version code = "));Serial.println(OpenBCI_version);
  Serial.flush();
  
  // setup the channels as desired on the ADS1299..set gain, input type, referece (SRB1), and patient bias signal
  for (int chan=1; chan <= ADS1299_CHANNELS; chan++)  ADSManager.deactivateChannel(chan); //deactivate all channels
  for (int chan=1; chan <= N_EEG_CHANNELS; chan++) ADSManager.activateChannel(chan, gainCode, inputType); //activate just the channels I want

  //print state of all registers
  ADSManager.printAllRegisters();Serial.flush();

  //setup other pins
  pinMode(LED_OUTPUT_PIN,OUTPUT);
  analogWrite(LED_OUTPUT_PIN,255);      //initilize to having the LED be ON...so you can tell that it's working
  pinMode(BUZZER_OUTPUT_PIN,OUTPUT);
  tone(BUZZER_OUTPUT_PIN,TONE_MIN_HZ);  //initialize to tone being on at lowest frequency...so you can tell that it's working
  
  
  // pause and then start processing
  delay(1000);
  Serial.print(F("Starting "));Serial.print(N_EEG_CHANNELS);Serial.println(F(" EEG Channels..."));
  ADSManager.start();    //start the data acquisition
 
} // end of setup



long sampleCounter = 0;
unsigned long totalMicrosBusy = 0;  //use this to count time
unsigned long start_micros = 0;
int prev_LED_val = 0, LED_val = 0;
void loop(){
  static boolean printUpdateThisTime = true;
   
  //Is EEG data ready?  watch the DRDY pin.  Stay in this loop until data is available   
  while(!(ADSManager.isDataAvailable())) { delayMicroseconds(50);};
  
  //start of work
  start_micros = micros();

  //get the data
  ADSManager.updateChannelData();          // update the channelData array 
  sampleCounter++;                        // increment my sample counter
  
  //process the data
  //processData();
  processData_wGuard();

  //act on the data
  if ((sampleCounter % (250/10)) == 0) {
    //print some debugging info to the serial port...but at a lower rate
    printUpdateThisTime = false;
    if (SEND_OTHER_TEXT & ((sampleCounter % 250) == 0)) {
      printUpdateThisTime = true;
      //printIntermediateResults();
      printRMSResults();
    }
    
    //light up LED based on (rms-guard) of first EEG channel
    prev_LED_val = LED_val;
    LED_val = updateLED(EEG_inband_rms[0],EEG_guard_rms[0],prev_LED_val,printUpdateThisTime);
    updateBuzzer(LED_val,printUpdateThisTime);
  }
    
  //print the data
  sendRawDataOverSerial();
  
  //assess CPU utilization (put at end of the work section) and RAM
  if (true) {
    totalMicrosBusy += (micros()-start_micros); //accumulate
    if (sampleCounter % 250 == 0) {
      if (SEND_DEBUG) {
        float micros_per_250samples = 1000000.0;
        float CPU_percent = ((float)(totalMicrosBusy))/micros_per_250samples*100.0;
        Serial.print(F("Debug: CPU Utilization = "));
        Serial.print(CPU_percent);Serial.print("%");
        Serial.print(F(", "));
        Serial.print(F("Free RAM = "));
        Serial.println(freeRam());
      }
      totalMicrosBusy = 0; //reset the time accumulator
    }
  }
} // end of loop() function


void processData(void) {
  float val,val2;
  
  //loop over each channel
  for (int Ichan=0; Ichan < N_EEG_CHANNELS; Ichan++) {
    //get the EEG data for this channel, convert to float
    val=(float)ADSManager.channelData[Ichan]; 
  
    //apply DC-blocking highpass filter
    val = stopDC_filter.process(val,Ichan);    //apply DC-blocking filter
    
    //apply 60Hz notch filter...twice to make it more effective
    val = notch_filter1.process(val,Ichan);     //apply 60Hz notch filter
    val = notch_filter2.process(val,Ichan);     //apply 60Hz notch again
    
    //put this data back into ADSManager in case we want to output i tusing the ADSManager buit-in output functions
    //ADSManager.channelData[Ichan] = (long)val; //
    ADSManager.channelData[Ichan+2] = (long)val; //
    
    //apply bandpass filter to focus on bandwidth of interest
    val = bandpass_filter1.process(val,Ichan);    //apply bandpass filter
    val = bandpass_filter2.process(val,Ichan);    //do it again to make it even tighter
    
    //put this value back into the ADSManager, too.  utilize upper channels, if available
    if (N_EEG_CHANNELS <= 4) ADSManager.channelData[Ichan+4] = (long)val;
    //if (N_EEG_CHANNELS <= 4) ADSManager.channelData[Ichan] = (long)val; //no, put it on top of the raw data
    
    //return to processing...get the RMS value by squaring, lowpass filtering (a type of average), and sqrt
    val2 = val*val;  ///square the data
    val2 = rms_lowpass_filter1.process(val2,Ichan);  //low pass filter to take the "mean"
    //val2 = rms_lowpass_filter2.process(val2,Ichan);  //do it again for smoother mean    
    EEG_inband_rms[Ichan] = sqrt(abs(val2));  //take the square root to finally get the RMS
    
    //put this value back into the ADSManager, too
    if (N_EEG_CHANNELS <= 2) ADSManager.channelData[Ichan+6] = (long)EEG_inband_rms[Ichan];
  }
}

void processData_wGuard(void) {
  float val,val2;
  
  //loop over each channel
  for (int Ichan=0; Ichan < N_EEG_CHANNELS; Ichan++) {
    //get the EEG data for this channel, convert to float
    val=(float)ADSManager.channelData[Ichan]; 
  
    //apply DC-blocking highpass filter
    val = stopDC_filter.process(val,Ichan);    //apply DC-blocking filter
    
    //apply 60Hz notch filter...twice to make it more effective
    val = notch_filter1.process(val,Ichan);     //apply 60Hz notch filter
    val = notch_filter2.process(val,Ichan);     //apply 60Hz notch again
    
    //put this data back into ADSManager in case we want to output i tusing the ADSManager buit-in output functions
    //ADSManager.channelData[Ichan] = (long)val; //
    ADSManager.channelData[Ichan+2] = (long)val; //
    
    float val_common = val;
    for (int Iband=0;Iband<2;Iband++) {
      val=val_common;
      switch (Iband) {
        case (0):
          bp1 = &bandpass_filter1;
          bp2 = &bandpass_filter2;
          lp1 = &rms_lowpass_filter1;
          lp2 = &rms_lowpass_filter2; 
          break;
       case (1):       
          bp1 = &bandpass_filter3;
          bp2 = &bandpass_filter4;
          lp1 = &rms_lowpass_filter3;
          lp2 = &rms_lowpass_filter4; 
          break;
      }   
      
      //apply bandpass filter to focus on bandwidth of interest
      val = bp1->process(val,Ichan);    //apply bandpass filter
      val = bp2->process(val,Ichan);    //do it again to make it even tighter
      
      //put this value back into the ADSManager, too.  utilize upper channels, if available
      if ((N_EEG_CHANNELS==1) | (Iband==0)) { if (N_EEG_CHANNELS <= 4) ADSManager.channelData[Ichan+4+Iband] = (long)val;}
      //if (N_EEG_CHANNELS <= 4) ADSManager.channelData[Ichan] = (long)val; //no, put it on top of the raw data
      
      //return to processing...get the RMS value by squaring, lowpass filtering (a type of average), and sqrt
      val2 = val*val;  ///square the data
      val2 = lp1->process(val2,Ichan);  //low pass filter to take the "mean"
      //val2 = lp2->process(val2,Ichan);  //do it again for smoother mean    
      float EEG_rms = sqrt(abs(val2));  //take the square root to finally get the RMS
  
      switch (Iband) {
        case (0):
          EEG_inband_rms[Ichan] = EEG_rms;
          break;
        case (1):
          EEG_guard_rms[Ichan] = EEG_rms;
          break;
      }
    
      //put this value back into the ADSManager, too
      if ((N_EEG_CHANNELS==1) | (Iband==0)) if (N_EEG_CHANNELS <= 2) ADSManager.channelData[Ichan+6+Iband] = (long)EEG_rms;
    }
  }
}

int freeRam() 
{
  extern int __heap_start, *__brkval; 
  int v; 
  return (int) &v - (__brkval == 0 ? (int) &__heap_start : (int) __brkval); 
}

int sendRawDataOverSerial(void) {
  switch (outputFormat_rawData) {
    case OUTPUT_NOTHING:
      //don't output anything...the Arduino is still collecting data from the OpenBCI board...just nothing is being done with it
      //if ((sampleCounter % 250) == 1) { Serial.print(F("Free RAM = ")); Serial.println(freeRam()); }; //print memory status
      break;
    case OUTPUT_BINARY:
      ADSManager.writeChannelDataAsBinary(8,sampleCounter);  //print all channels, whether active or not
      break;
    case OUTPUT_BINARY_4CHAN:
      ADSManager.writeChannelDataAsBinary(4,sampleCounter);  //print all channels, whether active or not
      break; 
    case OUTPUT_BINARY_OPENEEG:
      ADSManager.writeChannelDataAsOpenEEG_P2(sampleCounter);  //print all channels, whether active or not
      break; 
    case OUTPUT_BINARY_OPENEEG_SYNTHETIC:
      ADSManager.writeChannelDataAsOpenEEG_P2(sampleCounter,true);  //print all channels, whether active or not
      break;           
    default:
      ADSManager.printChannelDataAsText(8,sampleCounter);  //print all channels, whether active or not
  }
}


void printIntermediateResults(void) {
  for (int Idata=0; Idata<3; Idata++) {
    Serial.print(F("TEXT: EEG "));
    switch (Idata) {
     case 0:
        Serial.print(F(" Raw = ["));
        break;
     case 1:
        Serial.print(F(" DC-Block, 60Hz Notched = ["));
        break;
     case 2:
        Serial.print(F(" BP Filtered = ["));
        break;
    }
    for (int Ichan=0; Ichan < N_EEG_CHANNELS; Ichan++) {
      Serial.print( ADSManager.channelData[Ichan+(Idata*2)]*MICROVOLTS_PER_COUNT);
      if (Ichan < N_EEG_CHANNELS-1) Serial.print(", ");
    }
    Serial.println("] uV");
  }
}

void printRMSResults(void) {
  Serial.print(F("TEXT: Signal RMS Around "));
  Serial.print(BP_FREQ_HZ);
  Serial.print(F("Hz = ["));
  for (int Ichan=0; Ichan < N_EEG_CHANNELS; Ichan++) {
    Serial.print(EEG_inband_rms[Ichan]*MICROVOLTS_PER_COUNT);
    if (Ichan < N_EEG_CHANNELS-1) Serial.print(", ");
  }
  Serial.print("] uV");
  
  Serial.print(F(", Guard RMS = ["));
  for (int Ichan=0; Ichan < N_EEG_CHANNELS; Ichan++) {
    Serial.print(EEG_guard_rms[Ichan]*MICROVOLTS_PER_COUNT);
    if (Ichan < N_EEG_CHANNELS-1) Serial.print(", ");
  }
  Serial.print(F("] uV"));
  
  Serial.println();
}

// Decide how bright to make the LED.
// We want to illuminate the LED when the inband energy is high, but ONLY the inband energy.
// To exclude cases where all energy (inband and out-of-band) is high, we look at energy in
// near-by frequencies (the "guard" frequencies).  If the energy in the guard frequencies
// is similar to the in-band, do not light the LED
//
// Note: EEG_inband_rms and EEG_guard_rms are floats, but they're in "counts" not in uV
float prev_eeg = 0.0f;
int updateLED(const float &EEG_inband_rms, const float &EEG_guard_rms, const int &prev_LED_val, const boolean &printThisTime) {
  static const float smooth_EEG_fac = 0.95f;  //should be [0 1.0]...bigger is more smoothing
  //static float prev_val = 0.0f;
  const float max_uV_10x = 12.0f*10.0f; //this is the EEG amplitude (times 10!) causing maximum LED intensity
  const float min_uV_10x = 2.0f*10.0f;  //this is the EEG amplitude (times 10!) required for minimum LED instensity
  

    
  //here is how it decides how bright to make the LED...this is ugly...
  //there are lots of approaches that one can do to get the LED brightness
  //to look right in response to your brain waves.  Please make this better!
//  if (EEG_guard_rms*MICROVOLTS_PER_COUNT > 10.0f) { 
//    //use these brightness rules when the guard EEG is larger than 1 uV
//    float eeg_ratio_10x = 10.0f*EEG_inband_rms / EEG_guard_rms;
//    float min_ratio_10x = min_uV_10x;
//    eeg_ratio_10x = constrain(eeg_ratio_10x, min_ratio_10x,max_uV_10x);
//    eeg_ratio_10x = smooth_EEG_fac * prev_val + (1.0f-smooth_EEG_fac)*eeg_ratio_10x;
//    prev_val = eeg_ratio_10x;
//    
//    LED_val = map(eeg_ratio_10x,min_ratio_10x,max_uV_10x,MIN_LED_VAL,255);
//  } else {
    
    float eeg_inband_uV = EEG_inband_rms * MICROVOLTS_PER_COUNT;
    float eeg_guard_uV = EEG_guard_rms * MICROVOLTS_PER_COUNT;
    float eeg_uV;
    if (eeg_guard_uV >10.0f) {
      eeg_uV = 0.0f;
    } else {
    //use these brightness rules when the guard EEG amplitude is smaller
      eeg_uV = eeg_inband_uV - eeg_guard_uV;
      eeg_uV -= 0.5f;  //penalize it a bit to reduce false alarms
      //eeg_uV = constrain(eeg_uV,-5.0f,+5.0f);
    }
    eeg_uV = smooth_EEG_fac * prev_eeg + (1.0f - smooth_EEG_fac)*eeg_uV;
    prev_eeg = eeg_uV;
    
    
    //Serial.print("EEG_inband: ");Serial.print(eeg_inband_uV);
    Serial.print("eeg: ");Serial.println(eeg_uV);
    
    int int_val = (constrain(eeg_uV*10.0f,min_uV_10x,max_uV_10x));
    LED_val=(int)(map(int_val,(int)(min_uV_10x),(int)(max_uV_10x),MIN_LED_VAL,255));
// }
  LED_val = constrain(LED_val,MIN_LED_VAL,255);
  //LED_val = (3*prev_LED_val + LED_val)/4;       //smooth the LED brightness in time
  analogWrite(LED_OUTPUT_PIN,(byte)constrain(LED_val,MIN_LED_VAL,255));  //finally, issue the LED command
  if (SEND_OTHER_TEXT && printThisTime) { Serial.print(F("LED Value = "));Serial.println(LED_val);} //a little text output for debugging
  //prev_LED_val = LED_val;
  return LED_val;
}
    
//update the buzzer sound.  Make the pitch proportional to the strength of the desired signal
//Note: all the processing to determine the "strength of the desired signal" by the LED routine
//      so we'll just re-use the value that it produced
//Note: "val" is some number [0 255] that had been used to set the LED
int updateBuzzer(const int &val, const boolean &printThisTime) {
  //is the value big enough to warrent a tone?
  static const int Thresh = max(MIN_LED_VAL,20); //set the threshold based on how the LED was being set
  if (val > Thresh){
    //yes, it is big enough.  Compute the desired frequency and do the tone
    int freq_Hz = int(map(val,Thresh,255,TONE_MIN_HZ,TONE_MAX_HZ)); //pick the frequency of the tone
    tone(BUZZER_OUTPUT_PIN,freq_Hz); //set the buzzer to the desired tone
    if (SEND_OTHER_TEXT && printThisTime) { Serial.print(F("Buzzer Freq = "));Serial.println(freq_Hz);} //a little text output for debugging
  } else {
    //no it is not big enough.  Stop the tone
    noTone(BUZZER_OUTPUT_PIN);  // otherwise this can get annoying.
    if (SEND_OTHER_TEXT && printThisTime) { Serial.println(F("Buzzer Freq = OFF")); } //a little text output for debugging
  }
  
}
