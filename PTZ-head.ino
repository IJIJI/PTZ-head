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
AccelStepper yAxis(1, Y_STEP_PIN, Y_DIR_PIN);


//create an RF24 object
RF24 radio(53, 49);  // CE, CSN

//address through which two modules communicate.
const byte address[6] = "69489";

const byte EEPROMIdPTZ[5] = {3, 99, 17, 123, 45}; // Numbers written in the EEPROM (0 - 4) to detect first time boot. On the PTZ head.


int gotoPos[] = {0, 90, 180, -90, -180};


unsigned long lastJoyReceive;

struct vector {
  int x;
  int y;
};

vector speed = {127, 127};

void setup() {

  Serial.begin(115200);
  delay(500);
  Serial.println("Starting... ");


  pinMode(LED, OUTPUT);

  pinMode(X_ENABLE_PIN, OUTPUT);
  pinMode(X_MAX_PIN, INPUT_PULLUP);

  pinMode(Y_ENABLE_PIN, OUTPUT);
  pinMode(Y_MAX_PIN, INPUT_PULLUP);

  digitalWrite(LED, HIGH);

  EEPROMCheck();

  digitalWrite(X_ENABLE_PIN, HIGH);
  digitalWrite(Y_ENABLE_PIN, HIGH);

  radio.begin();
  
  //set the address
  radio.openReadingPipe(0, address);
  
  //Set module as receiver
  radio.startListening();

  // home();
  errorLoop(true);

  digitalWrite(LED, LOW);

  xAxis.setMaxSpeed(5000);
  // xAxis.setMaxSpeed(750);
  xAxis.setAcceleration(4000);

  yAxis.setMaxSpeed(2500);
  // xAxis.setMaxSpeed(750);
  yAxis.setAcceleration(2000);

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
      speed.x = receivedData[1];
      speed.y = receivedData[2];
      lastJoyReceive = millis();
    }
    else if (receivedData[0] == writePos){
      posToWrite(receivedData[1]);
    }
    else if (receivedData[0] == callPos){
      moveToPos(receivedData[1]);
    } 
    else if (receivedData[0] == homeNow){
      home();
    } 
    else if (receivedData[0] == errorNow){
      errorCode(receivedData[1]);
    } 


  }
  if (speed.x != 127 || speed.y != 127 && millis() < lastJoyReceive + 250){
    xAxis.setAcceleration(5000000);
    xAxis.setSpeed(map(speed.x, 0, 255, -2000, 2000));

    yAxis.setAcceleration(5000000);
    yAxis.setSpeed(map(speed.y, 0, 255, -2000, 2000));

    xAxis.runSpeed();
    xAxis.moveTo(xAxis.currentPosition());

    yAxis.runSpeed();
    yAxis.moveTo(yAxis.currentPosition());
  }
  else {
    xAxis.run();
    yAxis.run();
  }
}







void moveToPos(int pos){

  xAxis.setMaxSpeed(5000.0);
  xAxis.setAcceleration(4000.0);

  yAxis.setMaxSpeed(2500.0);
  yAxis.setAcceleration(2000.0);

  vector newPos;
  EEPROM.get(5 + (pos-1) * 4, newPos);
  xAxis.moveTo(degree(newPos.x));
  yAxis.moveTo(degree(newPos.y));
}

void posToWrite(int pos){
  vector newPos;
  newPos.x = steps(xAxis.currentPosition());
  newPos.y = steps(yAxis.currentPosition());
  EEPROM.put(5 + (pos-1) * 4, newPos);
}

void home(){
  homeX();
  homeY();
  moveToPos(1);
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

}

void homeY(){

  // 360 degree 24000
  // 1.75 rotation = 42000
  yAxis.setCurrentPosition(0);
  yAxis.setMaxSpeed(2500.0);
  yAxis.setAcceleration(2000.0);

  digitalWrite(Y_ENABLE_PIN, LOW);
  

  yAxis.setCurrentPosition(0);

}

int degree(int degree){
  // return 26000 / 360 * degree; // Returned incorrect value, probably due to too big var size
  return 8.888888 * degree * 4;
}

int steps(int steps){
  return steps * 0.1125 / 4;
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
  digitalWrite(Y_ENABLE_PIN, HIGH);


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
        home();
        break;
      }
    }

  }
}



