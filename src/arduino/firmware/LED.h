/**
 * DMX-84
 * LED indicator header
 *
 * This file contains the external defines and prototypes for managing the LED.
 *
 * Last modified July 13, 2014
 */

#ifndef LED_H
#define LED_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <inttypes.h>

#include "firmware.h"

/******************************************************************************
 * External constants
 ******************************************************************************/

//Each pattern bit is 0.1 seconds.
//Duration is the repeat time in tenths of seconds.
#ifdef LED_MODE_HEARTBEAT
//Heartbeat blink mode
#define NORMAL_LED_PATTERN              0b000001111111111
#define NORMAL_LED_DURATION             15
#else
//Solid blink mode
#define NORMAL_LED_PATTERN              0b1
#define NORMAL_LED_DURATION             1
#endif

#define ERROR_LED_PATTERN               0b01
#define ERROR_LED_DURATION              2

#define SOS_LED_PATTERN                 0b00000101010001110111011100010101
#define SOS_LED_DURATION                32

#define DEBUG_LED_PATTERN               0b0000000001
#define DEBUG_LED_DURATION              10

/******************************************************************************
 * External global variables
 ******************************************************************************/


/******************************************************************************
 * External function prototypes
 ******************************************************************************/

void initLED(void);
void blinkLED(void);
void setLEDPattern(uint32_t pattern, uint8_t duration);
void restoreLastPattern(void);

#endif