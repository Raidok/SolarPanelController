/*
  Solar panel controller
  Created by Raido Kalbre, May 2012.
  Licensed under GPL v3.
 */


#define LOGLEVEL LOG_LEVEL_DEBUG

#include <SPI.h>
#include <Ethernet.h>
#include <Time.h>
#include <TimeAlarms.h>
#include <Wire.h>
#include <DS1307RTC.h>
#include <Logging.h>
#include <EEPROM.h>
#include "Controller.h"


// array for temporary values, initally used for MAC-address,
// later holds start, stop and rewind times
byte temp[6] = { 0x90, 0xA2, 0xDA, 0x00, 0xF8, 0x03 };
// command char
char command = '\0'; // nullchar (end of string)
// holds repeating alarms id for enabling/disabling
AlarmID_t alarmId;
// instance of my controller library
Controller controller((byte*)&temp/*, MOVE_LEFT, MOVE_RIGHT, LEFT_END, RIGHT_END, LEFT_BTN, RIGHT_BTN*/);
// WebServer
EthernetServer server(80);
// variable for client connection
EthernetClient client;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete
boolean blocked = false;         // just in case

const char headerStr[] PROGMEM = 
  "HTTP/1.1 200 OK\n" \
  "Content-Type: text/html\n" \
  "Access-Control-Allow-Origin: *\n" \
  "Access-Control-Allow-Headers: Content-Type, X-Requested-With\n" \
  "Access-Control-Allow-Methods: GET, POST, OPTIONS\n\n)]}',\n";


void setup() {
  
  Log.Init(LOG_LEVEL_VERBOSE, 9600);
  
  // clock sync
  setSyncProvider(RTC.get);
  
  // begin ethernet
  Log.Debug("Starting webserver");
  Ethernet.begin(temp);
  Serial.print("IP: ");
  Serial.println(Ethernet.localIP());
  server.begin();
  
  // serial input buffer
  inputString.reserve(20);
  
  byte* t = controller.getTimes();
  for (int i = 0; i < 6; i++)
  {
    temp[i] = t[i];
  }
  
  // set alarms
  setAlarms(temp);
}



void loop() {
  
  Alarm.delay(100);
  
  if (controller.runWithBlocking()) { // movement is priority!
    
    if (isConnected()) {
      sendProgStr(headerStr);
      //inputString += '\0';
      stringComplete = true;
    }
    
    if (stringComplete) {
      String ret = controller.doCommand(inputString);
      if (ret != NULL) {
        client.println(ret);
      }
      inputString = "";
      stringComplete = false;
      close();
    }
  }
}



/////////////////////////////////// ALARMS

void MovingAlarm()
{
  alarmId = Alarm.getTriggeredAlarmId();
  if (!blocked && controller.moveRight(100)) {
    Alarm.timerOnce(controller.getInterval(), MovingAlarm);
  } else {
    NightAlarm();
  }
}


void MorningAlarm()
{
  Log.Debug("Good morning!");
  Alarm.timerOnce(5, MovingAlarm);
}


void NightAlarm()
{
  Log.Debug("Good night!");
  Alarm.disable(alarmId);
}


void RewindAlarm()
{
  Log.Debug("Rewind.");
  if (!blocked) controller.moveLeft(10000); // turn to the other end
}


void setAlarms(byte* temp)
{
  //log("INFO", "Setting alarms!");
  Alarm.alarmRepeat(temp[0], temp[1], 0, MorningAlarm); // first in the morning
  Alarm.alarmRepeat(temp[2], temp[3], 0, NightAlarm); // stop at night
  Alarm.alarmRepeat(temp[4], temp[5], 0, RewindAlarm); // rewind
  //time_t time = now();
  if (controller.calculateProgress() < 100) {
    Alarm.timerOnce(5, MovingAlarm); // in case the cycle has started at the time of restarting
  }
}



/////////////////////////////////// ETHERNET

boolean isConnected()
{
  client = server.available();
  if (client)
  {
    char c, pc = '\0';
    boolean ok = false;
    int i = 0;
    while (client.connected())
    {
      if (client.available())
      {
        char c = client.read();
        
        if (i == 0) // workaround for OPTIONS request
        {
          if (c == 'O')
          {
            inputString = "";
            return true;
          }
          i++;
        }
        
        if (c == '/') // ignore OPTIONS request
        {
          ok = true;
        }
        else if (ok)
        {
          
          if (c == ' ')
          {
            if (inputString.charAt(0) == 'T') // time change blocks alarms
            {
              Serial.println("kell!");
              blocked = true;
            }
            return true;
          }
          else
          {
            inputString += c;
          }
          
        }
        pc = c;
      }
    }
  }
  return false;
}

void sendProgStr(const prog_char str[])
{
  char c;
  if(!str) return;
  while((c = pgm_read_byte(str++)))
    client.print(c);
}

void close()
{
  delay(1);
  client.stop();
}

