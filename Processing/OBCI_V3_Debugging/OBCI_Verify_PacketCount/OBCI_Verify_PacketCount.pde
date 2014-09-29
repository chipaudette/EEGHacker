/**
 * This code will ask you to select an OBCI data file to verify
 * The file is then checked for dropped packets by verifying the sample counter increment (+1)
 * After checking, it prints a report to the sketch terminal
 * Started with the Button example, added scratch from other examples that work with files.
 * Relies on the OpenBCI Processing GUI by Chip Audette (link)
 * 
 * OBCI raw count to uV scale factor:
 * float scale_fac_uVolts_per_count = ADS1299_Vref / (pow(2,23)-1) / ADS1299_gain  * 1000000.f; //ADS1299 datasheet Table 7, confirmed through experiment
 *
 */
 
 
//properties of the openBCI board
float fs_Hz = 250.0f;  //sample rate used by OpenBCI board
final float ADS1299_Vref = 4.5f;  //reference voltage for ADC in ADS1299
final float ADS1299_gain = 24;  //assumed gain setting for ADS1299
final float scale_fac_uVolts_per_count = ADS1299_Vref / (pow(2,23)-1) / ADS1299_gain  * 1000000.f; //ADS1299 datasheet Table 7, confirmed through experiment
 
PFont font;  

BufferedReader dataReader;
String dataLine;
PrintWriter dataWriter;
String convertedLine;
int lastSampleCounter = 0;
long lineCounter = 0;
String fileToVerify;
String thisLine;
String[] hexNums;
float[] intData = new float[20];
String logFileName;
long thisTime;
long thatTime;

int buttonX, buttonY;      // Position of square button
float buttonHeight = 60;     // height of button
float buttonWidth = 400;
color buttonColor, buttonHighlight, strokeColor; 
color baseColor;
boolean overButton = false;
boolean reading = false;

void setup() {
  size(640, 360);
  font = createFont("Dialog",24);
  textFont(font);
  textAlign(CENTER);
  rectMode(CENTER);
  buttonColor = color(0,200,148);
  buttonHighlight = color(19,203,48);
  strokeColor =  color(0,0,0);
  baseColor = color(77,36,21);
  buttonX = width/2;
  buttonY = height/2;
  
  
}

void draw() {
  background(baseColor);
  
  drawButton();
    
  while (reading){
    try {
      dataLine = dataReader.readLine();
    }catch (IOException e) {
      e.printStackTrace();
      dataLine = null;
    }
  
    if (dataLine == null) {
    // Stop reading because of an error or file is empty
      thisTime = millis() - thatTime;
      reading = false;
      dataWriter.println("read " + lineCounter + " lines");
      println("read " + lineCounter + " lines");
      println("nothing left in file"); 
      dataWriter.println("verify took "+thisTime+" mS");
      println("verify took "+thisTime+" mS");
      dataWriter.flush();
      dataWriter.close();
      lineCounter = 0;
      lastSampleCounter = 0;
    }else{
//        println(dataLine);
      lineCounter++;
      String[] sampleString = splitTokens(dataLine,",");
//      println(sampleString[0].charAt(0);
      if(sampleString[0].charAt(0) == '%'){  // assume '%' as line comment marker
//          println(dataLine);
        dataWriter.println(dataLine);  // print the comment to the file
        println(dataLine);
      }
      else{
        int thisSampleCounter = int(sampleString[0]);
//        println(thisSampleCounter);        
        if(thisSampleCounter > lastSampleCounter+1){
          if((thisSampleCounter == 0x00) && (lastSampleCounter != 0xFF)){
            println("roll-over error");
          }
//              write the difference to file
            dataWriter.print("At line " + lineCounter + "  "); 
            dataWriter.print(thisSampleCounter - lastSampleCounter -1);
            dataWriter.println(" packets dropped");
            print("At line " + lineCounter + "  ");
            print(thisSampleCounter - lastSampleCounter -1);
            println(" packets dropped");
        }
        lastSampleCounter = thisSampleCounter;
      }
      } 
    }
}// end of draw
  






