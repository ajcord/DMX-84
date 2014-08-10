/**
 * DMX-84
 * Status and error reporting code
 *
 * This file contains the code for managing the system status and error flags.
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