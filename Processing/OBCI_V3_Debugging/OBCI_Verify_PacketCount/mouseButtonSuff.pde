


void drawButton(){
  updateButton();
  if (overButton) {
      fill(buttonHighlight);
    } else {
      fill(buttonColor);
    }
    stroke(strokeColor);
    rect(buttonX, buttonY, buttonWidth, buttonHeight);
    fill(0);
    text("select file to verify", width/2,height/2 + 10);
}


void updateButton() {
  if (buttonOver(buttonX, buttonY, buttonWidth, buttonHeight) ) {
    overButton = true;
  } else {
    overButton = false;
  }
}

boolean buttonOver(int x, int y, float w, float h)  {
  if (mouseX >= x-w/2 && mouseX <= x+w/2 && 
      mouseY >= y-h/2 && mouseY <= y+h/2) {
    return true;
  } else {
    return false;
  }
}

void mousePressed() {
  if (overButton) {
    println("mouse pressed");
    createFile();
    selectInput("Select a folder to process:", "folderSelected");
  }
}
