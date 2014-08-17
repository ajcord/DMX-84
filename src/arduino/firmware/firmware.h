/**
 * DMX-84
 * Arduino firmware header
 *
 * This file contains the external defines and prototypes for the main firmware.
 *
 * Last modified August 17, 2014
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

#ifndef FIRMWARE_H
#define FIRMWARE_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

/******************************************************************************
 * External constants
 ******************************************************************************/

//Pins and hardware
#define LED_PIN               9
#define DMX_OUT_PIN           10
#define TI_RING_PIN           4
#define TI_TIP_PIN            6

//Compile-time options:
//AUTO_SHUT_DOWN_ENABLED > 0 enables auto shutdown.
//SERIAL_DEBUG_ENABLED > 0 enables serial input and output to a PC.
//LED_MODE_* defines which LED flash pattern to use normally.
#define AUTO_SHUT_DOWN_ENABLED      1
#define SERIAL_DEBUG_ENABLED        0
//Normal LED mode. Uncomment one of the following LED_MODEs:
#define LED_MODE_HEARTBEAT
//#define LED_MODE_SOLID

/******************************************************************************
 * External global variables
 ******************************************************************************/


/******************************************************************************
 * External function prototypes
 ******************************************************************************/

void processCommand(uint8_t cmd);
void manageTimeouts();
void initShutDown(bool reset = false);
int32_t readTemp(void);

#endif