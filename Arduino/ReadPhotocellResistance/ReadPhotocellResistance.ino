/*
  Program: ReadPhotoCellResistance
  By: Chip Audette  May 2014
  Based on: Arduino example program "AnalogInOutSerial"
  
  The Circuit: Assumes the photocell is wired with one end plugged into +5V 
  and the other end has a 10K resistor to ground.  The Arduino AnalogIn pin
  is measuring at the junction between the photocell and the resistor.
  
  This example code is in the public domain.
*/

const int analogInPin = A0;  // Analog input pin that the potentiometer is attached to
int sensorValue = 0;        // value read from the pot

float V_ref = 5.000;       //this is the voltage reference of the Arduino ADC
float V_total = 5.000;     //this is the voltage that we're applying to the photocell+resistor
float R_bot_ohm = 10000.0; //this is the value of the resistor

void setup() {
  // initialize serial communications at 9600 bps:
  Serial.begin(115200); 
  
  // set the analog pin as an input
  pinMode(analogInPin,INPUT);
}

void loop() {
  // read the analog in value:
  sensorValue = analogRead(analogInPin);            

  // compute the voltage at the analog input pin
  float V= ((float)sensorValue)/((float)1023)*V_ref;
  
  // compute the voltage across the photocell
  float R_ohm = R_bot_ohm*(V_total / V) - R_bot_ohm;       

  // print the results to the serial monitor:
  Serial.print("sensor = " );                       
  Serial.print(sensorValue);      
  Serial.print("\t, R (kOhm) = ");      
  Serial.println(R_ohm/1000.0);   

  // wait XXX milliseconds before the next loop
  delay(1000);                     
}
