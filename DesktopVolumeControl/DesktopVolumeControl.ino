/*******Interrupt-based Rotary Encoder Sketch*******
by Simon Merrett, based on insight from Oleg Mazurov, Nick Gammon, rt, Steve Spence
*/

#include <Keyboard.h>
#include <Mouse.h>

static int pinA = 2; // Our first hardware interrupt pin is digital pin 2
static int pinB = 3; // Our second hardware interrupt pin is digital pin 4

static int rLed = 9; // Red LED
static int gLed = 6; // Green LED
static int bLed = 5; // Blue LED
static int micLed = 10; // Mic Button LED
static int otherLed = 11; // Extra Button LED

static int volMutePress = 4; // Rotary Encoder Switch
static int micMutePress = 7; // Mic Button Switch
static int otherPress = 8; // Other Button Switch

static int stepSize = 10; // Size of step to take with each volume increase (255 values, so number of steps = 255/stepSize)
static int debounce = 35; // Number of milliseconds to wait for debounce.

volatile byte aFlag = 0; // let's us know when we're expecting a rising edge on pinA to signal that the encoder has arrived at a detent
volatile byte bFlag = 0; // let's us know when we're expecting a rising edge on pinB to signal that the encoder has arrived at a detent 
  //(opposite direction to when aFlag is set)
volatile byte encoderPos = 0; //this variable stores our current value of encoder position. Change to int or uin16_t instead of byte 
  //if you want to record a larger range than 0-255
volatile byte oldEncPos = 0; //stores the last encoder position value so we can compare to the current reading and see if it has changed

volatile bool volMuteState = 0; // Stores whether the Volume Mute switch has been toggled "on" (1) or "off" (0).
volatile bool micMuteState = 0; // Stores whether the Mic Mute switch has been toggled "on" (1) or "off" (0).
volatile bool otherState = 0; // Stores whether the Other switch has been toggled "on" (1) or "off" (0).
volatile byte volumeLevel = 55; // Starting "volume" brightness. The Arduino has no idea what volume the computer is set to.

void setup() {

  //Set the LED pins to be outputs.
  pinMode(rLed, OUTPUT);
  pinMode(gLed, OUTPUT);
  pinMode(bLed, OUTPUT);
  pinMode(micLed, OUTPUT);
  pinMode(otherLed, OUTPUT);

  // Set the mute button to be an input.
  pinMode(volMutePress, INPUT);
  pinMode(micMutePress, INPUT);
  pinMode(otherPress, INPUT);

  // Turn off the LED's as a start state, then display the volume.
  digitalWrite(rLed, HIGH);
  digitalWrite(gLed, HIGH);
  digitalWrite(bLed, HIGH);
  digitalWrite(micLed, HIGH);
  digitalWrite(otherLed, HIGH);
  displayVolume();

  // Set the encoder pins as inputs with pullups and attach interrupts to both of them.
  pinMode(pinA, INPUT_PULLUP); // set pinA as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  pinMode(pinB, INPUT_PULLUP); // set pinB as an input, pulled HIGH to the logic voltage (5V or 3.3V for most cases)
  attachInterrupt(digitalPinToInterrupt(pinA),PinA,RISING); 
    // set an interrupt on PinA, looking for a rising edge signal and executing the "PinA" Interrupt Service Routine (below)
  attachInterrupt(digitalPinToInterrupt(pinB),PinB,RISING); 
    // set an interrupt on PinB, looking for a rising edge signal and executing the "PinB" Interrupt Service Routine (below)

  Keyboard.begin();
  Mouse.begin();
}

void PinA(){
  /* This is the interrupt method called when pinA is rising. It checks the values and can increase the volume. */
  
  cli(); //stop interrupts happening before we read pin values
  delay(debounce); // For debounce.

  if (digitalRead(pinA) == HIGH && digitalRead(pinB) == HIGH && aFlag) { 
    //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    
    encoderPos = oldEncPos - 1; //increment the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    
  }
  else if (digitalRead(pinA) == HIGH && digitalRead(pinB) == LOW) {
    bFlag = 1; //signal that we're expecting pinB to signal the transition to detent from free rotation
  }

  sei(); //restart interrupts
}

void PinB(){
  /* This is the interrupt method called when pinB is rising. It checks the values and can decrease the volume. */
  
  cli(); //stop interrupts happening before we read pin values
  delay(debounce); // For debounce.

  if (digitalRead(pinA) == HIGH && digitalRead(pinB) == HIGH && bFlag) { 
    //check that we have both pins at detent (HIGH) and that we are expecting detent on this pin's rising edge
    
    encoderPos = oldEncPos + 1; // decrement the encoder's position count
    bFlag = 0; //reset flags for the next turn
    aFlag = 0; //reset flags for the next turn
    
  }
  else if (digitalRead(pinA) == LOW && digitalRead(pinB) == HIGH) {
    
    aFlag = 1; //signal that we're expecting pinA to signal the transition to detent from free rotation
    
  }
  
  sei(); //restart interrupts
}

void volumeUp(){
  /* This method is responsible for increasing the volume. It increases the volumeLevel value by stepSize, 
   *  sends the keyboard command to increase the volume, and displays a brightness on the knob to indicate the new volume. */
  if (volumeLevel <= (255-stepSize)) { // Check for overruns
    volumeLevel = volumeLevel + stepSize; // increase volumeLevel
  }
  else {
    volumeLevel = 255; // or set it to the maximum
  }

  // Send a Win + Scroll Up command (3RVX command to raise the volume)
  Keyboard.press(KEY_LEFT_ALT); // KEY_LEFT_GUI is Winows, KEY_LEFT_CTRL is control, KEY_LEFT_ALT is Alt
  Mouse.move(0, 0, 1);
  delay(20);
  Keyboard.release(KEY_LEFT_ALT);

  displayVolume(); // Display the new volume.
}

void volumeDown(){
  /* This method is responsible for decreasing the volume. It decreases the volumeLevel value by stepSize, 
   *  sends the keyboard command to decrease the volume, and displays a brightness on the knob to indicate the new volume. */
  if (volumeLevel > (0 + stepSize)) { // Check for overruns
    volumeLevel = volumeLevel - stepSize; // decrease volumeLevel
  }
  else {
    volumeLevel = 0; // or set it to the minimum
  }

  // Send a Win + Scroll Down command (3RVX command to lower the volume)
  Keyboard.press(KEY_LEFT_ALT); // KEY_LEFT_GUI is the Winows Key, KEY_LEFT_CTRL is the control key
  Mouse.move(0, 0, -1);
  delay(20);
  Keyboard.release(KEY_LEFT_ALT);

  displayVolume(); // Display the new volume.
}

void muteVolume(){
  /* This method is responsible for muting the volume. It sets volMuteState to 1, sends the keyboard command, and 
   * turns on the green led to indicate that it has been muted. */
  
  volMuteState = 1; // For tracking that mute is on.

  // Send a Win + Middle Mouse command.
  Keyboard.press(KEY_LEFT_ALT);
  Mouse.click(MOUSE_MIDDLE);
  delay(50);
  Keyboard.release(KEY_LEFT_ALT);

  // Turn on the green LED.
  digitalWrite(rLed, HIGH);
  digitalWrite(gLed, LOW);
  digitalWrite(bLed, HIGH);
  
}

void unmuteVolume(){
  /* This method is responsible for unmuting the volume. It sets volMuteState to 0, sends the keyboard command, and
   * displays a brightness on the knob to indicate the volume. */
  
  volMuteState = 0; // For tracking that mute is off.

  // Send a Win + Middle Mouse command.
  Keyboard.press(KEY_LEFT_ALT);
  Mouse.click(MOUSE_MIDDLE);
  delay(50);
  Keyboard.release(KEY_LEFT_ALT);
  
  encoderPos = oldEncPos; // Clear any changes that have happened with the knob since mute began so that it won't jump volumes.
  
  displayVolume(); // Display the volume.
}

void muteMic(){
  /* This method is responsible for muting the microphone. It sets micMuteState to 1, sends the keyboard command, and 
   * turns on the mute led to indicate that it has been muted. */
  
  micMuteState = 1; // For tracking that mute is on.

  // Send a Win + Right Control command.
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press(KEY_RIGHT_CTRL);
  delay(debounce);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release(KEY_RIGHT_CTRL);

  // Turn on the mic LED.
  digitalWrite(micLed, LOW);
  
}

void unmuteMic(){
  /* This method is responsible for muting the microphone. It sets micMuteState to 0, sends the keyboard command, and 
   * turns off the mute led to indicate that it has been unmuted. */
  
  micMuteState = 0; // For tracking that mute is on.

  // Send a Win + Middle Mouse command.
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press(KEY_RIGHT_CTRL);
  delay(debounce);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release(KEY_RIGHT_CTRL);

  // Turn off the mic LED.
  digitalWrite(micLed, HIGH);
  
}

void pressOther(){
  /* This method is responsible for muting the microphone. It sets micMuteState to 1, sends the keyboard command, and 
   * turns on the mute led to indicate that it has been muted. */
  
  otherState = 1; // For tracking that mute is on.

/*  // Send a Win + Middle Mouse command.
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press(KEY_RIGHT_CTRL);
  delay(debounce);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release(KEY_RIGHT_CTRL);
*/

  // Turn on the mic LED.
  digitalWrite(otherLed, LOW);
  
}

void unpressOther(){
  /* This method is responsible for muting the microphone. It sets micMuteState to 0, sends the keyboard command, and 
   * turns off the mute led to indicate that it has been unmuted. */
  
  otherState = 0; // For tracking that mute is on.

/*  // Send a Win + Middle Mouse command.
  Keyboard.press(KEY_LEFT_GUI);
  Keyboard.press(KEY_RIGHT_CTRL);
  delay(debounce);
  Keyboard.release(KEY_LEFT_GUI);
  Keyboard.release(KEY_RIGHT_CTRL);
*/

  // Turn off the mic LED.
  digitalWrite(otherLed, HIGH);
  
}

void displayVolume(){
  /* This method simply displays the volume as an analog value. */

  // Turn off the red and green LED's and set the blue to indicate brightness.
  digitalWrite(rLed, HIGH);
  digitalWrite(gLed, HIGH);
  analogWrite(bLed, 255-volumeLevel);
  
}

void loop(){
  /* The loop just checks the state of the interrupts and the mute button and runs the methods pertaining to each of the 
   *  different commands that need to be sent. */
   
  if(oldEncPos != encoderPos) { // Check if the encoder has been rotated.
    
    if (volMuteState != 1) { // Check if the volume is muted.
      
      if (encoderPos > oldEncPos || (oldEncPos >= (255 - stepSize) && (encoderPos <= stepSize))) { 
        // Check that the encoder value is increasing
        
        volumeDown(); // Increase the volume.
        
      }
      else { // If the encoder is decreasing
        
        volumeUp(); // Decrease the volume.
        
      }
      
      oldEncPos = encoderPos; // Set the new state for reference.
      
    }
  }
  
  if (digitalRead(volMutePress) == HIGH) {  // Check whether the mute button is pressed.
    
    delay(debounce); // wait for debounce.
    
    if (digitalRead(volMutePress) == HIGH) { // Check whether the button is still pressed.
      
      if (volMuteState == 0) { // If the volume is not muted...
        
        muteVolume(); // Mute the volume.
        
      }
      else { //Otherwise,
        
        unmuteVolume(); // Unmute the volume.
        
      }
    }
    while (digitalRead(volMutePress) == HIGH) { // Wait for the button to be released before continuing.
      delay(debounce);
    }
  } 

  if (digitalRead(micMutePress) == HIGH) {  // Check whether the mute button is pressed.
    
    delay(debounce); // wait for debounce.
    
    if (digitalRead(micMutePress) == HIGH) { // Check whether the button is still pressed.
      
      if (micMuteState == 0) { // If the volume is not muted...
        
        muteMic(); // Mute the volume.
        
      }
      else { //Otherwise,
        
        unmuteMic(); // Unmute the volume.
        
      }
    }
    while (digitalRead(micMutePress) == HIGH) { // Wait for the button to be released before continuing.
      delay(debounce);
    }
  } 

  if (digitalRead(otherPress) == HIGH) {  // Check whether the mute button is pressed.
    
    delay(debounce); // wait for debounce.
    
    if (digitalRead(otherPress) == HIGH) { // Check whether the button is still pressed.
      
      if (otherState == 0) { // If the volume is not muted...
        
        pressOther(); // Mute the volume.
        
      }
      else { //Otherwise,
        
        unpressOther(); // Unmute the volume.
        
      }
    }
    while (digitalRead(otherPress) == HIGH) { // Wait for the button to be released before continuing.
      delay(debounce);
    }
  }
  
}
