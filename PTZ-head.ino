#include "Arduino.h"


#include <AccelStepper.h>
#include <MultiStepper.h>

#include <SPI.h>
// #include <nRF24L01.h>
#include <RF24.h>

#include "commands.h"
//! REMOVE WHEN .h FILE WORKS
//! REMOVE WHEN .h FILE WORKS
//! REMOVE WHEN .h FILE WORKS

// const String commandNames[] = {"null", "joyUpdate", "writePos", "callPos"};


// const byte joyUpdate = 1; // formatting: identifier, joyX, joyY, joyZ, Speed
// // ID identifies the call type, in this case a joy update.
// // joyX/Y/Z are 0-255, with 127 being centered.
// // speed is speed in modes 1-7


// const byte writePos = 2; // formatting: identifier, button
// const byte callPos = 3; // formatting: identifier, button, speed
// // button formatting = none:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 0:10

// // Button formatting = none:0, 1:1, 2:2, 3:3, 4:4, 5:5, 6:6, 7:7, 8:8, 9:9, 0:10, *:11, #:12

//! REMOVE WHEN .h FILE WORKS
//! REMOVE WHEN .h FILE WORKS
//! REMOVE WHEN .h FILE WORKS



#define LED 8


#define X_STEP_PIN                            54
#define X_DIR_PIN                             55
#define X_ENABLE_PIN                          38

#define Y_STEP_PIN                            60
#define Y_DIR_PIN                             61
#define Y_ENABLE_PIN                          56


#define X_MIN_PIN                             3
#define X_MAX_PIN                             2

#define Y_MIN_PIN                             14
#define Y_MAX_PIN                             15



AccelStepper xAxis(1, X_STEP_PIN, X_DIR_PIN);


//create an RF24 object
RF24 radio(53, 49);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "69489";

const byte EEPROMIdPTZ[] = {2, 99, 17, 123, 55}; // Numbers written in the EEPROM (0 - 4) to detect first time boot. On the PTZ head.


int gotoPos[] = {0, 90, 180, -90, -180};




void setup() {

  Serial.begin(115200);
  Serial.print("Starting... ");


  pinMode(LED, OUTPUT);

  pinMode(X_ENABLE_PIN, OUTPUT);
  pinMode(X_MAX_PIN, INPUT_PULLUP);



  digitalWrite(LED, HIGH);
  digitalWrite(X_ENABLE_PIN, LOW);

  homeX();


  radio.begin();
  
  //set the address
  radio.openReadingPipe(0, address);
  
  //Set module as receiver
  radio.startListening();


  digitalWrite(LED, LOW);

  // xAxis.setMaxSpeed(5000);
  xAxis.setMaxSpeed(750);
  xAxis.setAcceleration(4000);

  Serial.println("done.");
}




void loop(){


  //Read the data if available in buffer
  if (radio.available())
  {
    byte receivedData[10] = {0};
    radio.read(&receivedData, sizeof(receivedData));
    


    // Serial.print("R: Command: ");
    // Serial.print(receivedData[0]);
    // Serial.print(" Data: ");
    // for (int x = 1; x < sizeof(receivedData); x++){
    //   // if (receivedData[x] == 0){
    //   //   break;
    //   // }
    //   Serial.print(receivedData[x]);
    //   Serial.print(" ");
    // }
    // Serial.println();


    if (receivedData[0] == callPos){
      moveToPos(receivedData[1]);
    }
  }
  xAxis.run();
}



int degree(int degree){
  // return 26000 / 360 * degree; // Returned incorrect value, probably due to too big var size
  return 72.22 * degree;
}



void moveToPos(int pos){

  xAxis.setMaxSpeed(5000.0);
  xAxis.setAcceleration(4000.0);

  xAxis.moveTo(degree(gotoPos[pos-1]));
}



void homeX(){

  // 360 degree 24000
  // 1.75 rotation = 42000
  xAxis.setCurrentPosition(0);
  xAxis.setMaxSpeed(5000.0);
  xAxis.setAcceleration(4000.0);



  while (!digitalRead(X_MAX_PIN)) { // Make the Stepper move CW until the switch is deactivated
    xAxis.setSpeed(5000);
    xAxis.run();

    if (xAxis.currentPosition() >= degree(720)){
      errorCode(1);
    }
  }
  

  xAxis.setCurrentPosition(19500);
  xAxis.moveTo(0);
  xAxis.runToPosition();
}







void errorCode(int code){
  if (code == 1){
    Serial.println("");
    Serial.println("ERROR: 001. Failed homing. Reset to continue.");
  }
  else {
    errorCode();
  }

  errorLoop();
}

void errorCode(){

  Serial.println("");
  Serial.println("ERROR. Reset to continue.");
  
  errorLoop();
}



void errorLoop(){

  digitalWrite(X_ENABLE_PIN, HIGH);

  while(true){
    digitalWrite(LED, HIGH);
    delay(750);
    digitalWrite(LED, LOW);
    delay(750);
  }
}



