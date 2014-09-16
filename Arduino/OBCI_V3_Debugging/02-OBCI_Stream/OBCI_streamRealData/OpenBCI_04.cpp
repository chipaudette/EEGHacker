

#include "OpenBCI_04.h"

void OpenBCI::initialize_ads(void) {
  ads.initialize(PIN_DRDY, PIN_RST, ADS_SS);  // adjust here to test daisy module!
}

void OpenBCI::initialize_accel(void) {
  accel.initialize();
}

void OpenBCI::enable_accel(void) {
  accel.enable_accel();
}

void OpenBCI::disable_accel(void){
  accel.disable_accel();
}

byte OpenBCI::getAccelID(void){
  return accel.getDeviceID();
}

boolean OpenBCI::LIS3DH_DataReady(void){
  return (accel.LIS3DH_DataReady());
}


int OpenBCI::getX(void){ return accel.getX(); }
int OpenBCI::getY(void){ return accel.getY(); }
int OpenBCI::getZ(void){ return accel.getZ(); }



// ADS FUNCTIONS
void OpenBCI::printAllRegisters(void) {
  ads.printAllRegisters();
  delay(100);
  Serial.println("LIS3DH Registers:");
  delay(100);
  accel.readAllRegs();
}

void OpenBCI::getADS_ID(void){
  ads.getDeviceID();
}

void OpenBCI::activateChannel(byte chan, byte gainCode, byte inputType){
  ads.activateChannel(chan, gainCode, inputType);
}

void OpenBCI::start_ads(void){
  ads.startADS();
}

void OpenBCI::stop_ads(void){
  ads.stopADS();
}

void OpenBCI::reset_ads(void){
  ads.resetADS();
}

boolean OpenBCI::isDataAvailable(void){
  return ads.isDataAvailable();
}

void OpenBCI::updateChannelData(void){
  ads.updateChannelData();
//  channelData = ads.channelData;
}

void OpenBCI::setSRB1(boolean desired_state){
  ads.setSRB1(desired_state);
}


