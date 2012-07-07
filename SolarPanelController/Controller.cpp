/*
  Controller.h - Library for controlling solar panels by rotating them.
  Created by Raido Kalbre, July 4, 2012.
  Licensed under GPL v3.
*/

#include "Arduino.h"
#include "Controller.h"
#include <EEPROM.h>
#include <Time.h>

#ifdef DS1307RTC_h
#include <DS1307RTC.h>
#endif

// this array holds start, stop and rewind times
byte* _times;
// lenght of one step, in milliseconds
unsigned long stepTime = 5000; // TODO: remove value
// time when current movement should end, in milliseconds
unsigned long endTime;
// for measuring how long current movement lasted
unsigned long eventTime;
// temporary values
unsigned int count;


boolean left = false;
boolean right = false;
boolean leftEdge = false;
boolean rightEdge = false;
boolean leftBtn = false;
boolean rightBtn = false;


int _leftOutputPin, _rightOutputPin, _leftEdgePin, _rightEdgePin, _leftButtonPin, _rightButtonPin;


Controller::Controller(byte *temp, int leftOutputPin, int rightOutputPin, int leftEdgePin, int rightEdgePin, int leftButtonPin, int rightButtonPin)
{
  _times = temp;
  
  // set pinmodes
  
  pinMode(leftOutputPin, OUTPUT);
  //digitalWrite(leftOutputPin, HIGH);
  _leftOutputPin = leftOutputPin;
  
  pinMode(rightOutputPin, OUTPUT);
  //digitalWrite(rightOutputPin, HIGH);
  _rightOutputPin = rightOutputPin;
  
  pinMode(leftButtonPin, INPUT);
  _leftButtonPin = leftButtonPin;
  
  pinMode(rightButtonPin, INPUT);
  _rightButtonPin = rightButtonPin;
  
  pinMode(leftEdgePin, INPUT);
  _leftEdgePin = leftEdgePin;
  
  pinMode(rightEdgePin, INPUT);
  _rightEdgePin = rightEdgePin;
}


boolean Controller::isMoving()
{
  return !(left || right); // true only if not moving
}


boolean Controller::runWithBlocking()
{
  leftEdge = digitalRead(_leftEdgePin);
  rightEdge = digitalRead(_rightEdgePin);
  leftBtn = digitalRead(_leftButtonPin);
  rightBtn = digitalRead(_rightButtonPin);
  
  if (left)
  {
    if (leftEdge && millis() < endTime)
    {
      //Serial.println("LEFT");
      digitalWrite(_leftOutputPin, HIGH);
    }
    else if (!leftBtn || !leftEdge) // if button isn't still pressed or has reached the end
    {
      digitalWrite(_leftOutputPin, LOW);
      left = false;
      eventTime = millis() - eventTime;
    }
  }
  else if (right)
  { 
    if (rightEdge && millis() < endTime)
    {
      //Serial.println("RIGHT");
      digitalWrite(_rightOutputPin, HIGH);
    }
    else if (!rightBtn || !rightEdge) // if button isn't still pressed or has reached the end
    {
      digitalWrite(_rightOutputPin, LOW);
      right = false;
      eventTime = millis() - eventTime;
    }
  }
  else if (eventTime != 0)
  {
    Log.Debug("Last action time: %d ms", eventTime);
    eventTime = 0;
  }
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
  return isMoving() && eventTime == 0; // FIXME
}


boolean Controller::moveLeft(int amount)
{
  unsigned long time = stepTime * amount / 100;
  
  Log.Debug("Moving left for %d ms", time);
  
  if (!(left || right) && leftEdge)
  {
    left = true;
    endTime = millis() + time;
    eventTime = millis();
    return true;
  }
  
  Log.Debug("Moving failed! Left: %T Right: %T LeftEdge: %T RightEdge: %T", time, left, right, rightEdge);
  
  return false;
}


boolean Controller::moveRight(int amount)
{
  unsigned long time = stepTime * amount / 100;
  
  Log.Debug("Moving right for %d ms", time);
  
  if (!(left || right) && rightEdge)
  {
    right = true;
    endTime = millis() + time;
    eventTime = millis();
    return true;
  }
  
  Log.Debug("Moving failed! Left: %T Right: %T LeftEdge: %T RightEdge: %T", time, left, right, rightEdge);
  
  return false;
}



//
// GETTERS
//
void getTimes() {
  
  for (int i = 0, j = EE_START_H; j < EE_RUNTIME_VARS; i++, j++) {
    _times[i] = EEPROM.read(i); // read from EEPROM
  }
}


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


//
// SETTERS
//


boolean setTimes(String string)
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
      
      if (j == 1)
      {
        Serial.print(i);
        Serial.print(" = ");
        Serial.println(temp[i]);
      }
    }
  }
  
  // saving
  for (int i = 0, j = EE_START_H; j < EE_RUNTIME_VARS; i++, j++)
  {
    _times[i] = temp[i]; // save to the array
    //EEPROM.write(j, temp[i]); // save to EEPROM
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


void setTimestamp(unsigned long t)
{
  if (t > 0)
  {
#ifdef DS1307RTC_h
    RTC.set(t);
#endif
    setTime(t);
  }
}


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
  Serial.print("0:");
  Serial.println(elements.Hour);
  elements.Minute = (int)*(_times+1);
  Serial.print("1:");
  Serial.println(elements.Minute);
  time_t start = makeTime(elements); // convert back to timestamp
  
  if (current > start) // if the cycle has started today
  {
    elements.Hour = (unsigned int)*(_times+2); // reuse elements to create stop time timestamp
    Serial.print("2:");
    Serial.println(elements.Hour);
    
    elements.Minute = (unsigned int)*(_times+3);
    Serial.print("3:");
    Serial.println(elements.Minute);
    
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
    Serial.print("4:");
    Serial.println(elements.Hour);
    
    elements.Minute = (unsigned int)*(_times+5);
    Serial.print("5:");
    Serial.println(elements.Minute);
    
    time_t rewind = makeTime(elements); // make the timestamp
    
    if (current < rewind)
    {
      progress = 100;
    }
  }
  return progress;
}


byte getProperty(byte index)
{
  
}




/*
 *  COMMAND CENTRE
 */
boolean Controller::doCommand(String string)
{
  Log.Debug("Command: %s", string);
  
  char command;
  
  if (string.length() > 0)
  {
    command = string.charAt(0);
  }
  else
  {
    return false;
  }
  
  switch (command)
  {
    case 'C':
      Log.Debug("Calculated progress is %d%", calculateProgress());
      break;
    case 'R':
      //log("DEBUG", "Read command requested!");
      //loadFromEeprom();
      break;
    case 'W':
      //log("DEBUG", "Write command requested!");
      //writeToEeprom();
      break;
    case 'P':
      //log("DEBUG", "Permanent time set command requested!");
      //setPermTime(stringToTime(inputString));
      break;
    case 'S':
      setTimes(string);
      break;
    case 'T':
      setTimestamp(stringToTime(string));
      break;
    default:
      return false;
  }
  
  return true;
}
