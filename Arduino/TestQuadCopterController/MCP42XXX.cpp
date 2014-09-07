
#include "MCP42XXX.h"


MCP42XXX::MCP42XXX(int SSpin,boolean start_SPI_bus, int chanPerDevice) {
  chan_per_device= chanPerDevice;
  
  //configure the slave select pin on the Arduino (ie, the pin that connects to *CS)
  SS_pin = SSpin;
  digitalWrite(SS_pin,HIGH);
  pinMode(SS_pin, OUTPUT);

  //start the SPI bus
  if (start_SPI_bus) {
    SPI.begin();
    SPI.setDataMode(SPI_MODE0);
    SPI.setBitOrder(MSBFIRST);
  }
}

//Assume that the values are chan 0 then chan 1.
//If more values are included assume it is for additional daisy-chained devices.
//Assume the values for the last device are first and the values for the first deivce are last
void MCP42XXX::setValues(byte values[], int n_values) {  
  
  //prepare command byte
  byte command;
  int ind;
  int n_devices = n_values / chan_per_device;
  for (int Ichan = 0; Ichan < chan_per_device; Ichan++) {
    //set chip select low to begin data transfer for this set of channels
//    Serial.println(F("MCP42XXX: setting SS_pin LOW"));
    digitalWrite(SS_pin,LOW);
    delayMicroseconds(10);
    
    for (int Idevice = 0; Idevice < n_devices; Idevice++) {
      
      command = 0b00010000; //write data
      if (Ichan==0) {
        //potentiometer 0
        command = bitSet(command,0);
      } else {
        //potentiometer 1
        command = bitSet(command,1);
      }
    
      //transmit data
      ind = (Idevice*chan_per_device)+Ichan;
      if (ind < n_values) {
//        Serial.print("MCP42XXX: Transfering ");
//        Serial.print(command,BIN);
//        Serial.print(" ");
//        Serial.println(values[ind]);
        SPI.transfer(command);
        SPI.transfer(values[ind]);
      }
    }
  
    //set chip select hight to close the data transfer
//    Serial.println(F("MCP42XXX: setting SS_pin HIGH"));
    digitalWrite(SS_pin,HIGH);
    delayMicroseconds(10);
  }
}

