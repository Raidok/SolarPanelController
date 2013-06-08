/*
  Controller.h - Library for controlling solar panels by rotating them.
  Created by Raido Kalbre, July 4, 2012.
  Licensed under GPL v3.
*/

#include "Arduino.h"
#include "Controller.h"
#include <EEPROM.h>
#include <Time.h>

#ifndef DS1307RTC_h
#include <DS1307RTC.h>
#endif

#define LEFT_BTN  2
#define RIGHT_BTN 3
#define LEFT_END  4
#define RIGHT_END 5

#define MOVE_LEFT 7
#define MOVE_RIGHT 8

// this array holds start, stop and rewind times
byte* _times;
// run interval, in seconds
unsigned long interval; // TODO: remove value
// lenght of one step, in milliseconds
unsigned long stepTime; // TODO: remove value
// time when current movement should end, in milliseconds
unsigned long endTime;
// for measuring how long current movement lasted
//unsigned long eventTime;


boolean left = false;
boolean right = false;
boolean leftEdge = false;
boolean rightEdge = false;
boolean leftBtn = false;
boolean rightBtn = false;


//int _leftOutputPin, _rightOutputPin, _leftEdgePin, _rightEdgePin, _leftButtonPin, _rightButtonPin;


Controller::Controller(byte *temp/*, int leftOutputPin, int rightOutputPin, int leftEdgePin, int rightEdgePin, int leftButtonPin, int rightButtonPin*/)
{
  _times = temp;
  
  // set pinmodes
  
  pinMode(MOVE_LEFT, OUTPUT);
  //digitalWrite(leftOutputPin, HIGH);
  //_leftOutputPin = leftOutputPin;
  
  pinMode(MOVE_RIGHT, OUTPUT);
  //digitalWrite(rightOutputPin, HIGH);
  //_rightOutputPin = rightOutputPin;
  
  pinMode(LEFT_BTN, INPUT);
  //_leftButtonPin = leftButtonPin;
  
  pinMode(RIGHT_BTN, INPUT);
  //_rightButtonPin = rightButtonPin;
  
  pinMode(LEFT_END, INPUT);
  //_leftEdgePin = leftEdgePin;
  
  pinMode(RIGHT_END, INPUT);
  //_rightEdgePin = rightEdgePin;
  
  getInterval();
  getStepSize();
}


boolean Controller::isMoving()
{
  return !(left || right); // true only if not moving
}


void Controller::stop()
{
  left = false;
  right = false;
}


boolean Controller::runWithBlocking()
{
  leftEdge = digitalRead(LEFT_END);
  rightEdge = digitalRead(RIGHT_END);
  leftBtn = digitalRead(LEFT_BTN);
  rightBtn = digitalRead(RIGHT_BTN);
  
  if (left)
  {
    if (leftEdge && millis() < endTime)
    {
      digitalWrite(MOVE_LEFT, HIGH);
    }
    else if (!leftBtn || !leftEdge) // if button isn't still pressed or has reached the end
    {
      digitalWrite(MOVE_LEFT, LOW);
      left = false;
      //eventTime = millis() - eventTime;
    }
  }
  else if (right)
  { 
    if (rightEdge && millis() < endTime)
    {
      digitalWrite(MOVE_RIGHT, HIGH);
    }
    else if (!rightBtn || !rightEdge) // if button isn't still pressed or has reached the end
    {
      digitalWrite(MOVE_RIGHT, LOW);
      right = false;
      //eventTime = millis() - eventTime;
    }
  }
  /*else if (eventTime != 0)
  {
    Log.Debug("Last action time: %d ms", eventTime);
    eventTime = 0;
  }*/
  else
  {
    if (leftBtn)
    {
      Log.Debug("Left button pressed");
      moveLeft(25);
    }
    else if (rightBtn)
    {
      Log.Debug("Right button pressed");
      moveRight(25);
    }
  }
  return isMoving()/* && eventTime == 0*/;
}

void Controller::failMsg()
{
  Log.Debug("Moving failed! Left: %T Right: %T LeftEdge: %T RightEdge: %T", left, right, leftEdge, rightEdge);
}

boolean Controller::moveLeft(int amount)
{
  unsigned long time = stepTime * amount / 100;
  
  stop();
  
  Log.Debug("Moving left for %d ms", time);
  
  if (!(left || right) && leftEdge)
  {
    left = true;
    endTime = millis() + time;
    //eventTime = millis();
    return true;
  }
  failMsg();
  return false;
}


boolean Controller::moveRight(int amount)
{
  unsigned long time = stepTime * amount / 100;
  
  stop();
  
  Log.Debug("Moving right for %d ms", time);
  
  if (!(left || right) && rightEdge)
  {
    right = true;
    endTime = millis() + time;
    //eventTime = millis();
    return true;
  }
  failMsg();
  return false;
}



//
// GETTERS
//

byte Controller::getProperty(int key)
{
  return EEPROM.read(key);
}


int Controller::getPropertyWord(int key1, int key2)
{
  int temp;
  temp = EEPROM.read(key1) << 8;
  return temp | EEPROM.read(key2);
}

byte* Controller::getTimes()
{
  for (int i = 0, j = EE_START_H; j < EE_RUNTIME_VARS; i++, j++)
  {
    _times[i] = getProperty(i); // read from EEPROM
  }
  return _times;
}

String Controller::getStatus()
{
  time_t t = now(); // save current time
  char buffer[200];
  sprintf(buffer, "{\"status\":%d,\"start\":\"%02d:%02d\",\"stop\":\"%02d:%02d\",\"rewind\":\"%02d:%02d\",\"date\":\"%02d.%02d.%u\",\"time\":\"%02d:%02d:%02d\",\"interval\":%u,\"step\":",
    calculateProgress(),_times[0],_times[1],_times[2],_times[3],_times[4],_times[5],
    day(t), month(t), year(t), hour(t), minute(t), second(t), interval); // max 16 arguments :(
  String s = buffer;
  s += stepTime;
  s += "}";
  return s;
}

long Controller::getInterval()
{
  interval = getPropertyWord(EE_INTERVAL_H, EE_INTERVAL_L);
  return interval;
}

long Controller::getStepSize()
{
  stepTime = getPropertyWord(EE_STEP_TIME_H, EE_STEP_TIME_L);
  return stepTime;
}


//
// SETTERS
//

boolean Controller::setTimes(String string)
{
  if (string.length() < 12)
  {
    return false;
  }
  
  byte temp[6] = {0,0,0,0,0,0};
  
  // parsing
  for (int i = 0; i < 6; i++)
  {
    for (int j = 0; j < 2; j++)
    {
      temp[i] = 10 * temp[i] + string.charAt(2*i+j+1) - '0';
    }
  }
  
  // saving
  for (int i = 0, j = EE_START_H; j < EE_RUNTIME_VARS; i++, j++)
  {
    _times[i] = temp[i]; // save to the array
    setProperty(j, temp[i]); // save to EEPROM
  }
  
  return true;
}


void Controller::setProperty(int key, byte value)
{
  EEPROM.write(key, value);
}


void Controller::setPropertyWord(int key1, int key2, int value)
{
  setProperty(key1, highByte(value));
  setProperty(key2, lowByte(value));
}


void Controller::setTimestamp(unsigned long t)
{
  if (t > 0)
  {
#ifdef DS1307RTC_h
    Serial.print("new timestamp:");
    Serial.println(t);
    RTC.set(t);
#endif
    setTime(t);
  }
}


void Controller::setInterval(long _interval)
{
  interval = _interval;
  setPropertyWord(EE_INTERVAL_H, EE_INTERVAL_L, _interval);
}


void Controller::setStepSize(long _stepSize)
{
  stepTime = _stepSize;
  setPropertyWord(EE_STEP_TIME_H, EE_STEP_TIME_L, _stepSize);
}



//
// UTILITY
//

time_t stringToTime(String str)
{
  time_t pctime = 0;
  for (int i = 0; i < str.length(); i++)
  {
    char c = str.charAt(i);
    
    if( c >= '0' && c <= '9')
    {
      pctime = (10 * pctime) + (c - '0') ; // convert digits to a number
    }
  }
  return pctime;
}


int Controller::calculateProgress()
{
  int progress = 0; // return variable
  time_t current = now(); // save current time
  tmElements_t elements; // create start time for comparsion
  breakTime(current, elements); // break time into elements
  elements.Second = 0; // set set start time
  elements.Hour = (int)*_times;
  elements.Minute = (int)*(_times+1);
  time_t start = makeTime(elements); // convert back to timestamp
  
  if (current > start) // if the cycle has started today
  {
    elements.Hour = (unsigned int)*(_times+2); // reuse elements to create stop time timestamp
    elements.Minute = (unsigned int)*(_times+3);
    
    time_t stop = makeTime(elements); // make the timestamp
    
    if (current < stop) {
      progress = (current-start)*100/(stop-start); // calculate progress
    }
    else
    {
      progress = 100;
    }
  }
  else
  {
    //breakTime(nextMidnight(current), elements); // lets make an elements object holding next day's date
    elements.Hour = (unsigned int)*(_times+4); // set the rewind time
    elements.Minute = (unsigned int)*(_times+5);
    
    time_t rewind = makeTime(elements); // make the timestamp
    
    if (current < rewind)
    {
      progress = 100;
    }
  }
  return progress;
}



/*
 *  COMMAND CENTRE
 */
String Controller::doCommand(String string)
{
  //Log.Debug("Command: %s", string);
  
  String s = NULL;
  int i = (int) stringToTime(string);
  char command;
  
  if (string.length() > 0)
  {
    command = string.charAt(0);
  }
  else
  {
    return s;
  }
  
  switch (command)
  {
    case 'A': // status request only
      break;
    case 'L': // move left
      moveLeft(i);
      break;
    case 'R': // move right
      moveRight(i);
      break;
    case 'I': // update interval
      setInterval(i);
      break;
    case 'J': // update step size
      setStepSize(i);
      break;
    case 'S': // update start/end/rewind times
      setTimes(string);
      break;
    case 'T': // update date and time
      setTimestamp(stringToTime(string));
      break;
    default: // returns NULL
      return s;
  }
  s = getStatus();
  
  return s;
}
