/**
 * DMX-84
 * LED indicator header
 *
 * This file contains the external defines and prototypes for managing the LED.
 *
 * Last modified November 4, 2014
 *
 *
 * Copyright (C) 2014  Alex Cordonnier
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
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

//Each pattern bit is 0.1 seconds. The LSbit occurs first.
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
 * Class definition
 ******************************************************************************/

class LEDClass {
    public:
        void init(void);
        void update(void);
        void choosePattern(void);

    private:
        uint32_t ledPattern;
        uint32_t ledDuration;

        uint32_t previousPattern;
        uint32_t previousDuration;
};

extern LEDClass LED;

#endif
