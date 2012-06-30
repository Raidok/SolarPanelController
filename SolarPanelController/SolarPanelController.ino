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



// array for temporary values, initally used for MAC-address,
// later holds start, stop and rewind times
byte temp[6] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
// run interval, in seconds
unsigned long interval = 10; // TODO: remove value
// lenght of one step, in milliseconds
unsigned long stepTime = 5000; // TODO: remove value
// time when current movement should end, in milliseconds
unsigned long endTime;
// for measuring how long current movement lasted
unsigned long eventTime;
// command char
char command = '\0'; // nullchar (end of string)
//
AlarmID_t alarmId;

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

#define MOVE_LEFT 7
#define MOVE_RIGHT 8
#define MAX_TIME 21000



// EEPROM INDEX CONSTANTS

#define EE_RUNTIME_VARS 6

#define EE_START_H      0
#define EE_START_M      1
#define EE_STOP_H       2
#define EE_STOP_M       3
#define EE_REWIND_H     4
#define EE_REWIND_M     5

#define EE_INTERVAL_H   6
#define EE_INTERVAL_L   7
#define EE_STEP_TIME_H  8
#define EE_STEP_TIME_L  9


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
  pinMode(MOVE_LEFT, OUTPUT);
  //digitalWrite(PIN1, HIGH);
  pinMode(MOVE_RIGHT, OUTPUT);
  //digitalWrite(PIN2, HIGH);
  pinMode(LEFT_BTN, INPUT);
  pinMode(RIGHT_BTN, INPUT);
  pinMode(LEFT_END, INPUT);
  pinMode(RIGHT_END, INPUT);

  // load runtime variables
  //loadFromEeprom();
  temp[0] = 6;
  temp[1] = 30;
  temp[2] = 18;
  temp[3] = 38;
  temp[4] = 2;
  temp[5] = 20;
  
  // serial input buffer
  inputString.reserve(20);
  
  // set alarms
  handleMoving();
  setAlarms();
}



/////////////////////////////////// LOOP

void loop() {
  
  handleMoving();
  
  if (!left && !right && eventTime == 0) { // movement is priority!
    
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

  Alarm.delay(100);
  
  // lets check the boundaries
  /*leftEdge = parseAnalog(END1);
  rightEdge = parseAnalog(END2);
  leftBtn = parseAnalog(BTN1);
  rightBtn = parseAnalog(BTN2);*/
  leftEdge = digitalRead(LEFT_END);
  rightEdge = digitalRead(RIGHT_END);
  leftBtn = digitalRead(LEFT_BTN);
  rightBtn = digitalRead(RIGHT_BTN);
  
  /*Serial.print(leftEdge);
  Serial.print(" ");
  Serial.print(rightEdge);
  Serial.print(" ");
  Serial.print(leftBtn);
  Serial.print(" ");
  Serial.println(rightBtn);*/
  
  if (left) {
    if (leftEdge && millis() < endTime) {
      //Serial.println("LEFT");
      digitalWrite(MOVE_LEFT, HIGH);
    } else if (!leftBtn || !leftEdge) { // if button isn't still pressed or has reached the end
      digitalWrite(MOVE_LEFT, LOW);
      left = false;
      eventTime = millis() - eventTime;
    }
  } else if (right) { 
    if (rightEdge && millis() < endTime) {
      //Serial.println("RIGHT");
      digitalWrite(MOVE_RIGHT, HIGH);
    } else if (!rightBtn || !rightEdge) { // if button isn't still pressed or has reached the end
      digitalWrite(MOVE_RIGHT, LOW);
      right = false;
      eventTime = millis() - eventTime;
    }
  } else if (eventTime > 0) {
    String str = "Last action time: ";
    str += eventTime;
    str += " ms.";
    log("DEBUG", str);
    eventTime = 0;
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
    eventTime = millis();
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
    eventTime = millis();
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
  String str = "Returned: ";
  switch (command) {
    case '\0':
      // do nothing
      break;
    case 'C':
      log("DEBUG", "Progress calculation command requested!");
      str += calculateProgress();
      log("DEBUG", str);
      break;
    case 'R':
      log("DEBUG", "Read command requested!");
      loadFromEeprom();
      break;
    case 'W':
      log("DEBUG", "Write command requested!");
      writeToEeprom();
      break;
    case 'P':
      log("DEBUG", "Permanent time set command requested!");
      setPermTime(stringToTime(inputString));
      break;
    case 'T':
      log("DEBUG", "Temporary time set command requested!");
      setTempTime(stringToTime(inputString));
      break;
    default:
      String str = "Unknown command \'";
      str += command;
      str += "\' requested!";
      log("DEBUG", str);
  }
}

int calculateProgress() {
  int progress = 0; // return variable
  time_t current = now(); // save current time
  tmElements_t elements; // create start time for comparsion
  breakTime(current, elements); // break time into elements
  elements.Second = 0; // set set start time
  elements.Hour = temp[0];
  elements.Minute = temp[1];
  time_t start = makeTime(elements); // convert back to timestamp
  
  if (current > start) { // if the cycle has started today
    elements.Hour = temp[2]; // reuse elements to create stop time timestamp
    elements.Minute = temp[3];
    time_t stop = makeTime(elements); // make the timestamp
    if (current < stop) {
      progress = (current-start)*100/(stop-start); // calculate progress
    } else {
      progress = 100;
    }
  } else {
    //breakTime(nextMidnight(current), elements); // lets make an elements object holding next day's date
    elements.Hour = temp[4]; // set the rewind time
    elements.Minute = temp[5];
    time_t rewind = makeTime(elements); // make the timestamp
    if (current < rewind) {
      progress = 100;
    }
  }
  String str = "Current progress: ";
  str += progress;
  str += "%.";
  log("DEBUG", str);
  return progress;
}


/////////////////////////////////// LOGGING

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



/////////////////////////////////// STORING VARIABLES

void loadFromEeprom() {
  String str = "READ EEPROM";
  for (byte i = 0; i < EE_RUNTIME_VARS; i++) {
    temp[i] = EEPROM.read(i);
    str += " ";
    str += i;
    str += ":";
    str += temp[i];
  }
  
  // read interval 
  interval = EEPROM.read(EE_INTERVAL_H) << 8;
  interval |= EEPROM.read(EE_INTERVAL_L);
  str += " 6-7: ";
  str += interval;
  
  // read stepTime
  stepTime = EEPROM.read(EE_STEP_TIME_H) << 8;
  stepTime |= EEPROM.read(EE_STEP_TIME_L);
  str += " 8-9: ";
  str += stepTime;
  log("DEBUG", str);
}

void writeToEeprom() {
  String str = "WRITE EEPROM";
  for (int i = 0; i < EE_RUNTIME_VARS; i++) {
    EEPROM.write(i, temp[i]);
    str += " ";
    str += i;
    str += ":";
    str += temp[i];
  }
  
  interval = 10; // seconds
  EEPROM.write(EE_INTERVAL_H, highByte(interval));
  EEPROM.write(EE_INTERVAL_L, lowByte(interval));
  str += " 6-7: ";
  str += interval;
  
  stepTime = 5000; // milliseconds
  EEPROM.write(EE_STEP_TIME_H, highByte(stepTime));
  EEPROM.write(EE_STEP_TIME_L, lowByte(stepTime));
  str += " 8-9: ";
  str += stepTime;
  log("DEBUG", str);
}


/////////////////////////////////// ALARMS

void setAlarms() {
  log("INFO", "Setting alarms!");
  Alarm.alarmRepeat(temp[0], temp[1], 0, MorningAlarm); // first in the morning
  Alarm.alarmRepeat(temp[2], temp[3], 0, NightAlarm); // stop at night
  Alarm.alarmRepeat(temp[4], temp[5], 0, RewindAlarm); // rewind
  time_t time = now();
  /*if () {
    Alarm.timerOnce(5, MovingAlarm); // in case the cycle has started at the time of restarting
  }*/
}

void MorningAlarm() {
  log("INFO", "Good morning!");
  Alarm.timerOnce(5, MovingAlarm);
  //MovingAlarm();
  //Alarm.enable(alarmId);
}

void MovingAlarm() {
  alarmId = Alarm.getTriggeredAlarmId();
  Serial.print("alarm:");
  Serial.println(alarmId);
  if (rightEdge) {
    log("INFO", "This just moved!!");
    moveRight(stepTime); // move it!
    Alarm.timerOnce(interval, MovingAlarm);
  } else {
    log("INFO", "Cycle has reached the end.");
    NightAlarm();
  }
}

void NightAlarm() {
  log("INFO", "Good night!");
  Alarm.disable(alarmId);
}

void RewindAlarm() {
  log("INFO", "Rewind.");
  moveLeft(MAX_TIME); // turn it for 30 seconds or until reaches end
}



/////////////////////////////////// TIME

void setPermTime(unsigned long t) {
  if (t > 0) {
    RTC.set(t);
    setTime(t);
  }
  String str = "Time set to ";
  str += t;
  log("INFO", str);
}

void setTempTime(unsigned long t) {
  if (t > 0) {
    setTime(t);
  }
  String str = "Time temporarly set to ";
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
    if (inChar == '\n') {
      stringComplete = true;
      inChar = '\0'; // nullchar (end of string)
    }
    inputString += inChar;
  }
}


