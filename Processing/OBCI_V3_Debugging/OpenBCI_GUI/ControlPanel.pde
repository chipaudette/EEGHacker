

//////////////////////////////////////////////////////////////////////////
//
//		System Control Panel
//		- Select serial port from dropdown
//		- Select default configuration (EEG, EKG, EMG)
//		- Select Electrode Count (8 vs 16)
//		- Select data mode (synthetic, playback file, real-time)
//		- Record data? (y/n)
//			- select output location
//		- link to help guide
//		- buttons to start/stop/reset application
//
//////////////////////////////////////////////////////////////////////////

// import processing.serial.*;  //for serial communication to Arduino/OpenBCI
// import java.util.*; //for Array.copyOfRange()
// import java.lang.Math; //for exp, log, sqrt...they seem better than Processing's built-in

ControlP5 cp5;
String[] serialPorts = new String[Serial.list().length];

class ControlPanel {

	public int panelWidth, panelHeight;
	public boolean isOpen;

	Button initSystemButton;
	boolean initButtonPressed = false;

	PlotFontInfo fontInfo;

	// ControlP5 cp5;
	ListBox serialListBox;
	// Textfield serialTextfield;

	int vertOffset;
	int padding;

	ControlPanel(OpenBCI_GUI mainClass){
		panelWidth = width/4;
		panelHeight = height;
		isOpen = true;

		vertOffset = controlPanelCollapser.but_dy;

		fontInfo = new PlotFontInfo();

		padding = 5;

		initSystemButton = new Button (padding, (panelHeight-(panelHeight/10))-padding, panelWidth-padding*2, panelHeight/10, "Initialize System", fontInfo.buttonLabel_size);

		cp5 = new ControlP5(mainClass);

		PFont pfont = createFont("Helvetica", 16, false);
		ControlFont font = new ControlFont(pfont,16);

		// setting up serial list box
		// serialTextfield = cp5.addTextfield("Selected Serial/COM Port")
		// 	.setPosition(padding, vertOffset + padding)
		// 	.setSize(panelWidth - padding*2, 30)
		// 	;

		// serialTextfield.setAutoClear(true).keepFocus(true);



		serialListBox = cp5.addListBox("serialList")
			.setPosition(padding, vertOffset*2 + padding)
			.setSize(panelWidth - padding*2, 200 - padding*2)
			.setItemHeight(24)
			.setBarHeight(24)
			.setColorBackground(color(255, 128))
			.setColorActive(color(0))
			.setColorForeground(color(255, 100,0))
			.setScrollbarWidth(25)
			.disableCollapse()
			// .setScrollbarHeight(100)
			;

		serialListBox.captionLabel().toUpperCase(false);
		serialListBox.captionLabel().setFont(font);
		serialListBox.captionLabel().set("Select Your Port");
		serialListBox.captionLabel().setColor(0xff000000); //set caption label to black .. first two bytes = alpha .. then r, g, b
		serialListBox.captionLabel().style().marginTop = 4;

		// serialListBox.valueLabel().toUpperCase(false);
		// serialListBox.valueLabel().setFont(font);
		// serialListBox.valueLabel().setColor(0xff000000); //set caption label to black .. first two bytes = alpha .. then r, g, b
		
		// serialListBox.valueLabel().style().marginTop = 4;
		// serialListBox.valueLabel().style().marginTop = 4;

		// cp5.getContoller("serialList")
		// 	.setFont(font)
		// 	.toUpperCase(false)
		// 	;

		serialPorts = Serial.list();
		for(int i = 0; i < serialPorts.length; i++){
			String tempPort = serialPorts[i];
			ListBoxItem lbi = serialListBox.addItem(tempPort, i);
		}
	}

	public void update(){
		//toggle view of cp5 / serial list selection table
		if(isOpen){ // if control panel is open
			if(!cp5.isVisible()){  //and cp5 is not visible
				cp5.show(); // shot it
			}
		}
		else{ //the opposite of above
			if(cp5.isVisible()){
				cp5.hide();
			}
		}
	}

	public void draw(){

		pushStyle();
		noStroke();

		// if(isOpen){

			//dark overlay of rest of interface to indicate it's not clickable
			fill(0,0,0,185);
			rect(0,0,width,height);

			//background pane of control panel
			fill(35,35,35);
			rect(0,0,panelWidth,panelHeight);
		// }	

		initSystemButton.draw();

		popStyle();
	}

	//mouse pressed in control panel
	public void CPmousePressed(){
		println("CPmousePressed");
		if(initSystemButton.isMouseHere()){
			initSystemButton.setIsActive(true);
			initButtonPressed = true;
		}
	}

	//mouse released in control panel
	public void CPmouseReleased(){
		if(initSystemButton.isMouseHere() && initButtonPressed){
			println("init");

			//prepare the serial port
		    // println("port is open? ... " + portIsOpen);
		    // if(portIsOpen == true){
		    //   openBCI.closeSerialPort();
		    // }

			initSystem();
			systemMode = 10;
		}

		//always unclick/deactivate buttons if mouse released
		initSystemButton.setIsActive(false);
		initButtonPressed = false;
	}
};

void controlEvent(ControlEvent theEvent) {
  // ListBox is if type ControlGroup.
  // 1 controlEvent will be executed, where the event
  // originates from a ControlGroup. therefore
  // you need to check the Event with
  // if (theEvent.isGroup())
  // to avoid an error message from controlP5.

  if (theEvent.isGroup()) {
    // an event from a group e.g. scrollList
    println(theEvent.group().value()+" from "+theEvent.group());
  }
  
  //after picking serial port from list...
  if(theEvent.isGroup() && theEvent.name().equals("serialList")){
    String tempSerial = serialPorts[(int)theEvent.group().value()];
    // controlPanel.serialTextfield.setText(tempSerial); //set text field = to port name
    controlPanel.serialListBox.captionLabel().set(tempSerial);
    openBCI_portName = tempSerial; //and make GLOBAL serial/com port = to selected string
    println("openBCI_portName = " + openBCI_portName);
  }
}
