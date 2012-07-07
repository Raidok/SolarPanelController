/*
  Controller.h - Library for controlling solar panels by rotating them.
  Created by Raido Kalbre, July 4, 2012.
  Licensed under GPL v3.
*/

#ifndef Controller_h
#define Controller_h

#include "Arduino.h"
#include <Logging.h>


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


class Controller
{
  public:
    Controller(byte* times, int leftOutputPin, int rightOutputPin, int leftEdgePin, int rightEdgePin, int leftButtonPin, int rightButtonPin);
    boolean runWithBlocking();
    boolean doCommand(String string);
    boolean isMoving();
    boolean moveLeft(int amount);
    boolean moveRight(int amount);
    void setAlarms(byte startH, byte startM, byte stopH, byte stopM, byte rewindH, byte rewindM);
    int calculateProgress();
    void setProperty(int key, byte value);
    void setPropertyWord(int key1, int key2, int value);
    byte getProperty(int key);
    int getPropertyWord(int key1, int key2);
  private:
    Logging* _logger;

};

#endif
