/**
 * DMX-84
 * Arduino firmware header
 *
 * This file contains the external defines and prototypes for the main firmware.
 *
 * Last modified July 13, 2014
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

//Compile-time options
#define AUTO_SHUT_DOWN_ENABLED      1
#define SERIAL_DEBUG_ENABLED        1
//LED mode. Uncomment one of the following LED_MODEs:
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