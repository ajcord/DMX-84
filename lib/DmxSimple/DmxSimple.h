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

#ifndef DmxSimple_h
#define DmxSimple_h

#include <inttypes.h>

#if RAMEND <= 0x4FF
#define DMX_SIZE 128
#else
#define DMX_SIZE 512
#endif

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
};
extern DmxSimpleClass DmxSimple;

#endif
