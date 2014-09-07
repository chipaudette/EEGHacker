/*
  TextQuadCopterController
  
  Created: Chip Audette, Sept 2014
  http://eeghacker.blogspot.com

  Purpose: This code assumes that you've hacked quad copter remote control with digital pots
  to allow the Arduino to "drive" the remote control's potentiometers.  You issue serial commands
  from the PC, which are received by the Arduino, when then interacts with the digital pots.
  
  Quadcopter Remote Control: There are two, 2-D joysticks quadcopter remote.  Each axis of the
  joysticks is an analog potentiometer.  The high side of each pot is connected to the device's
  ~3V power supply and the low side is ground.  For all but one of the pots, the joystick's default
  position is in the middle, which creates a 3V * 1/2 = ~1.5V control voltage.  For the last pot
  (the pot controlling the quadcopter's overal rotor power, thereby controling up-down), the pot
  starts at zero (no power) and goes up max (full power).

  Hack: I soldered wires to the wiper of each pot to the wiper of a digital pot.  The high side
  of each digital pot is tied to ~3V (via 1K) and the low side of each pot is tied to gnd.  The
  exception is the digital pot for up-down.  Here, I wired the digital pot as a rheostat (ie,
  variable resistor).  I connected it between the wiper of the joystick and ground.  Therefore,
  you set the joystick to a given position, and the digital pot will the resulting control voltage
  down (and only down) from the joystick's value.
  
  License: MIT License 2014
 
 */

#include "MCP42XXX.h"
#include "SPI.h"

#define SLIDE_LEFT ('[')   //it's below the '-', so it's close to the power controls
#define SLIDE_RIGHT (']')  //it's below the '+', so it's close to the power controls
#define BACKWARD ('s')     //it's between the 'a' and the 'd', forming a cluster
#define FOREWARD ('w')     //it's above the 's', so it's close to its opposite
#define TURN_LEFT ('a')    //it's the row as 'd',it's opposite, forming a cluster
#define TURN_RIGHT ('d')   //it's the row as 'a',it's opposite, forming a cluster
#define POWER_DOWN ('-')   //clearly, this means "less power"
#define POWER_UP ('=')     //it's the '+' (more power), but without needing to press the shift key
#define SHUT_DOWN (' ')
#define CHANGE_MODE_ACCUM ('1')
#define CHANGE_MODE_TEMP ('2')

#define MAX_VAL (255)
#define MIN_VAL (0)

//define order of joystick axes in the potValues array
#define IND_UP_DOWN (0)
#define IND_SLIDE (1)
#define IND_BACK_FORE (2)
#define IND_TURN (3)

//define the operating mode
#define MODE_ACCUM 0     //for trimming
#define MODE_TEMP 1  // for temporary commands


const int SS_pin = 10;
const int chan_per_device = 2;
MCP42XXX digiPots = MCP42XXX(SS_pin,true,chan_per_device);
const int n_Devices = 2;
const int n_PotValues = 4;
byte potValues[n_PotValues];
byte defaultPotValues[n_PotValues];
byte potIncrement_accum[] = {2,2,2,5};
byte potIncrement_temp[] = {10, 10, 10, 10};
int operating_mode = MODE_ACCUM;

volatile char inChar;
unsigned long lastCommand_millis = 0;
int commndDuration_millis = 500;


void setup() {
  // initialize serial:
  Serial.begin(115200);
  
  //define default pot values...adjust these to trim out level flight
  defaultPotValues[IND_UP_DOWN] = 0; //always have this be zero!
  defaultPotValues[IND_SLIDE] = 127;  //something close to mid point
  defaultPotValues[IND_BACK_FORE] = 127; //something close to mid point
  defaultPotValues[IND_TURN] = 127;  //something close to mid point

  //initialize setup
  shutDownQuadcopter();
  
  // print help
  Serial.println(F("TestHexBugController: starting..."));
  Serial.println(F("Commands Include: "));
  Serial.print(F("    Power Up:    ")); Serial.println(POWER_UP);
  Serial.print(F("    Power Down:  ")); Serial.println(POWER_DOWN);
  Serial.print(F("    Foreward:    ")); Serial.println(FOREWARD);
  Serial.print(F("    Backward:    ")); Serial.println(BACKWARD);
  Serial.print(F("    Turn Left:   ")); Serial.println(TURN_LEFT);
  Serial.print(F("    Turn Right:  ")); Serial.println(TURN_RIGHT);
  Serial.print(F("    Slide Left:  ")); Serial.println(SLIDE_LEFT);
  Serial.print(F("    Slide Right: ")); Serial.println(SLIDE_RIGHT);
  Serial.print(F("    Shut Down:   ")); Serial.println(F(" (space bar) "));
  printOperatingMode();
  Serial.print(F("    Switch to Accum Mode: ")); Serial.println(CHANGE_MODE_ACCUM);
  Serial.print(F("    Switch to Temp Mode:  ")); Serial.println(CHANGE_MODE_TEMP);
  
}

void shutDownQuadcopter() {
  Serial.println(F("shutDownQuadCopter..."));
  for (int i=0; i<n_PotValues; i++) {
    potValues[i]=defaultPotValues[i];
  }
  potValues[IND_UP_DOWN] = 0;
  digiPots.setValues(potValues,n_PotValues);
}

void stopTempCommand() {
  Serial.println(F("stopTempCommand..."));
  for (int i=0; i<n_PotValues; i++) {
    if (i != IND_UP_DOWN) {
      potValues[i]=defaultPotValues[i];
    }
  }
  digiPots.setValues(potValues,n_PotValues);
}  
  
void loop() {
  // print the string when a newline arrives:
  if (millis() > lastCommand_millis+commndDuration_millis) {
    lastCommand_millis = millis()+500; //don't do this branch for a few milliseconds
    if (operating_mode == MODE_TEMP) {
      stopTempCommand();
    }
  }
}


/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so using delay inside loop can delay
 response.  Multiple bytes of data may be available.
 */
void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    Serial.print("Received "); Serial.println(inChar);
    switch (inChar) {
      case SLIDE_LEFT:
        incrementPotAndIssueCommand(IND_SLIDE,-1); break;
      case SLIDE_RIGHT:
        incrementPotAndIssueCommand(IND_SLIDE,1); break;
      case TURN_LEFT:
        incrementPotAndIssueCommand(IND_TURN,-1); break;
      case TURN_RIGHT:
        incrementPotAndIssueCommand(IND_TURN,1); break;
      case BACKWARD:
        incrementPotAndIssueCommand(IND_BACK_FORE,-1); break;
      case FOREWARD:
        incrementPotAndIssueCommand(IND_BACK_FORE,1); break;
       case POWER_DOWN:
        incrementPotAndIssueCommand(IND_UP_DOWN,-1); break;
      case POWER_UP:
        incrementPotAndIssueCommand(IND_UP_DOWN,1); break;
      case SHUT_DOWN:
        shutDownQuadcopter(); break;
      case CHANGE_MODE_ACCUM:
        switchOperatingMode(MODE_ACCUM); break;
      case CHANGE_MODE_TEMP:
        switchOperatingMode(MODE_TEMP); break;
     }
  }
}

void incrementPotAndIssueCommand(int index,int sign) {
  
  //is this a legitimate index?
  if (index < n_PotValues) {
    
    //compute the new command value
    int increment = (int)potIncrement_accum[index];
    if (operating_mode == MODE_TEMP) increment = (int)potIncrement_temp[index];
    if (sign < 0) increment = -increment;
    int foo_val = ((int)potValues[index]) + increment;
    foo_val = max(MIN_VAL,min(MAX_VAL,foo_val));
    potValues[index] = ((byte)foo_val);
    
    if (operating_mode == MODE_ACCUM) {
      defaultPotValues[index] = potValues[index];
    }
    
    //print the command
    Serial.print(F("issuing command: "));
    for(int i=0; i<n_PotValues; i++) {
      Serial.print(potValues[i]);
      Serial.print(" ");
    }
    Serial.println();
    
    //issue the command
    digiPots.setValues(potValues,n_PotValues);
  }
}

void switchOperatingMode(int new_mode) {
  Serial.print(F("Current mode: "));
  Serial.print(operating_mode);
  Serial.print(F(", Commanded mode: "));
  Serial.println(new_mode);
  
  stopTempCommand(); //stop any temporary command currently happening
  operating_mode = new_mode; //change the mode
  printOperatingMode();
}

void printOperatingMode(void) {
  switch (operating_mode) {
    case MODE_ACCUM:
      Serial.println(F("operating_mode = MODE_ACCUM"));
      break;
    case MODE_TEMP:
      Serial.println(F("operating_mode = MODE_TEMP"));
      break;
  }
}
