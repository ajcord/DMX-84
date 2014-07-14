/**
 * DMX-84
 * Status and error reporting header
 *
 * This file contains the external defines and prototypes for managing the
 * system status and error flags.
 *
 * Last modified July 13, 2014
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
//#define SERIAL_DIAGNOSTICS_STATUS       0x40
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