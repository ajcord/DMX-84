/**
 * DMX-84
 * Status and error reporting code
 *
 * This file contains the code for managing the system status and error flags.
 *
 * Last modified July 20, 2014
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

#include "status.h"
#include "LED.h"

/******************************************************************************
 * Internal constants
 ******************************************************************************/


/******************************************************************************
 * Internal function prototypes
 ******************************************************************************/


/******************************************************************************
 * Internal global variables
 ******************************************************************************/

static uint8_t errorFlags = 0; //Any errors that have not yet been reported
static uint8_t statusFlags = 0; //The current firmware status

/******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * setError - Sets a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to set
 */
void setStatus(uint8_t status) {
  statusFlags |= status;
  chooseLEDPattern();
}

/**
 * clearStatus - Clears a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to clear
 */
void clearStatus(uint8_t status) {
  statusFlags &= ~status;
  chooseLEDPattern();
}

/**
 * toggleStatus - Toggles a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to toggle
 */
void toggleStatus(uint8_t status) {
  statusFlags ^= status;
  chooseLEDPattern();
}

/**
 * testError - Tests a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to test
 * Returns:
 *    bool isSet: whether the flag is set or not
 */
bool testStatus(uint8_t status) {
  return (statusFlags & status ? true : false);
}

/**
 * getStatus - Gets the current status flags.
 *
 * Returns:
 *    uint8_t statusFlags: the current status flags
 */
uint8_t getStatus(void) {
  return statusFlags;
}

/**
 * resetStatus - Resets the status flags.
 */
void resetStatus(void) {
  statusFlags = 0;
  chooseLEDPattern();
}

/**
 * setError - Sets the error status and changes the LED pattern.
 *
 * Parameter:
 *    uint8_t error: the new error code
 */
void setError(uint8_t error) {
  errorFlags |= error;
  setStatus(ERROR_STATUS);
  chooseLEDPattern();
}

/**
 * clearError - Clears an error flag.
 *
 * Parameter:
 *    uint8_t error: the error flag to clear
 */
void clearError(uint8_t error) {
  errorFlags &= ~error;
  chooseLEDPattern();
}

/**
 * toggleError - Toggles an error flag.
 *
 * Parameter:
 *    uint8_t error: the error flag to toggle
 */
void toggleError(uint8_t error) {
  errorFlags ^= error;
  chooseLEDPattern();
}

/**
 * testError - Tests an error flag.
 *
 * Parameter:
 *    uint8_t error: the error flag to test
 * Returns:
 *    bool isSet: whether the flag is set or not
 */
bool testError(uint8_t error) {
  return (errorFlags & error ? true : false);
}

/**
 * getErrors - Gets the current error flags.
 *
 * Returns:
 *    uint8_t errorFlags: the current error flags
 */
uint8_t getErrors(void) {
  return errorFlags;
}

/**
 * resetErrors - Resets the error flags and clears the error status.
 */
void resetErrors(void) {
  errorFlags = 0;
  clearStatus(ERROR_STATUS);
  chooseLEDPattern();
}