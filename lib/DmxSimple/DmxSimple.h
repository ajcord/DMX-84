/**
 * DmxSimple - A simple interface to DMX.
 *
 * Copyright (c) 2008-2009 Peter Knight, Tinker.it! All rights reserved.
 */

/** 
 * Some alterations by Andreas Rothenwänder
 *   
 *   * added comments
 *   * write   : added return value - returns now previous setting
 *   * usePin  : minor bug corrected
 *   * modulate: new function - relative value change of given channel (returns new value)
 *   * getValue: new function - returns current value of given channel
 */

/**
 * Some other alterations by Alex Cordonnier (ajcord)
 *
 *    * Added support for digital blackout
 *    * Made dmxBuffer available externally
 *
 *    Alterations commented as // (ajcord)
 */

#ifndef DmxSimple_h
#define DmxSimple_h

#include <inttypes.h>

//#if RAMEND <= 0x4FF // (ajcord) Commented out because it was causing dmxBuffer to resize
//#define DMX_SIZE 128
//#else
#define DMX_SIZE 512
//#endif

class DmxSimpleClass
{
  public:
    void maxChannel(int);		// set number of channels that should be transmitted permanently (as little as possible to save time)
    uint8_t write(int, uint8_t);    // set new value for channel
                                    //   returns previous channel value
    void usePin(uint8_t);           // set output pin for DMX signal
    uint8_t modulate(int, int);     // increase/decrease current channel value by given amount (clipped to interval [0..255])
                                    //   returns new channel value
    uint8_t getValue(int);          // returns current value of given channel
    void startDigitalBlackout();    // (ajcord) Starts a digital blackout
    void stopDigitalBlackout();     // (ajcord) Stops a digital blackout
};
extern DmxSimpleClass DmxSimple;

extern volatile uint8_t dmxBuffer[DMX_SIZE]; // (ajcord) Moved here so it is available to other files. Defined in firmware.ino

#endif
