/*
  Solar panel controller
  by Raidok
 */


#define LOGLEVEL LOG_LEVEL_DEBUG

#define LEFT_BTN  2
#define RIGHT_BTN 3
#define LEFT_END  4
#define RIGHT_END 5

#define MOVE_LEFT 7
#define MOVE_RIGHT 8
#define MAX_TIME 21000

#define LEFT_BTN  2
#define RIGHT_BTN 3
#define LEFT_END  4
#define RIGHT_END 5

#define MOVE_LEFT 7
#define MOVE_RIGHT 8
#define MAX_TIME 21000



#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Logging.h>
#include <EEPROM.h>
#include <TinyWebServer.h>
#include "Controller.h"
//#include "Logger.h"


// array for temporary values, initally used for MAC-address,
// later holds start, stop and rewind times
byte temp[6] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
// command char
char command = '\0'; // nullchar (end of string)
// holds repeating alarms id for enabling/disabling
AlarmID_t alarmId;
// run interval, in seconds
unsigned long interval = 10; // TODO: remove value
// instance of my controller library
Controller controller((byte*)&temp, MOVE_LEFT, MOVE_RIGHT, LEFT_END, RIGHT_END, LEFT_BTN, RIGHT_BTN);





String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete



void setup() {
  
  Log.Init(LOG_LEVEL_VERBOSE, 9600);
  
  // clock sync
  setSyncProvider(RTC.get);
  
  Log.Debug("Startup");
  
  // begin ethernet
  //Serial.print("IP: ");
  //Ethernet.begin(temp);
  //Serial.println(Ethernet.localIP());
  
  // serial input buffer
  inputString.reserve(20);
  
  // TODO: from EEPROM
  temp[0] = 6;
  temp[1] = 30;
  temp[2] = 18;
  temp[3] = 30;
  temp[4] = 2;
  temp[5] = 2;
  
  // set alarms
  setAlarms(temp);
}



void loop() {
  
  Alarm.delay(100);
  
  if (controller.runWithBlocking()) { // movement is priority!
    
    if (stringComplete) {
      Serial.print(">>");
      Serial.println(inputString);
      if (!controller.doCommand(inputString)) {
        Log.Debug("Command unsuccessful!");
      }
      inputString = "";
      stringComplete = false;
    }
  }
}



/////////////////////////////////// ALARMS

void MovingAlarm()
{
  alarmId = Alarm.getTriggeredAlarmId();
  Serial.print("alarm:");
  Serial.println(alarmId);
  if (controller.moveRight(100)) {
    //log("INFO", "This just moved!!");
    Alarm.timerOnce(interval, MovingAlarm);
  } else {
    //log("INFO", "Cycle has reached the end.");
    NightAlarm();
  }
}


void MorningAlarm()
{
  Log.Debug("Good morning!");
  Alarm.timerOnce(5, MovingAlarm);
  //MovingAlarm();
  //Alarm.enable(alarmId);
}


void NightAlarm()
{
  Log.Debug("Good night!");
  Alarm.disable(alarmId);
}


void RewindAlarm()
{
  Log.Debug("Rewind.");
  controller.moveLeft(MAX_TIME); // turn it for 30 seconds or until reaches end
}


void setAlarms(byte* temp)
{
  //log("INFO", "Setting alarms!");
  Alarm.alarmRepeat(temp[0], temp[1], 0, MorningAlarm); // first in the morning
  Alarm.alarmRepeat(temp[2], temp[3], 0, NightAlarm); // stop at night
  Alarm.alarmRepeat(temp[4], temp[5], 0, RewindAlarm); // rewind
  //time_t time = now();
  if (controller.calculateProgress() < 100) {
    //Alarm.timerOnce(5, MovingAlarm); // in case the cycle has started at the time of restarting
  }
}



/////////////////////////////////// SERIAL

void serialEvent() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
      return;
      //inChar = '\0'; // nullchar (end of string)
    }
    inputString += inChar;
  }
}

