

#ifndef _____OpenBCI_04__
#define _____OpenBCI_04__

#include <SdFat.h>
#include <SdFatUtil.h>

#include "ADS1299.h"
#include "LIS3DH.h"
#include "Definitions.h"

class OpenBCI {

  public:
  
    ADS1299 ads;
    LIS3DH accel;
    SdFat card;
  
    // VARIABLES
    long channelData[9];
  
  
  
    //  ADS1299 FUNCITONS
    void initialize_ads(void);
    void printAllRegisters(void);  
    void activateChannel(byte,byte,byte);
    void start_ads(void);
    void stop_ads(void);
    void reset_ads(void);
    boolean isDataAvailable(void);
    void updateChannelData(void);
    void setSRB1(boolean);
    //void printChannelDataAsText(int N, long int sampleNumber);
    //void writeChannelDataAsBinary(int N, uint16_t sampleNumber);
    //void writeChannelDataAsOpenEEG_P2(long int sampleNumber);
    //void setSRB1(boolean desired_state);
    void getADS_ID(void);
    
    //  ACCELEROMETER FUNCIONS
    void initialize_accel(void);
    void disable_accel(void);
    void enable_accel(void);
    byte getAccelID(void);
    boolean LIS3DH_DataReady(void);
    int getX(void);
    int getY(void);
    int getZ(void);
    
    
    
};

#endif
