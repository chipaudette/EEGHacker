


import processing.serial.*;

String openBCI_portName = "COM5"; 
int baud = 115200;
Serial serial_openBCI = null;
float fs_Hz = 250.0;  //sample rate
boolean writeHeader = false;
OutputFile_rawtxt outputFile = null;

boolean mode_isStreaming = false;

void setup() {
  println("Serial Ports:");
  println(Serial.list());
  println("Using Predefined Port: " + openBCI_portName);
  serial_openBCI = new Serial(this, openBCI_portName, baud);
}

void draw() {
  
}

void keyPressed() {
  println("From keyboard: '" + key + "'...sending to OpenBCI...");
  switch (key) {
    case 'b':
      //open file
      if (outputFile == null) {
        outputFile = new OutputFile_rawtxt(fs_Hz,writeHeader);
        println("Openned Data File: " + outputFile.fname);
      }
      mode_isStreaming = true;
      break;
    case 's':
      //close file
      if (serial_openBCI != null) {
        println("Closing Data File: " + outputFile.fname);
        outputFile.closeFile();
        outputFile = null;
      }
      mode_isStreaming = false;
      break;
  }
      
  //send command to OpenBCI
  serial_openBCI.write(key);
    
}

final int packetLength = 33;
int packet[] = new int[packetLength];
int packetPosition=0;
boolean goodValue = false;
boolean isEndOfPacket = false;
int packetCounter = 0;
void serialEvent(Serial p) {
  goodValue=false;
  isEndOfPacket=false;
  int value = p.read();
  //print(char(value));

  if (mode_isStreaming == false) {
    //not streaming...so just print the received value
    print(char(value));
  } else {
    if (outputFile != null) {
      outputFile.output.print(value + "\t");
      if (value == 192) {
        outputFile.output.println();
        packetCounter++;
        if ((packetCounter % 100) == 0) println("Written packet " + packetCounter);
      }
//    //gather into a packet
//    if (packetPosition==0) {
//      //looking for packet start
//      if (value == 160) {
//        goodValue = true;
//      }
//    } else if (packetPosition == (packetLength-1)) {
//      //looking for end
//      if (value == 192) {
//        goodValue = true;
//        isEndOfPacket = true;
//      }
//    } else {
//      goodValue = true;
//    }
//   
//    //should we save this value into the packet structure?
//    if (goodValue) {
//      packet[packetPosition] = value;
//      packetPosition++;
//      
//      //should we write the packet
//      if (isEndOfPacket) {
//        packetPosition = 0;
//        packetCounter++;
//        if (outputFile != null) {
//          if ((packetCounter % 100) == 0) println("Writing packet " + packetCounter);
//          for (int i=0; i < packetLength; i++) {
//             outputFile.output.print(packet[i] + "\t");
//          }
//          outputFile.output.println();
//        }
//      }
//    }
    } 
  }
  
}
