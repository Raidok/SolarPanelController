/*
  Solar panel controller
  by Raidok
 */


#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <EEPROM.h>

// NEID VÕIB VABALT MUUTA
byte temp[6] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
/*
 * management during time calculation
 * 0,1 - start time
 * 2,3 - rewind time
 * 4 - interval (ms)
 * 5 - stepTime (ms)
 * management during runtime
 * 0 - current step
 * 
 */
/*{ 6, 30 }; { 18, 30 }; { 3, 30 }; */
unsigned long interval;
unsigned long stepTime;
unsigned long endTime;
char command = '\0'; // nullchar (end of string)
AlarmID_t alarmId;


// SIIT EDASI EI TASU VÄGA PUUTUDA
boolean left = false;
boolean right = false;
boolean leftEdge = false;
boolean rightEdge = false;
boolean leftBtn = false;
boolean rightBtn = false;

#define LEFT_BTN  2
#define RIGHT_BTN 3
#define LEFT_END  4
#define RIGHT_END 5

#define PIN1 7
#define PIN2 8
#define MAX_TIME 21000


// EEPROM INDEX CONSTANTS
#define EE_START_H      0
#define EE_START_M      1
#define EE_REWIND_H     2
#define EE_REWIND_M     3
#define EE_INTERVAL_H   4
#define EE_INTERVAL_L   5
#define EE_STEP_TIME_H  6
#define EE_STEP_TIME_L  7

#define USE_SPECIALIST_METHODS 1

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

/////////////////////////////////// SETUP

void setup() {
  // begin serial debugging
  Serial.begin(9600);
  Serial.println("Setup started");
  
  // clock sync
  setSyncProvider(RTC.get);
  
  // begin ethernet
  //Serial.print("IP: ");
  //Ethernet.begin(temp);
  //Serial.println(Ethernet.localIP());
  
  // set pinmodes
  pinMode(PIN1, OUTPUT);
  digitalWrite(PIN1, HIGH);
  pinMode(PIN2, OUTPUT);
  digitalWrite(PIN2, HIGH);
  pinMode(LEFT_BTN, INPUT);
  pinMode(RIGHT_BTN, INPUT);
  pinMode(LEFT_END, INPUT);
  pinMode(RIGHT_END, INPUT);

  // load variables from EEPROM
  loadFromEeprom();

  // serial input buffer
  inputString.reserve(20);
  
  // set alarms
  handleMoving();
  setAlarms();
}



/////////////////////////////////// LOOP

void loop() {
  
  handleMoving();
  
  if (!left && !right) { // movement is priority!
    
    if (stringComplete) {
      Serial.println(inputString);
      command = inputString.charAt(0);
      handleCommands();
      inputString = "";
      stringComplete = false;
    }
    
    if (leftBtn && !left) {
      Serial.println("VASAK NUPP");
      moveLeft(stepTime/4);
    } else if (rightBtn && !right) {
      Serial.println("PAREM NUPP");
      moveRight(stepTime/4);
    }
  }
}



/////////////////////////////////// ACTION FUNCTIONS

void handleMoving() {

  Alarm.delay(1000);
  
  // lets check the boundaries
  /*leftEdge = parseAnalog(END1);
  rightEdge = parseAnalog(END2);
  leftBtn = parseAnalog(BTN1);
  rightBtn = parseAnalog(BTN2);*/
  leftEdge = digitalRead(LEFT_END);
  rightEdge = digitalRead(RIGHT_END);
  leftBtn = digitalRead(LEFT_BTN);
  rightBtn = digitalRead(RIGHT_BTN);
  
  Serial.print(leftEdge);
  Serial.print(" ");
  Serial.print(rightEdge);
  Serial.print(" ");
  Serial.print(leftBtn);
  Serial.print(" ");
  Serial.println(rightBtn);
  
  if (leftEdge && left && millis() < endTime) {
    //Serial.println("LEFT");
    digitalWrite(PIN1, HIGH);
  } else {
    digitalWrite(PIN1, LOW);
    left = false;
  }
  
  if (rightEdge && right && millis() < endTime) {
    //Serial.println("RIGHT");
    digitalWrite(PIN2, HIGH);
  } else {
    digitalWrite(PIN2, LOW);
    right = false;
  }
}

boolean moveLeft(long time) {
  if (!(left || right) && leftEdge) {
    String str = "Moving left for ";
    str += time;
    str += " ms!";
    log("INFO", str);
    left = true;
    endTime = millis() + time;
    return true;
  }
  String str = "Moving left for ";
  str += time;
  str += " ms failed! Left:";
  str += left;
  str += " Right:";
  str += right;
  str += " LeftEdge:";
  str += leftEdge;
  log("INFO", str);
  return false;
}

boolean moveRight(long time) {
  if (!(left || right) && rightEdge) {
    String str = "Moving right for ";
    str += time;
    str += " ms!";
    log("INFO", str);
    right = true;
    endTime = millis() + time;
    return true;
  }
  String str = "Moving right for ";
  str += time;
  str += " ms failed! Left:";
  str += left;
  str += " Right:";
  str += right;
  str += " RightEdge:";
  str += rightEdge;
  log("INFO", str);
  return false;
}

/*void runTest() {
  
  if (tests == 0) {
    moveRight(MAX_TIME);
    tests++;
    return;
  } else if (tests == 1) {
    if (right) {
      return;
    }
    tests++;
    return;
  } else if (tests == 2) {
    delay(100);
    moveLeft(MAX_TIME);
    interval = millis();
    tests++;
    return;
  } else if (tests == 3) {
    if (left) {
      return;
    }
    
    interval = millis() - interval;
    String str = "Trip length: ";
    str += interval;
    str += " ms.";
    log("INFO", str);
    
    ms = interval / steps;
    str = "Step length: ";
    str += ms;
    str += " ms.";
    log("INFO", str);
    
    nextRun = millis() + ms;
    
    interval = stop[0] - start[0];
    interval = interval / steps * 60 * 60 * 1000;
    str = "Interval: ";
    str += interval;
    str += " ms.";
    log("INFO", str);
    
    tests++;
    return;
  }
}*/



/////////////////////////////////// HELPERS

void handleCommands() {
  switch (command) {
    case '\0':
      // do nothing
      break;
    case 'R':
      //log("DEBUG", "Read command requested!");
      loadFromEeprom();
      break;
    case 'W':
      log("DEBUG", "Write command requested!");
      writeToEeprom();
      break;
    case 'T':
      log("DEBUG", "Time set command requested!");
      setMyTime(stringToTime(inputString));
      break;
    default:
      String str = "Unknown command \'";
      str += command;
      str += "\' requested!";
      log("DEBUG", str);
  }
  command = ' '; // reset
  Serial.flush();
}

void log(String type, String msg) {
  time_t t = now();
  String logString = "";
  logString.reserve(200);
  logString += getDigits(day(t));
  logString += ".";
  logString += getDigits(month(t));
  logString += ".";
  logString += year(t);
  logString += " ";
  logString += getDigits(hour(t));
  logString += ":";
  logString += getDigits(minute(t));
  logString += ":";
  logString += getDigits(second(t));
  logString += "|";
  logString += type;
  logString += "|";
  logString += msg;
  Serial.println(logString);
}

String getDigits(int digits) {
  String returned = "";
  if (digits < 10) returned += "0";
  returned += digits;
  return returned;
}

boolean parseAnalog(byte pin) {
  /*Serial.print("ANALOG PIN ");
  Serial.print(pin);
  Serial.print(" : ");*/
  int reading = analogRead(pin);
  //Serial.println(reading);
  return reading < 50;
}

void loadFromEeprom() {
  String str = "EEPROM";
  for (byte i = 0; i < 4; i++) {
    temp[i] = EEPROM.read(i);
    str += " ";
    str += i;
    str += ":";
    str += temp[i];
  }
  
  // read interval 
  interval = EEPROM.read(EE_INTERVAL_H) << 8;
  interval |= EEPROM.read(EE_INTERVAL_L);
  str += " 4-5: ";
  str += interval;
  
  // read stepTime
  stepTime = EEPROM.read(EE_STEP_TIME_H) << 8;
  stepTime |= EEPROM.read(EE_STEP_TIME_L);
  str += " 6-7: ";
  str += stepTime;
  log("DEBUG", str);
}

void writeToEeprom() {
  EEPROM.write(EE_START_H, 6);
  EEPROM.write(EE_START_M, 30);
  EEPROM.write(EE_REWIND_H, 3);
  EEPROM.write(EE_REWIND_M, 30);
  interval = 10; // seconds
  EEPROM.write(EE_INTERVAL_H, highByte(interval));
  EEPROM.write(EE_INTERVAL_L, lowByte(interval));
  stepTime = 4000; // milliseconds
  EEPROM.write(EE_STEP_TIME_H, highByte(stepTime));
  EEPROM.write(EE_STEP_TIME_L, lowByte(stepTime));
}


/////////////////////////////////// ALARMS

void setAlarms() {
  log("INFO", "Setting alarms!");
  Alarm.alarmRepeat(temp[0], temp[1], 0, MorningAlarm); // first in the morning
  Alarm.timerRepeat(interval, MovingAlarm);
  Alarm.alarmRepeat(temp[2], temp[3], 0, RewindAlarm); // rewind at night
  if (leftEdge && rightEdge) {
    MovingAlarm(); // in case the cycle has started at the time of restarting
  }
}

void MorningAlarm() {
  log("INFO", "Good morning!");
  Alarm.timerRepeat(interval, MovingAlarm);
  MovingAlarm();
  Alarm.enable(alarmId);
}

void MovingAlarm() {
  alarmId = getTriggeredAlarmId();
  Serial.print("alarm:");
  Serial.println(alarmId);
  if (rightEdge) {
    log("INFO", "This just moved!!");
    moveRight(stepTime); // move it!
  } else {
    log("INFO", "Cycle has reached the end.");
  }
}

void NightAlarm() {
  log("INFO", "Good night!");
  Alarm.disable(alarmId);
}

void RewindAlarm() {
  log("INFO", "Rewinded.");
  moveLeft(MAX_TIME); // turn it for 30 seconds or until reaches end
}



/////////////////////////////////// TIME

void setMyTime(unsigned long t) {
  if (t > 0) {
    RTC.set(t);
    setTime(t);
  }
  String str = "Time set to ";
  str += t;
  log("INFO", str);
}

time_t stringToTime(String str) {
  time_t pctime = 0;
  String strlog = "String is ";
  strlog += str;
  log("INFO", strlog);
  for (int i = 0; i < str.length(); i++) {
    char c = str.charAt(i);
    if( c >= '0' && c <= '9') {
      pctime = (10 * pctime) + (c - '0') ; // convert digits to a number
    }
  }
  return pctime;
}




/////////////////////////////////// SERIAL


void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '.') {
      stringComplete = true;
      inChar = '\0'; // nullchar (end of string)
    }
    inputString += inChar;
  }
}


