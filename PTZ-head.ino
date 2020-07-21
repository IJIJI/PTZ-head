#include "Arduino.h"

#include <EEPROM.h>

#include <AccelStepper.h>
#include <MultiStepper.h>

#include <SPI.h>
// #include <nRF24L01.h>
#include <RF24.h>

#include "commands.h"




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

const byte EEPROMIdPTZ[5] = {2, 99, 17, 123, 45}; // Numbers written in the EEPROM (0 - 4) to detect first time boot. On the PTZ head.


int gotoPos[] = {0, 90, 180, -90, -180};

byte speed = 127;
unsigned long lastJoyReceive;

struct vector {
  int x;
  int y;
};


void setup() {

  Serial.begin(115200);
  delay(500);
  Serial.println("Starting... ");


  pinMode(LED, OUTPUT);

  pinMode(X_ENABLE_PIN, OUTPUT);
  pinMode(X_MAX_PIN, INPUT_PULLUP);

  digitalWrite(LED, HIGH);

  EEPROMCheck();

  digitalWrite(X_ENABLE_PIN, LOW);

  radio.begin();
  
  //set the address
  radio.openReadingPipe(0, address);
  
  //Set module as receiver
  radio.startListening();

  homeX();


  digitalWrite(LED, LOW);

  xAxis.setMaxSpeed(5000);
  // xAxis.setMaxSpeed(750);
  xAxis.setAcceleration(4000);

  lastJoyReceive = millis();

  Serial.println("done.");
}




void loop(){

  //Read the data if available in buffer
  if (radio.available())
  {
    byte receivedData[8] = {0};
    radio.read(&receivedData, sizeof(receivedData));
    



    if (receivedData[0] == joyUpdate){
      speed = receivedData[1];
      lastJoyReceive = millis();
    }
    else if (receivedData[0] == writePos){
      posToWrite(receivedData[1]);
    }
    else if (receivedData[0] == callPos){
      moveToPos(receivedData[1]);
    } 
    else if (receivedData[0] == homeNow){
      homeX();
    } 
    else if (receivedData[0] == errorNow){
      errorCode(receivedData[1]);
    } 


  }
  if (speed != 127 && millis() < lastJoyReceive + 250){
    xAxis.setAcceleration(5000000);
    xAxis.setSpeed(map(speed, 0, 255, -2000, 2000));
    xAxis.runSpeed();
    xAxis.moveTo(xAxis.currentPosition());
  }
  else {
    
    xAxis.run();
  }
}







void moveToPos(int pos){

  xAxis.setMaxSpeed(5000.0);
  xAxis.setAcceleration(4000.0);

  vector newPos;
  EEPROM.get(5 + (pos-1) * 4, newPos);
  xAxis.moveTo(degree(newPos.x));
}

void posToWrite(int pos){
  vector newPos;
  newPos.x = steps(xAxis.currentPosition());
  newPos.y = steps(0);
  EEPROM.put(5 + (pos-1) * 4, newPos);
}


void homeX(){

  // 360 degree 24000
  // 1.75 rotation = 42000
  xAxis.setCurrentPosition(0);
  xAxis.setMaxSpeed(5000.0);
  xAxis.setAcceleration(4000.0);

  digitalWrite(X_ENABLE_PIN, LOW);

  while (!digitalRead(X_MAX_PIN)) { // Make the Stepper move CW until the switch is deactivated
    xAxis.setSpeed(5000);
    xAxis.run();

    if (xAxis.currentPosition() >= degree(720)){
      errorCode(1);
    }
  }
  

  xAxis.setCurrentPosition(19500);
  // xAxis.moveTo(0);
  // xAxis.runToPosition();

  moveToPos(10);
  // xAxis.runToPosition();
}

int degree(int degree){
  // return 26000 / 360 * degree; // Returned incorrect value, probably due to too big var size
  return 72.22 * degree;
}

int steps(int steps){
  return steps / 72.22;
}

void EEPROMCheck(){
  bool success = true;
  for (int x; x < sizeof(EEPROMIdPTZ); x++){
    if(EEPROM.read(x) != EEPROMIdPTZ[x]){
      success = false;
      break;
    }
  }
  
  if (success == false){
    Serial.print("Rewriting EEPROM...");
    for (int x; x < sizeof(EEPROMIdPTZ); x++){
      EEPROM.update(x, EEPROMIdPTZ[x]);
    }
    for (int x = sizeof(EEPROMIdPTZ); x < EEPROM.length(); x++){
      EEPROM.update(x, 0x00);
    }
    Serial.println("Rewritten.");
  }
}




void errorCode(int code){
  if (code == 1){
    Serial.println("");
    Serial.println("ERROR: 001. Failed homing. Reset to continue.");
    errorLoop(true);
  }
  else {
    errorCode();
  }

  
}

void errorCode(){

  Serial.println("");
  Serial.println("ERROR. Reset to continue.");
  
  errorLoop(false);
}



void errorLoop(bool allowedHome){

  digitalWrite(X_ENABLE_PIN, HIGH);


  unsigned long lastChanged = millis();

  while(true){
    if (lastChanged + 600 < millis()){
      lastChanged = millis();
      digitalWrite(LED, !digitalRead(LED));
    }


    if (radio.available())
    {
      byte receivedDataError[8] = {0};
      radio.read(&receivedDataError, sizeof(receivedDataError));

      if (receivedDataError[0] == homeNow){
        homeX();
        break;
      }
    }

  }
}



