//
//  Definitions.h
//  
//
//  Created by Conor Russomanno, Luke Travis, and Joel Murphy. Summer 2013.
//
//

#ifndef _Definitions_h
#define _Definitions_h

//SPI
#define SPI_DATA_MODE 0x04 //CPOL = 0; CPHA = 1 (Datasheet, p8)
#define SPI_MODE_MASK 0x0C  // mask of CPOL and CPHA  on SPCR
#define SPI_CLOCK_MASK 0x03  // SPR1 = bit 1, SPR0 = bit 0 on SPCR
#define SPI_2XCLOCK_MASK 0x01  // SPI2X = bit 0 on SPSR
#define SPI_CLOCK_DIV_2 0X04	// 8MHz SPI SCK
#define SPI_CLOCK_DIV_4 0X00	// 4MHz SPI SCK
#define SPI_CLOCK_DIV_16 0x01    // 1MHz SPI SCK

//ADS1299 SPI Command Definition Byte Assignments (Datasheet, p35)
#define _WAKEUP 0x02 // Wake-up from standby mode
#define _STANDBY 0x04 // Enter Standby mode
#define _RESET 0x06 // Reset the device registers to default
#define _START 0x08 // Start and restart (synchronize) conversions
#define _STOP 0x0A // Stop conversion
#define _RDATAC 0x10 // Enable Read Data Continuous mode (default mode at power-up)
#define _SDATAC 0x11 // Stop Read Data Continuous mode
#define _RDATA 0x12 // Read data by command; supports multiple read back

//ASD1299 Register Addresses
#define ID      0x00
#define CONFIG1 0x01
#define CONFIG2 0x02
#define CONFIG3 0x03
#define LOFF 0x04
#define CH1SET 0x05
#define CH2SET 0x06
#define CH3SET 0x07
#define CH4SET 0x08
#define CH5SET 0x09
#define CH6SET 0x0A
#define CH7SET 0x0B
#define CH8SET 0x0C
#define BIAS_SENSP 0x0D
#define BIAS_SENSN 0x0E
#define LOFF_SENSP 0x0F
#define LOFF_SENSN 0x10
#define LOFF_FLIP 0x11
#define LOFF_STATP 0x12
#define LOFF_STATN 0x13
#define GPIO 0x14
#define MISC1 0x15
#define MISC2 0x16
#define CONFIG4 0x17

//LIS3DH
#define READ_REG		0x80
#define READ_MULTI		0x40
#define LIS3DH_DRDY             3

#define LIS3DH_MODE		3	// c pol =1, c pha = 1, mode = 3
#define STATUS_REG_AUX	        0x07	// axis over-run and data available flags (see 0x27)
#define OUT_ADC1_L		0x08	// 
#define OUT_ADC1_H		0x09	//
#define OUT_ADC2_L		0x0A	//	ADC input values (check DS)
#define OUT_ADC2_H		0x0B	//
#define OUT_ADC3_L		0x0C	//
#define OUT_ADC3_H		0x0D	//
#define INT_COUNTER_REG	        0x0E	// ??
#define WHO_AM_I		0x0F	// DEVICE ID = 0x33
#define TMP_CFG_REG		0x1F	// ADC enable (0x80); Temperature sensor enable (0x40)
#define CTRL_REG1		0x20	// Data Rate; Power Mode; X enable; Y enable; Z enable (on >= 0x10)
#define CTRL_REG2		0x21	// High Pass Filter Stuph
#define CTRL_REG3		0x22	// INT1 select register
#define CTRL_REG4		0x23	// Block update timing; endian; G-force; resolution; self test; SPI pins
#define CTRL_REG5		0x24	// reboot; FIFO enable; latch; 4D detection;
#define CTRL_REG6		0x25	// ??
#define REFERENCE		0x26	// interrupt reference
#define STATUS_REG2		0x27	// axis overrun and availale flags (see 0x07)
#define OUT_X_L			0x28	//
#define OUT_X_H			0x29	//
#define OUT_Y_L			0x2A	//	tripple axis values (see 0x0A)
#define OUT_Y_H			0x2B	//
#define OUT_Z_L			0x2C	//
#define OUT_Z_H			0x2D	//
#define FIFO_CTRL_REG	        0x2E	// FIFO mode; trigger output pin select (?); 
#define FIFO_SRC_REG	        0x2F	// ??
#define INT1_CFG		0x30	// 6 degree control register
#define INT1_SOURCE		0x31	// axis threshold interrupt control
#define INT1_THS		0x32	// INT1 threshold
#define INT1_DURATION	        0x33	// INT1 duration
#define CLICK_CFG		0x38	// click on axis
#define CLICK_SRC		0x39	// other click
#define CLICK_THS		0x3A	// more click
#define TIME_LIMIT		0x3B	// click related
#define TIME_LATENCY	        0x3C	// and so on
#define TIME_WINDOW		0x3D	// contined click

//GENERAL STUFF
#define PIN_DRDY (8)
#define PIN_RST (9)
#define SCK_MHZ (4)
#define ADS_SS (10) // ADS chip select
#define DAISY_SS (7)  // ADS Daisy chip select
#define SD_SS (6)  // SD card chip select
#define LIS3DH_SS (5)  // LIS3DH chip select

//Pick which version of OpenBCI you have
#define OPENBCI_V1 (1)    //Sept 2013
#define OPENBCI_V2 (2)    //Oct 24, 2013
#define OPENBCI_V2 (3)	  //April, 2014
#define OPENBCI_NCHAN (8)  // number of EEG channels

//gainCode choices
#define ADS_GAIN01 (0b00000000)
#define ADS_GAIN02 (0b00010000)
#define ADS_GAIN04 (0b00100000)
#define ADS_GAIN06 (0b00110000)
#define ADS_GAIN08 (0b01000000)
#define ADS_GAIN12 (0b01010000)
#define ADS_GAIN24 (0b01100000)

//inputCode choices
#define ADSINPUT_NORMAL (0b00000000)
#define ADSINPUT_SHORTED (0b00000001)
#define ADSINPUT_BIAS_MEAS (0b00000010)
#define ADSINPUT_MVDD (0b00000011)
#define ADSINPUT_TEMP (0b00000100)
#define ADSINPUT_TESTSIG (0b00000101)
#define ADSINPUT_BIAS_DRP (0b00000110)
#define ADSINPUT_BIAL_DRN (0b00000111)

//test signal choices...ADS1299 datasheet page 41
#define ADSTESTSIG_AMP_1X (0b00000000)
#define ADSTESTSIG_AMP_2X (0b00000100)
#define ADSTESTSIG_PULSE_SLOW (0b00000000)
#define ADSTESTSIG_PULSE_FAST (0b00000001)
#define ADSTESTSIG_DCSIG (0b00000011)
#define ADSTESTSIG_NOCHANGE (0b11111111)

//binary communication codes for each packet
#define PCKT_START 0xA0
#define PCKT_END 0xC0

#endif
