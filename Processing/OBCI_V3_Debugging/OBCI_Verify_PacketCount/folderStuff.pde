



void folderSelected(File selection) {
  if (selection == null) {
    println("Window was closed or the user hit cancel.");
  } else {
    fileToVerify = selection.getAbsolutePath();
    println("User selected " + fileToVerify);  //selection.getAbsolutePath());
    dataReader = createReader(fileToVerify);  //selection.getAbsolutePath()); 
    reading = true;
    dataWriter.println("%OBCI Packet Count Verification of file " + fileToVerify);
    println("OBCI Packet Count Verification of file " + fileToVerify);
    println("timing file verification");
    thatTime = millis();
  }
}

void createFile(){
   logFileName = "OBCI_packetCount_verification/"+month()+"_"+day()+"_"+hour()+"_"+minute()+"_"+second()+".txt";
   dataWriter = createWriter(logFileName);
   
}


