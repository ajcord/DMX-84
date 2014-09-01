/**
 * DMX-84
 * Status and error reporting header
 *
 * This file contains the external defines and prototypes for managing the
 * system status and error flags.
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

#ifndef STATUS_H
#define STATUS_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * External constants
 ******************************************************************************/

//Status flags
#define DMX_ENABLED_STATUS              0x01
#define DIGITAL_BLACKOUT_ENABLED_STATUS 0x02
#define RESTRICTED_MODE_STATUS          0x04
#define DEBUG_STATUS                    0x08
#define RECEIVED_HANDSHAKE_STATUS       0x10
#define SENT_SHUT_DOWN_WARNING_STATUS   0x20
#define SERIAL_DIAGNOSTICS_STATUS       0x40
#define ERROR_STATUS                    0x80
#define NORMAL_STATUS_MASK              0x11

//Error flags
#define DMX_DISABLED_ERROR              0x01
#define DIGITAL_BLACKOUT_ERROR          0x02
#define RESTRICTED_MODE_ERROR           0x04
#define INVALID_VALUE_ERROR             0x08
#define TIMEOUT_ERROR                   0x10
#define BAD_PACKET_ERROR                0x20
#define UNKNOWN_COMMAND_ERROR           0x40
#define UNKNOWN_ERROR                   0x80
#define DMX_ERROR_MASK                  0x03
#define COMMUNICATION_ERROR_MASK        0x78

/******************************************************************************
 * External global variables
 ******************************************************************************/


/******************************************************************************
 * External function prototypes
 ******************************************************************************/

void setStatus(uint8_t status);
void clearStatus(uint8_t status);
void toggleStatus(uint8_t status);
bool testStatus(uint8_t status);
uint8_t getStatus(void);
void resetStatus(void);

void setError(uint8_t error);
void clearError(uint8_t error);
void toggleError(uint8_t error);
bool testError(uint8_t error);
uint8_t getErrors(void);
void resetErrors(void);

#endif
