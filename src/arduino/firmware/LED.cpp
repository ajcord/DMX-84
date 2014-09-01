/**
 * DMX-84
 * LED indicator code
 *
 * This file contains the code for managing the LED.
 *
 * Last modified August 10, 2014
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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

#include "LED.h"
#include "firmware.h"
#include "status.h"

/******************************************************************************
 * Internal constants
 ******************************************************************************/

#define MILLISECONDS_PER_BLINK    100

/******************************************************************************
 * Internal function prototypes
 ******************************************************************************/


/******************************************************************************
 * Internal global variables
 ******************************************************************************/

static uint32_t ledPattern = 0;
static uint32_t ledDuration = 1;

static uint32_t previousPattern = 0;
static uint32_t previousDuration = 0;

/******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * initLED - Initializes the LED hardware and pattern.
 *
 * This function should be called at power up.
 */
void initLED(void) {
  pinMode(LED_PIN, OUTPUT);
  ledPattern = NORMAL_LED_PATTERN;
  ledDuration = NORMAL_LED_DURATION;
}

/**
 * blinkLED - Blinks the LED according to the previously set pattern.
 *
 * This function should be called at least 10 times per second.
 */
void blinkLED(void) {
  if (millis() % MILLISECONDS_PER_BLINK) { //Only change state every 100ms
    return;
  }

  //Get the bit for the new state
  uint8_t bit = (millis() / MILLISECONDS_PER_BLINK) % ledDuration;
  bool newState = (ledPattern >> bit) & 0x0001;

  //Turn the LED on or off
  digitalWrite(LED_PIN, newState);
}

/**
 * chooseLEDPattern - Chooses the pattern to display based on the status flags.
 */
void chooseLEDPattern(void) {
  if (testStatus(DEBUG_STATUS)) {
    ledPattern = DEBUG_LED_PATTERN;
    ledDuration = DEBUG_LED_DURATION;
  } else if (testStatus(ERROR_STATUS)) {
    ledPattern = ERROR_LED_PATTERN;
    ledDuration = ERROR_LED_DURATION;
  } else if (testStatus(SENT_SHUT_DOWN_WARNING_STATUS)) {
    ledPattern = SOS_LED_PATTERN;
    ledDuration = SOS_LED_DURATION;
  } else { //All systems nominal
    ledPattern = NORMAL_LED_PATTERN;
    ledDuration = NORMAL_LED_DURATION;
  }
}
