/**
 * DMX-84
 * LED indicator code
 *
 * This file contains the code for managing the LED.
 *
 * Last modified July 20, 2014
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