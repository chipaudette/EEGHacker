/*
  Serial Event example
 
 When new serial data arrives, this sketch adds it to a String.
 When a newline is received, the loop prints the string and 
 clears it.
 
 A good test for this is to try it with a GPS receiver 
 that sends out NMEA 0183 sentences. 
 
 Created 9 May 2011
 by Tom Igoe
 
 This example code is in the public domain.
 
 http://www.arduino.cc/en/Tutorial/SerialEvent
 
 */

#define PIN_GROUND A1
int pins[]= {A2, A3, A4, A5};
#define NPINS 4
#define COMMAND_FOREWARD 3
#define COMMAND_LEFT 1
#define COMMAND_RIGHT 2
#define COMMAND_FIRE 0

volatile char inChar;
unsigned long lastCommand_millis = 0;
int commndDuration_millis = 500;


void setup() {
  // initialize serial:
  Serial.begin(115200);
  
  // print help
  Serial.println("TestHexBugController: starting...");
  Serial.println("Commands Include: ");
  Serial.println("    'P' = Forward");
  Serial.println("    '{' = Left");
  Serial.println("    '}' = Right");
  Serial.println("    '|' = Fire");
  
  //initialize the pins
  pinMode(PIN_GROUND,OUTPUT); digitalWrite(PIN_GROUND,LOW);
  stopAllPins();
}

void stopAllPins() {
  //stopping all pins means putting them into a high impedance state
  //Serial.println("Stopping All Pins...");
  for (int Ipin=0; Ipin < NPINS; Ipin++) {
    digitalWrite(pins[Ipin],LOW);
    pinMode(pins[Ipin],INPUT);
  }
}

void loop() {
  // print the string when a newline arrives:
  if (millis() > lastCommand_millis+commndDuration_millis) {
    lastCommand_millis = millis()+10000; //don't do this branch for a while
    stopAllPins();
  }
}

void issueCommand(int command_pin_ind) {
  if (command_pin_ind < NPINS) {
    stopAllPins();
    pinMode(pins[command_pin_ind],OUTPUT);
    digitalWrite(pins[command_pin_ind],LOW);
    lastCommand_millis = millis();  //time the command was received
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
     case 'P':
       issueCommand(COMMAND_FOREWARD); break;
     case '{':
       issueCommand(COMMAND_LEFT); break;
     case '}':
       issueCommand(COMMAND_RIGHT); break;
     case '|':
       issueCommand(COMMAND_FIRE); break;
     }
  }
}


