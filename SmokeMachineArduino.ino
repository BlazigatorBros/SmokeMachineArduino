/*
This file is meant to operate as the .ino file for the second
arduino (or other processor) which will recieve a serial command
from the first processor (host arduino) and will begin the
ignition process, allowing the host arduino to continue to
monitor other conditions.

Ideally, the machine in it's stand-by state will already have a 
pellet loaded into it, waiting to be clamped and burned.  The 
break-beams that makes sure that there is a pellet ready to be
loaded should be interrupt driven (FALSE, turned out to cause problems).  
All functions (clamping, loading, igniting, etc) should all have 
their own functions for simplicity, but the serial command will 
call one function that utliizes all of the other functions accordingly.
*/

/***************************************************************
              Goals
***************************************************************/

// [ ] Use IR proximity sensor to determine if a capsule is loaded
// [ ] PWM interface?
// [ ] Be sure to keep track of pellet state sliders
// [ ] Ensure that stand-by state has pellet loaded

#include "SmokeMachine.h"

/*****************Defines for PINS************************/
#define wheelDirPin A3
#define wheelMovePin A5
#define wheelEnPin A4
#define ignitionPin 3 //12
#define fanCtrlPin 4 //13
#define fanFbPin A7
#define slot1Pin A2
#define slot2Pin A1
#define slot3Pin A0
#define clampPin 5 //11
#define loaderIRPin 8 //5
#define loaderFBPin 6 //6
#define loaderDirPin 7 //10
#define burnTime 15000
/*********************************************************/


//Instantiation of SmokeMachine
SmokeMachine smoker = SmokeMachine(wheelDirPin, wheelMovePin, wheelEnPin, ignitionPin, fanCtrlPin, fanFbPin, slot1Pin, slot2Pin, slot3Pin, clampPin, loaderIRPin, loaderFBPin, loaderDirPin, burnTime);

String inputString = "";          //String for incoming data
boolean stringComplete = false;   //Boolean for if string complete or not

String inputString1_1 = "";         //Copies for Serial1 port
String inputString1_2 = "";
String inputString1_3 = "";
boolean string1Complete_1 = false;
boolean string1Complete_2 = false;
boolean string1Complete_3 = false;

boolean stdby_st = false;
boolean empty_rail = false;
boolean empty_wheel = false;

boolean next_pel = false;

int loaded_pel = 0;
boolean loaded_fl = false;

int emp_fl = 0;

void setup(){
  Serial.begin(9600);
  inputString.reserve(200);
  Serial1.begin(9600);
  inputString1_1.reserve(200);
  inputString1_2.reserve(200);
  inputString1_3.reserve(200);
  pinMode(A0, INPUT);
  pinMode(A1, INPUT);
  pinMode(A2, INPUT);
  while(smoker.getPos() == 0){    //This loop will run until it detects the smoke machine
  }                               //Once smoke machine found, breaks loop and turns on fan
  smoker.fanCtrl(true);           //May need to change this to a serial signal from main arduino instead
  inputString1_1 = "";
  inputString1_2 = "";
  inputString1_3 = "";
  string1Complete_1 = false;
  string1Complete_2 = false;
  string1Complete_3 = false;
  Serial.print("Machine Properly Initialized\n");
}

void loop(){
  if(stringComplete){
    role(inputString);
    inputString = "";
    stringComplete = false;
  }
  if(string1Complete_1){
    role(inputString1_1);
    inputString1_1 = "";
    string1Complete_1 = false;
  }
  if(string1Complete_2){
    role(inputString1_2);
    inputString1_2 = "";
    string1Complete_2 = false;
  }
  if(string1Complete_3){
    role(inputString1_3);
    inputString1_3 = "";
    string1Complete_3 = false;
  }
}

void role(String input){
  String comm = getValue(input, ' ', 0);

/**************Load Command*********************/
  
  if(comm == "load\n"){
    if(loaded_fl == false){
      //Serial.println("Loading Machine");
      loaded_pel = smoker.standbyInit();
      if(loaded_pel == 0){
        //Serial.print("Warning: No Pellets to load\n");
        Serial1.print("\rWarning: No Pellets to load\n");
      }
      else if(loaded_pel == 1){
        //Serial.print("Warning: Loading failed, only one pellet loaded\n");
        Serial1.print("\rWarning: Loading failed, only one pellet loaded\n");
        loaded_fl = true;
        empty_rail = true;
      }
      else if(loaded_pel == 2)
      {
        loaded_fl = true;
        loaded_pel = 0;
        if(!(smoker.scanLA())){
          //Serial.print("Warning: Machine loaded but rail is empty\n");
          Serial1.print("\rWarning: Machine loaded but rail is empty\n");
          empty_rail = true;
        }
        else{
          //Serial.print("Machine Loaded Properly\n");
          Serial1.print("\rMachine Loaded Properly\n");
          empty_rail = false;
        }
      }
    }
    else
      //Serial.print("Warning: Pellets already loaded, doing nothing\n");
      Serial1.print("\rWarning: Pellets already loaded, doing nothing\n");
  }

/*****************Burn Command*********************/
  
  else if(comm == "burn\n"){
    if(loaded_fl == true && empty_rail == false){
      Serial1.println("\rBurning Started\n");
      smoker.burnRound();
      smoker.moveWheel();
      if(smoker.loadRound()){
        //Serial.print("Burning Complete\n");
        Serial1.print("\rBurning Complete\n");
        empty_rail = false;
      }
      else
      {
        //Serial.print("Warning: Rail is Empty\n");
        Serial1.print("\rBurning Complete\n");
        Serial1.print("\rWarning: Rail is Empty\n");
        empty_rail = true;
      } 
    }
    else if(loaded_fl == true && empty_rail == true){
      Serial1.print("\rBurning Started\n");
      Serial1.print("\rWarning: Machine is now empty, please re-load machine\n");
      smoker.burnRound();
      smoker.moveWheel();
      loaded_fl = false;
      Serial1.print("\rBurning Complete\n");
    }
    else{
      Serial1.print("\rError: Failed to burn, Machine is empty. Please re-load machine\n");
    }
  }

/*************Empty Command*******************************/

  else if(comm == "empty\n"){
    //Serial.print("Warning: Machine has been emptied of all pellets\n");
    smoker.Empty();
    smoker.Empty();
    loaded_fl = false;
    if(!(smoker.scanLA())){
      //Serial.println("Warning: Rail is empty");
      empty_rail = true;
    }
    else if(smoker.scanLA()){
      //Serial.println("Rail is loaded");
      empty_rail = false;
    }
    Serial1.print("\rWarning: Machine has been emptied of all pellets\n");
  }

/************Unknown Command Return************************/
  
  else{
    Serial.print("Warning: Unknown Command\n");
    Serial1.print("\rWarning: Unknown Command\n");
  }
}

String getValue(String data, char separator, int index) {
    int found = 0;
    int strIndex[] = {0, -1};
    int maxIndex = data.length()-1;

    for(int i=0; i<=maxIndex && found<=index; i++) {
        if(data.charAt(i)==separator || i==maxIndex) {
            found++;
            strIndex[0] = strIndex[1]+1;
            strIndex[1] = (i == maxIndex) ? i+1 : i;
        }
    }
    return found>index ? data.substring(strIndex[0], strIndex[1]) : "";
}

void serialEvent(){
  while(Serial.available()){
    char inChar = (char)Serial.read();
    inputString += inChar;
    if(inChar == '\n'){
      stringComplete = true;
    }
  }
}

void serialEvent1(){
  while(Serial1.available()){
    char inChar = (char)Serial1.read();
    if(string1Complete_1 && string1Complete_2){
      inputString1_3 += inChar;
      if(inChar == '\n'){
        string1Complete_3 = true;
      }
    }
    else if(string1Complete_1){
      inputString1_2 += inChar;
      if(inChar == '\n'){
        string1Complete_2 = true;
      }
    }
    else{
      inputString1_1 += inChar;
      if(inChar == '\n'){
        string1Complete_1 = true;
      }
    }
  }
}
