/*
This file is meant to operate as the .ino file for the second
arduino (or other processor) which will recieve a serial command
from the first processor (host arduino) and will begin the
ignition process, allowing the host arduino to continue to
monitor other conditions.

Ideally, the machine in it's stand-by state will already have a 
pellet loaded into it, waiting to be clamped and burned.  The 
break-beams that makes sure that there is a pellet ready to be
loaded should be interrupt driven.  All functions (clamping,
loading, igniting, etc) should all have their own functions for 
simplicity, but the serial command will call one function that
utliizes all of the other functions accordingly.
*/

/***************************************************************
              Goals
***************************************************************/

// [ ] Use IR proximity sensor to determine if a capsule is loaded
// [ ] PWM interface?
// [ ] Be sure to keep track of pellet state sliders
// [ ] Ensure that stand-by state has pellet loaded




#include "SmokeMachine.h"

//Instantiation of SmokeMachine
SmokeMachine smoker = SmokeMachine(4,6,5,7,8,9,14,15,16,36,47,49,48,6000);

String inputString = "";          //String for incoming data
boolean stringComplete = false;   //Boolean for if string complete or not
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
}

void loop(){
  if(stringComplete){
    role(inputString);
    inputString = "";
    stringComplete = false;
  }
}

void role(String input){
  String comm = getValue(input, ' ', 0);

/**************Setup Command*********************/
  
  if(comm == "setup\n"){
    if(loaded_fl == false){
      Serial.println("Loading Machine");
      loaded_pel = smoker.standbyInit();
      if(loaded_pel == 0)
        Serial.println("No Pellets to load");
      else if(loaded_pel == 1){
        Serial.println("Loading failed, only one pellet loaded");
        loaded_fl = true;
        empty_rail = true;
      }
      else if(loaded_pel == 2)
      {
        Serial.println("Machine Loaded Properly");
        loaded_fl = true;
        loaded_pel = 0;
        if(!(smoker.scanLA())){
          Serial.println("Warning: Rail is now Empty");
          empty_rail = true;
        }
        else
          empty_rail = false;
      }
    }
    else
      Serial.println("Error: Machine is not Empty");
  }

/*****************Burn Command*********************/
  
  else if(comm == "burn\n"){
    if(loaded_fl == true && empty_rail == false){
      Serial.println("Burning Starting");
      smoker.burnRound();
      Serial.println("Burning Completed");
      smoker.moveWheel();
      if(smoker.loadRound()){
        Serial.println("Next Pellet Loaded");
        empty_rail = false;
      }
      else
      {
        Serial.println("Rail is Empty");
        empty_rail = true;
      } 
    }
    else if(loaded_fl == true && empty_rail == true){
      Serial.println("Burning Startng");
      smoker.burnRound();
      Serial.println("Burning Completed");
      smoker.moveWheel();
      loaded_fl = false;
      Serial.println("Machine is now empty, please re-load machine");
    }
    else{
      Serial.println("Failed to burn, Machine is empty. Please re-load machine");
    }
  }

/*************Empty Command*******************************/

  else if(comm == "empty\n"){
    smoker.moveWheel();
    smoker.moveWheel();
    Serial.println("Machine has been empted of all pellets");
    loaded_fl = false;
    if(!(smoker.scanLA())){
      Serial.println("Rail is empty");
      empty_rail = true;
    }
    else if(smoker.scanLA()){
      Serial.println("Rail is loaded");
      empty_rail = false;
    }
  }

/************Unknown Command Return************************/
  
  else{
    Serial.println("unknown command, please try again");
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
