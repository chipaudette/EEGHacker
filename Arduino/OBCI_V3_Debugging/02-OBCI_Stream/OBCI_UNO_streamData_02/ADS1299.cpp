

#include "ADS1299.h"

void ADS1299::initialize(int _DRDY, int _RST, int _CS){
    DRDY = _DRDY;
    CS = _CS;
    RST = _RST;
// recommended power up sequence requiers Tpor (~32mS)			
    delay(50);				
    pinMode(RST,OUTPUT);
    digitalWrite(RST,LOW);
    delayMicroseconds(4);	// toggle reset pin
    digitalWrite(RST,HIGH);
    delayMicroseconds(20);	// recommended to wait 18 Tclk before using device (~8uS);    
// initalize the  data ready chip select and reset pins:
    pinMode(DRDY, INPUT);
    pinMode(CS, OUTPUT); digitalWrite(CS,HIGH); 	
    pinMode(SD_SS,OUTPUT); digitalWrite(SD_SS,HIGH);  // de-select the SD card if it's there
    delay(100);
	
    resetADS();
    
    use_N_inputs = true;  // switch this to use N or P channels and the corresponding SRB pin
    for (int i=0; i < OPENBCI_NCHAN; i++) {
      use_SRB2[i] = false;
    }
    
    verbosity = false;      // when verbosity is true, there will be Serial feedback
};


//System Commands
void ADS1299::WAKEUP() {
    csLow(); 
    xfer(_WAKEUP);
    csHigh(); 
    delayMicroseconds(3);  		//must wait 4 tCLK cycles before sending another command (Datasheet, pg. 35)
}

void ADS1299::STANDBY() {		// only allowed to send WAKEUP after sending STANDBY
    csLow();
    xfer(_STANDBY);
    csHigh();
}

void ADS1299::RESET() {			// reset all the registers to default settings
    csLow();
    xfer(_RESET);
    delayMicroseconds(12);   	//must wait 18 tCLK cycles to execute this command (Datasheet, pg. 35)
    csHigh();
}

void ADS1299::START() {			//start data conversion 
    csLow();
    xfer(_START);
    csHigh();
}

void ADS1299::STOP() {			//stop data conversion
    csLow();
    xfer(_STOP);
    csHigh();
}

void ADS1299::RDATAC() {
    csLow();
    xfer(_RDATAC);
    csHigh();
	delayMicroseconds(3);   
}
void ADS1299::SDATAC() {
    csLow();
    xfer(_SDATAC);
    csHigh();
	delayMicroseconds(3);   //must wait 4 tCLK cycles after executing this command (Datasheet, pg. 37)
}

// Register Read/Write Commands
byte ADS1299::getDeviceID() {			// simple hello world com check
	byte data = RREG(0x00);
	if(verbosity){						// verbosity otuput
		Serial.print("Device ID ");
		printHex(data);	
        Serial.println();
	}
	return data;
}

byte ADS1299::RREG(byte _address) {		//  reads ONE register at _address
    byte opcode1 = _address + 0x20; 	//  RREG expects 001rrrrr where rrrrr = _address
    csLow(); 				//  open SPI
    xfer(opcode1); 					//  opcode1
    xfer(0x00); 					//  opcode2
    regData[_address] = xfer(0x00);//  update mirror location with returned byte
    csHigh(); 			//  close SPI	
	if (verbosity){						//  verbosity output
		printRegisterName(_address);
		printHex(_address);
		Serial.print(", ");
		printHex(regData[_address]);
		Serial.print(", ");
		for(byte j = 0; j<8; j++){
			Serial.print(bitRead(regData[_address], 7-j));
			if(j!=7) Serial.print(", ");
		}
		
		Serial.println();
	}
	return regData[_address];			// return requested register value
}

// Read more than one register starting at _address
void ADS1299::RREGS(byte _address, byte _numRegistersMinusOne) {
//	for(byte i = 0; i < 0x17; i++){
//		regData[i] = 0;					//  reset the regData array
//	}
    byte opcode1 = _address + 0x20; 	//  RREG expects 001rrrrr where rrrrr = _address
    csLow(); 				//  open SPI
    xfer(opcode1); 					//  opcode1
    xfer(_numRegistersMinusOne);	//  opcode2
    for(int i = 0; i <= _numRegistersMinusOne; i++){
        regData[_address + i] = xfer(0x00); 	//  add register byte to mirror array
		}
    csHigh(); 			//  close SPI
	if(verbosity){						//  verbosity output
		for(int i = 0; i<= _numRegistersMinusOne; i++){
			printRegisterName(_address + i);
			printHex(_address + i);
			Serial.print(", ");
			printHex(regData[_address + i]);
			Serial.print(", ");
			for(int j = 0; j<8; j++){
				Serial.print(bitRead(regData[_address + i], 7-j));
				if(j!=7) Serial.print(", ");
			}
			Serial.println();
			delay(30);
		}
    }
}

void ADS1299::WREG(byte _address, byte _value) {	//  Write ONE register at _address
    byte opcode1 = _address + 0x40; 	//  WREG expects 010rrrrr where rrrrr = _address
    csLow(); 				//  open SPI
    xfer(opcode1);					//  Send WREG command & address
    xfer(0x00);						//	Send number of registers to read -1
    xfer(_value);					//  Write the value to the register
    csHigh(); 			//  close SPI
	regData[_address] = _value;			//  update the mirror array
	if(verbosity){						//  verbosity output
		Serial.print("Register ");
		printHex(_address);
		Serial.println(" modified.");
	}
}

void ADS1299::WREGS(byte _address, byte _numRegistersMinusOne) {
    byte opcode1 = _address + 0x40;		//  WREG expects 010rrrrr where rrrrr = _address
    csLow(); 				//  open SPI
    xfer(opcode1);					//  Send WREG command & address
    xfer(_numRegistersMinusOne);	//	Send number of registers to read -1	
	for (int i=_address; i <=(_address + _numRegistersMinusOne); i++){
		xfer(regData[i]);			//  Write to the registers
	}	
	digitalWrite(CS,HIGH);				//  close SPI
	if(verbosity){
		Serial.print("Registers ");
		printHex(_address); Serial.print(" to ");
		printHex(_address + _numRegistersMinusOne);
		Serial.println(" modified");
	}
}

void ADS1299::updateChannelData(){
	byte inByte;
  int byteCounter = 0;
	csLow();				//  open SPI
    for(int i=0; i<3; i++){
	     inByte = xfer(0x00);		        //  read status register (1100 + LOFF_STATP + LOFF_STATN + GPIO[7:4])
	     stat = (stat << 8) | inByte;
    }
    for(int i = 0; i<8; i++){
  	  for(int j=0; j<3; j++){		//  read 24 bits of channel data in 8 3 byte chunks
  	    inByte = xfer(0x00);
        rawChannelData[byteCounter] = inByte;
        byteCounter++;
  	    channelData[i] = (channelData[i]<<8) | inByte;
  	  }
	  }
	csHigh();				//  close SPI
	
	for(int i=0; i<8; i++){			// convert 3 byte 2's compliment to 4 byte 2's compliment	
		if(bitRead(channelData[i],23) == 1){	
			channelData[i] |= 0xFF000000;
		}else{
			channelData[i] &= 0x00FFFFFF;
		}
	}

}

void ADS1299::RDATA() {					//  use in Stop Read Continuous mode when DRDY goes low
	byte inByte;						//  to read in one sample of the channels
    csLow();				//  open SPI
    xfer(_RDATA);					//  send the RDATA command
	stat = xfer(0x00);				//  read status register (1100 + LOFF_STATP + LOFF_STATN + GPIO[7:4])
	for(int i = 0; i<8; i++){
		for(int j=0; j<3; j++){		//  read in the status register and new channel data
			inByte = xfer(0x00);
			channelData[i] = (channelData[i]<<8) | inByte;
		}
	}
	csHigh();				//  close SPI
	
	for(int i=0; i<8; i++){
		if(bitRead(channelData[i],23) == 1){	// convert 3 byte 2's compliment to 4 byte 2's compliment
			channelData[i] |= 0xFF000000;
		}else{
			channelData[i] &= 0x00FFFFFF;
		}
	}
}

// String-Byte converters for RREG and WREG
void ADS1299::printRegisterName(byte _address) {
    if(_address == ID){
        Serial.print("ID, ");
    }
    else if(_address == CONFIG1){
        Serial.print("CONFIG1, ");
    }
    else if(_address == CONFIG2){
        Serial.print("CONFIG2, ");
    }
    else if(_address == CONFIG3){
        Serial.print("CONFIG3, ");
    }
    else if(_address == LOFF){
        Serial.print("LOFF, ");
    }
    else if(_address == CH1SET){
        Serial.print("CH1SET, ");
    }
    else if(_address == CH2SET){
        Serial.print("CH2SET, ");
    }
    else if(_address == CH3SET){
        Serial.print("CH3SET, ");
    }
    else if(_address == CH4SET){
        Serial.print("CH4SET, ");
    }
    else if(_address == CH5SET){
        Serial.print("CH5SET, ");
    }
    else if(_address == CH6SET){
        Serial.print("CH6SET, ");
    }
    else if(_address == CH7SET){
        Serial.print("CH7SET, ");
    }
    else if(_address == CH8SET){
        Serial.print("CH8SET, ");
    }
    else if(_address == BIAS_SENSP){
        Serial.print("BIAS_SENSP, ");
    }
    else if(_address == BIAS_SENSN){
        Serial.print("BIAS_SENSN, ");
    }
    else if(_address == LOFF_SENSP){
        Serial.print("LOFF_SENSP, ");
    }
    else if(_address == LOFF_SENSN){
        Serial.print("LOFF_SENSN, ");
    }
    else if(_address == LOFF_FLIP){
        Serial.print("LOFF_FLIP, ");
    }
    else if(_address == LOFF_STATP){
        Serial.print("LOFF_STATP, ");
    }
    else if(_address == LOFF_STATN){
        Serial.print("LOFF_STATN, ");
    }
    else if(_address == GPIO){
        Serial.print("GPIO, ");
    }
    else if(_address == MISC1){
        Serial.print("MISC1, ");
    }
    else if(_address == MISC2){
        Serial.print("MISC2, ");
    }
    else if(_address == CONFIG4){
        Serial.print("CONFIG4, ");
    }
}

//SPI communication methods
byte ADS1299::xfer(byte _data) {
	cli();
    SPDR = _data;
    while (!(SPSR & _BV(SPIF)))
        ;
	sei();
    return SPDR;
}

// Used for printing HEX in verbosity feedback mode
void ADS1299::printHex(byte _data){
	Serial.print("0x");
    if(_data < 0x10) Serial.print("0");
    Serial.print(_data, HEX);
}

//reset all the ADS1299's settings.  Call however you'd like.  Stops all data acquisition
void ADS1299::resetADS(void)
{
  RESET();             // send RESET command to default all registers
  SDATAC();            // exit Read Data Continuous mode to communicate with ADS
  
  delay(100);
    
  // turn off all channels
  for (int chan=1; chan <= 8; chan++) {
    deactivateChannel(chan);
  }
  
  setSRB1(use_SRB1());  //set whether SRB1 is active or not
  
  
};


//  deactivate the given channel...note: stops data colleciton to issue its commands
//  N is the channel number: 1-8
// 
void ADS1299::deactivateChannel(int N)
{
  byte reg, config;
  
  //check the inputs
  if ((N < 1) || (N > 8)) return;
  
  //proceed...first, disable any data collection
  SDATAC(); delay(1);      // exit Read Data Continuous mode to communicate with ADS

  //shut down the channel
  N = constrain(N-1,0,7);  //subtracts 1 so that we're counting from 0, not 1
  reg = CH1SET+(byte)N;						// select the current channel
  config = RREG(reg); delay(1);	// get the current channel settings
  bitSet(config,7);  						// set bit7 to shut down channel
  if (use_N_inputs) bitClear(config,3);  	// clear bit3 to disclude from SRB2 if used
  WREG(reg,config); delay(1);	    // write the new value to disable the channel
  
  //remove the channel from the bias generation...
  reg = BIAS_SENSP; 					 	// set up to disconnect the P inputs from Bias generation
  config = RREG(reg); delay(1);	//get the current bias settings
  bitClear(config,N);          				//clear this channel's bit to remove from bias generation
  WREG(reg,config); delay(1); 	//send the modified byte back to the ADS

  reg = BIAS_SENSN; 						// set up to disconnect the N inputs from Bias generation
  config = RREG(reg); delay(1);	//get the current bias settings
  bitClear(config,N);          				//clear this channel's bit to remove from bias generation
  WREG(reg,config); delay(1);  	//send the modified byte back to the ADS
}; 

//Active a channel in single-ended mode  
//  N is 1 through 8
//  gainCode is defined in the macros in the header file
//  inputCode is defined in the macros in the header file
void ADS1299::activateChannel(int N,byte gainCode,byte inputCode) 
{
	byte reg, config;
   //check the inputs
  if ((N < 1) || (N > 8)) return;
  
  //proceed...first, disable any data collection
  SDATAC(); delay(1);      // exit Read Data Continuous mode to communicate with ADS

  //active the channel using the given gain.  Set MUX for normal operation
  //see ADS1299 datasheet, PDF p44
  N = constrain(N-1,0,7);  //shift down by one
  config = 0b00000000;  						//left-most zero (bit 7) is to activate the channel
  gainCode = gainCode & 0b01110000;  			//bitwise AND to get just the bits we want and set the rest to zero
  config = config | gainCode; 					//bitwise OR to set just the gain bits high or low and leave the rest
  inputCode = inputCode & 0b00000111;  		//bitwise AND to get just the bits we want and set the rest to zero
  config = config | inputCode; 					//bitwise OR to set just the gain bits high or low and leave the rest

  if (use_SRB2[N]) config |= 0b00001000;  	//set the SRB2 flag if you plan to use it
  WREG(CH1SET+(byte)N,config); delay(1);

  //add this channel to the bias generation
  //see ADS1299 datasheet, PDF p44
  reg = BIAS_SENSP; 					// set up to connect the P inputs for bias generation
  config = RREG(reg); 			//get the current bias settings
  bitSet(config,N);                   	//set this channel's bit to add it to the bias generation
  WREG(reg,config); delay(1); //send the modified byte back to the ADS

  reg = BIAS_SENSN;  					// set up to connect the N input for bias generation
  config = RREG(reg); 			//get the current bias settings
  bitSet(config,N);                   	//set this channel's bit to add it to the bias generation
  WREG(reg,config); delay(1); //send the modified byte back to the ADS
  
  // // Now, these actions are necessary whenever there is at least one active channel
  // // though they don't strictly need to be done EVERY time we activate a channel.
  // // just once after the reset.
  
  //activate SRB1 as the Negative input for all channels, if needed
 setSRB1(use_SRB1());
  // setSRB1(false);

  //Finalize the bias setup...activate buffer and use internal reference for center of bias creation, datasheet PDF p42
  WREG(CONFIG3,0b11101100); delay(1);   // THIS COULD MOVE TO THE INITIALIZATION
};


// USE THIS METHOD IF YOU WANT TO CONTROL THE SIGNAL INCLUSION IN SRB AND BIAS GENERATION
void ADS1299::activateChannel(int N,byte gainCode,byte inputCode,boolean useInBias) 
{
	byte reg, config;
   //check the inputs
  if ((N < 1) || (N > 8)) return;
  
  //proceed...first, disable any data collection
  SDATAC(); delay(1);      // exit Read Data Continuous mode to communicate with ADS

  //active the channel using the given gain.  Set MUX for normal operation
  //see ADS1299 datasheet, PDF p44
  N = constrain(N-1,0,7);  //shift down by one
  config = 0b00000000;  						//left-most zero (bit 7) is to activate the channel
  gainCode = gainCode & 0b01110000;  			//bitwise AND to get just the bits we want and set the rest to zero
  config = config | gainCode; 					//bitwise OR to set just the gain bits high or low and leave the rest
  inputCode = inputCode & 0b00000111;  		//bitwise AND to get just the bits we want and set the rest to zero
  config = config | inputCode; 					//bitwise OR to set just the gain bits high or low and leave the rest
  if ((use_SRB2[N]) && useInBias)config |= 0b00001000;  	//set the SRB2 flag if you plan to use it
  WREG(CH1SET+(byte)N,config); delay(1);

  //add this channel to the bias generation
  //see ADS1299 datasheet, PDF p44
if(useInBias){
  reg = BIAS_SENSP; 					// set up to connect the P inputs for bias generation
  config = RREG(reg); 			//get the current bias settings
  bitSet(config,N);                   	//set this channel's bit to add it to the bias generation
  WREG(reg,config); delay(1); //send the modified byte back to the ADS

  reg = BIAS_SENSN;  					// set up to connect the N input for bias generation
  config = RREG(reg); 			//get the current bias settings
  bitSet(config,N);                   	//set this channel's bit to add it to the bias generation
  WREG(reg,config); delay(1); //send the modified byte back to the ADS
}
  
  // // Now, these actions are necessary whenever there is at least one active channel
  // // though they don't strictly need to be done EVERY time we activate a channel.
  // // just once after the reset.
  
  //activate SRB1 as the Negative input for all channels, if needed
//  setSRB1(use_SRB1());
  setSRB1(false);

  //Finalize the bias setup...activate buffer and use internal reference for center of bias creation, datasheet PDF p42
  WREG(CONFIG3,0b11101100); delay(1);   // THIS COULD MOVE TO THE INITIALIZATION
};

// SRB1 is used when connecting to the P channel inputs
void ADS1299::setSRB1(boolean desired_state) {
	if (desired_state) {
		WREG(MISC1,0b00100000); delay(1);  //ADS1299 datasheet, PDF p46
	} else {
		WREG(MISC1,0b00000000); delay(1);  //ADS1299 datasheet, PDF p46
	}
}


//Configure the test signals that can be inernally generated by the ADS1299
void ADS1299::configureInternalTestSignal(byte amplitudeCode, byte freqCode)
{
	if (amplitudeCode == ADSTESTSIG_NOCHANGE) amplitudeCode = (RREG(CONFIG2) & (0b00000100));
	if (freqCode == ADSTESTSIG_NOCHANGE) freqCode = (RREG(CONFIG2) & (0b00000011));
	freqCode &= 0b00000011;  		//only the last two bits are used
	amplitudeCode &= 0b00000100;  	//only this bit is used
	byte message = 0b11010000 | freqCode | amplitudeCode;  //compose the code
	
	WREG(CONFIG2,message); delay(1);
	
}
 
// Start continuous data acquisition
void ADS1299::startADS(void)
{
    RDATAC(); 
	delay(1);   // enter Read Data Continuous mode
    START();   			// start the data acquisition
	delay(1);
    isRunning = true;
}
  
// Query to see if data is available from the ADS1299...return TRUE is data is available
boolean ADS1299::isDataAvailable(void)
{
  return (!(digitalRead(PIN_DRDY)));
}
  
// Stop the continuous data acquisition
void ADS1299::stopADS(void)
{
    STOP();
	delay(1);   		// start the data acquisition
    SDATAC();
	delay(1);      	// exit Read Data Continuous mode to communicate with ADS
    isRunning = false;
}


//write as binary each channel's data
void ADS1299::writeADSchannelData(void)
{

	//print rawChannelData array
	for (int i = 0; i < 24; i++)
	{
		Serial.write(rawChannelData[i]); 
	}

}



//print out the state of all the control registers
void ADS1299::printAllRegisters(void)   
{
    boolean wasRunning = false;
		boolean prevverbosityState = verbosity;
		if (isRunning){ stopADS(); wasRunning = true; }
        verbosity = true;						// set up for verbosity output
        RREGS(0x00,0x17);     	// read out the first registers
//        delay(10);  						// stall to let all that data get read by the PC
//        RREGS(0x11,0x17-0x11);	// read out the rest
        verbosity = prevverbosityState;
		if (wasRunning){ startADS(); }
}

//only use SRB1 if all use_SRB2 are set to false
boolean ADS1299::use_SRB1(void) {
	for (int Ichan=0; Ichan < OPENBCI_NCHAN; Ichan++) {
		if (use_SRB2[Ichan]) {
			return false;
		}
	}
	return true;
}

void ADS1299::printDeviceID(void)
{
    boolean wasRunning;
    boolean prevverbosityState = verbosity;
    if (isRunning){ stopADS(); wasRunning = true;}
        verbosity = true;
        getDeviceID();
        verbosity = prevverbosityState;
    if (wasRunning){ startADS(); }
        
}

void ADS1299::csLow(void)
{
	SPI.setDataMode(SPI_MODE1);
	digitalWrite(CS, LOW);
	
}

void ADS1299::csHigh(void)
{
	digitalWrite(CS, HIGH);
	SPI.setDataMode(SPI_MODE0);
}
