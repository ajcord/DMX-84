/**
 * DMX-84
 * LED indicator code
 *
 * This file contains the code for managing the LED.
 *
 * Last modified July 13, 2014
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

#include "LED.h"
#include "firmware.h"

/******************************************************************************
 * Internal constants
 ******************************************************************************/


/******************************************************************************
 * Internal function prototypes
 ******************************************************************************/


/******************************************************************************
 * Internal global variables
 ******************************************************************************/

uint32_t ledPattern = 0;
uint32_t ledDuration = 1;

uint32_t previousPattern;
uint32_t previousDuration;

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
  setLEDPattern(NORMAL_LED_PATTERN, NORMAL_LED_DURATION);
}

/**
 * blinkLED - Blinks the LED according to the previously set pattern.
 *
 * This function should be called at least 10 times per second.
 */
void blinkLED(void) {
  if (millis() % 100) { //Only change state every 100ms
    return;
  }

  //Get the bit for the new state
  uint8_t bit = (millis() / 100) % ledDuration;
  bool newState = (ledPattern >> bit) & 0x0001;

  //Turn the LED on or off
  digitalWrite(LED_PIN, newState);
}

/**
 * setLEDPattern - Sets the blink pattern of the LED.
 *
 * Parameters:
 *    uint32_t pattern: the LED blink pattern. 1 bit = 0.1 seconds. 1 is on.
 *    uint8_t duration: the number of significant pattern bits
 */
void setLEDPattern(uint32_t pattern, uint8_t duration) {
  //Save the current pattern so it can be restored later
  previousPattern = ledPattern;
  previousDuration = ledDuration;

  //Set the new pattern
  ledPattern = pattern;
  ledDuration = duration;
}

/**
 * restoreLastPattern - Restores the previous blink pattern.
 */
void restoreLastPattern(void) {
  ledPattern = previousPattern;
  ledDuration = previousDuration;
}