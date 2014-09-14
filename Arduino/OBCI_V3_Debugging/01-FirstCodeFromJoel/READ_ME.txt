	
	READ ME for early version of OBCI V3 Firmware.
	
	BLUE TOOTH RADIO LINK OVER VIEW
	
	First off, there are some major changes from the V2 software 
	requirements in terms of communication internal to the board and 
	also off the board. This is due to the use of the RF link. RFduinos
	are super powerful, but they come with limitations. They are based 
	on the Nordic BLE SOC.
	
	RFduino limitations and work-around.
		We are using the Gazelle library (Part of Nordic, wrapped by RFduino)
		This allows us to talk much faster than using BLE, but we need a dongle.
		Radio Data is sent in packets of 32 bytes MAX.
		There is a packet FIFO, but it is only 3 packets deep.
		The HOST (dongle) can only send data to DEVICE (board) on the ACK
		The ACK happens whenever HOST gets a message from DEVICE.
		So, DEVICE must poll the HOST to get any message from it.
		This is done with a timer. DEVICE polls every 50mS.
		Poll time seems to have a lower limit. Faster than 20mS is
		causing problems.

		
	Two modes of communication between Device and Host
	Standard Mode: 
		In Standard Mode, Device and Host have similar behavior.
			EXCEPT:
				Device polls Host when idle.
				Host sends special character to Device
				when PC wants to reset Arduino (for code upload)
		Serial -> Radio transfer:
			Host and Device use 2D serialBuffer[20][32] to prepare Serial data.
			When Serial is active, a timer is used to determine End Of Message.
			If the RX line is idle for > 2mS RFduino assumes end of message. 
			Serial data is prepared for the radio by taking the number of 32 byte
			arrays that have new data, placing that number in the serialBuffer[0][0]
			position, and then sending serialBuffer to the radio using the serialToSend flag.
			
		Radio -> Serial transfer:
			Host and Device use large radioBuffer[300] array to prepare Radio data.
			The first byte received on the radio contains the packetCount.
			When the packetsReceived = packetCount, the radioToSend flag is set.
			Then the entire radioBuffer array is sent out over Serial.
			
			
	SreamingDataMode:
		In Streaming Data Mode, Device and Host behave differently.
		Can't use the timeout, cause it takes too much time.
		DEVICE:
		Serial -> Radio transfer:
			When DEVICE receives a 'B' on the Serial port (coming from Arduino)
			it enters the streamingData mode. This flag triggers a different 
			way of handling data on the Serial port. Instead of using a timeout,
			it gets a checkSum, sent as the first byte from Arduino. 
			When DEVICE receives a 'S' on the Serial port (coming from Arduino)
			it exits streamingData mode.
			
		Radio -> Serial transfer:
			This operates the same way as Standard mode.
			
		HOST:
		Serial -> Radio transfer:
			This operates the same way as Standard mode.
			
		Radio -> Serial transfer:
			The radio packets are collected the same as in Standard mode, but
			before the data is sent out the Serial port, it is formatted to the 
			receiving software specifics. This is where data is packaged for 
			OpenBCI, OpenEEG, OpenVIBE, Plain Text, etc.
			
			
	Changes to V2 streaming data 
		sampleCounter in streaming data must be only one byte long
		
			
			
	Things To Do:
		Speed up the serial baud on both sides of the transmission. This has 
		run into a snag. I'm working with the RFduino folks on getting there.
		
		Handle multiple radios in the same space. The RFduino guys have assured me
		that they have this in hand. 
		
		
		
ARDUINO LIBRARY AND FIRMWARE OVER VIEW

	Big changes from the V2 firmware. 
		Added ACCELEROMETER and SD card.
		Arduino does not format data for specific PC software. 

OBCI_SD_LOG_05
	Arduino Library is being built up. The code in OBCI_SD_LOG_05 incorporates all the 
	on-board hardware. The software is designed to receive serial signals to initiate
	a logging session. The ADS data is written to the SD card in CSV HEX format. This is
	because we are writing to the SD card in BLOCKS of 512 bytes, and formatting in HEX
	gives us known data lengths. The BLOCK_COUNT variable determines the size of the file,
	and also the logging session time.
	 
		BLOCK_COUNT x 512 = file size
		Each block contain about 9 samples, so 9 x sample period x BLOCK_COUNT = log time(ish)
		
	MUST use high performance SC card for this to work. ScanDisk is a good brand. Needs 
	grade 10 or better!
	This code is the most up-to-date library for V3 hardware. Needs lots more functions 
	added from the V2 system. 
	
OBCI_V3_Stream_Raw_Data
	This code needs to be written and tested. Now that the streamingData mode is working 
	on the RFduinos, this project can commence. It needs Host formatting tested, and likely 
	there will be some timing issues with commands being sent from PC that may need adjustment.

		
UPLOADING FIRMWARE TO THE ON-BOARD RFDUINO
	Go to RFduino website and download their hardware profiles and insert them into
	the proper folder on your machine.
	
	Pictures in this file of the RFduino FTDI<>USB board and proper cable orientation.
	Note that you have to select the correct port in Arduino, and also select RFduino 
	as your board. 
	
UPLOADING FIRMWARE TO THE ON-DONGLE RFDUINO
	Move the slide switch to the RESET position (silk on bottom of board is labeled)
	Select the right serial port and RFduino as your board.
	
UPLOADING FIRMWARE TO THE TARGET ARDUINO
	Move the dongle slide switch to GPIO-6 selection. Make sure you have thie correct port and
	Arduino UNO selected as your board. Upload as normal from Arduino IDE. Note that it will
	take longer to upload than usual, because it is happening over air!
	If you want to, you can see the avrdude communication by going to Arduino,Preferences...
	and clicking the button beside 'Show Verbose Output During Upload'