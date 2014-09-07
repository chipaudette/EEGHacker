
/*
  MCP41XXX / MCP42XXX Dual Digital Poteniometer with SPI Interface.  
    * 8-Bit Potentiometer
    * 41XXX is one channel, 42XXX is two channel and can be daisy chained
    * Relies upon the Arduino SPI library
  
  Created: Chip Audette, Sept 2014
  License: MIT
*/


#include "Arduino.h"
#include "SPI.h"
  
#ifndef MCP42XXX_h
#define MCP42XXX_h

class MCP42XXX {
  public:
    MCP42XXX(int SSpin,boolean start_SPI_bus,int chanPerDevice); 
    void setValues(byte values[], int n_values); //last device first, chan 0 then chan 1
    
  private:
    int SS_pin;
    int chan_per_device;
};

#endif
