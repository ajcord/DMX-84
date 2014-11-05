/**
 * DMX-84
 * Status and error reporting code
 *
 * This file contains the code for managing the system status and error flags.
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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

#include "status.h"
#include "LED.h"

/******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * set - Sets a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to set
 */
void StatusClass::set(uint8_t status) {
  flags |= status;
  LED.choosePattern();
}

/**
 * clear - Clears a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to clear
 */
void StatusClass::clear(uint8_t status) {
  flags &= ~status;
  LED.choosePattern();
}

/**
 * toggle - Toggles a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to toggle
 */
void StatusClass::toggle(uint8_t status) {
  flags ^= status;
  LED.choosePattern();
}

/**
 * test - Tests a status flag.
 *
 * Parameter:
 *    uint8_t status: the status flag to test
 * Returns:
 *    bool isSet: whether the flag is set or not
 */
bool StatusClass::test(uint8_t status) {
  return (flags & status ? true : false);
}

/**
 * get - Gets the current status flags.
 *
 * Returns:
 *    uint8_t flags: the current status flags
 */
uint8_t StatusClass::get(void) {
  return flags;
}

/**
 * reset - Resets the status flags.
 */
void StatusClass::reset(void) {
  flags = 0;
  LED.choosePattern();
}

/**
 * set - Sets the error status and changes the LED pattern.
 *
 * Parameter:
 *    uint8_t error: the new error code
 */
void ErrorClass::set(uint8_t error) {
  flags |= error;
  Status.set(ERROR_STATUS);
  LED.choosePattern();
}

/**
 * clear - Clears an error flag.
 *
 * Parameter:
 *    uint8_t error: the error flag to clear
 */
void ErrorClass::clear(uint8_t error) {
  flags &= ~error;
  //If there are no more remaining errors, clear the error status.
  if (!flags) {
    Status.clear(ERROR_STATUS);
  }
  LED.choosePattern();
}

/**
 * toggle - Toggles an error flag.
 *
 * Parameter:
 *    uint8_t error: the error flag to toggle
 */
void ErrorClass::toggle(uint8_t error) {
  flags ^= error;
  //If there are remaining errors, set the error status. Else clear it.
  if (flags) {
    Status.set(ERROR_STATUS);
  } else {
    Status.clear(ERROR_STATUS);
  }
  LED.choosePattern();
}

/**
 * reset - Resets the error flags and clears the error status.
 */
void ErrorClass::reset(void) {
  flags = 0;
  Status.clear(ERROR_STATUS);
  LED.choosePattern();
}

StatusClass Status; //Create a public Status instance
ErrorClass Error; //Create a public Error instance
