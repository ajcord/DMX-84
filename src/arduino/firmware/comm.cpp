/**
 * DMX-84
 * Communication code
 *
 * This file contains the code for communicating with the calculator and the
 * serial port.
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

/******************************************************************************
 * Includes
 ******************************************************************************/

#include "Arduino.h"

#include "comm.h"
#include "firmware.h"
#include "status.h"
#include "LED.h"

/******************************************************************************
 * Internal constants
 ******************************************************************************/

//Valid Machine ID bytes
#define MACHINE_ID_PC_82      0x02
#define MACHINE_ID_PC_83      0x03
#define MACHINE_ID_PC_84      0x23
#define MACHINE_ID_TI_84      0x73
#define MACHINE_ID_TI_82      0x82
#define MACHINE_ID_TI_83      0x83

//Which Machine ID to use
#define MACHINE_ID            MACHINE_ID_PC_84

//Various calculator communication timeouts
#define ERR_READ_TIMEOUT      1000
#define ERR_WRITE_TIMEOUT     2000
#define TIMEOUT               4000
#define GET_ENTER_TIMEOUT     30000

/******************************************************************************
 * Internal function prototypes
 ******************************************************************************/

static void printHex(const uint8_t *data, uint16_t length);
static void resetLines(void);
static uint16_t par_put(const uint8_t *data, uint16_t length);
static uint16_t par_get(uint8_t *data, uint16_t length);
static uint16_t checksum(const uint8_t *data, uint16_t length);

/******************************************************************************
 * Internal global variables
 ******************************************************************************/

//Buffers for input & output
uint8_t packetHead[HEADER_LENGTH] = {0};
uint8_t packetData[PACKET_DATA_LENGTH] = {0};
uint8_t packetChecksum[CHECKSUM_LENGTH] = {0};

/******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * initComm - Initializes communication with the calculator and serial port.
 *
 * This function should be called once at power up.
 */
void initComm(void) {
#if SERIAL_DEBUG_ENABLED
  //Start serial communication with the computer
  Serial.begin(SERIAL_SPEED);
#endif

  resetLines(); //Set up the I/O lines
}

/**
 * send - Sends data to the calculator.
 *
 * Parameters:
 *    const uint8_t *data: a pointer to the data to send (will be
 *                         wrapped in a packet by this function)
 *    uint16_t length: the length of data to send
 *
 * Note: Reuses packetHead and packetChecksum. Does not copy data to a separate
 * buffer.
 */
void send(const uint8_t *data, uint16_t length) {
  packetHead[0] = MACHINE_ID;
  packetHead[1] = CMD_DATA;
  packetHead[2] = length & 0x00FF;
  packetHead[3] = length >> 8;

  uint16_t chksm = checksum(data, length);
  packetChecksum[0] = chksm & 0x00FF;
  packetChecksum[1] = chksm >> 8;

#if SERIAL_DEBUG_ENABLED
  Serial.print(F("Sent: "));
  printHex(packetHead, HEADER_LENGTH);
  printHex(data, length);
  printHex(packetChecksum, CHECKSUM_LENGTH);
  Serial.println();

  if (!testStatus(SERIAL_DIAGNOSTICS_STATUS)) {
    //Prevent recursively calling receive()
#endif

  /* These par_puts are in conditionals to prevent getting stuck receiving ACK
   * if there was a transmit error.
   */
  uint16_t err = 0;
  if (err = par_put(packetHead, HEADER_LENGTH)) {
    Serial.print(F("Error sending head: "));
    Serial.println(err);
  } else if (err = par_put(data, length)) {
    Serial.print(F("Error sending data: "));
    Serial.println(err);
  } else if (err = par_put(packetChecksum, CHECKSUM_LENGTH)) {
    Serial.print(F("Error sending checksum: "));
    Serial.println(err);
  } else {
    //Serial.println(F("Sent reply"));

    //Receive the ACK
    receive(packetHead, HEADER_LENGTH);
    //Serial.println(F("Received ACK"));
  }
#if SERIAL_DEBUG_ENABLED
  }
#endif
}

/**
 * sendTICommand - Sends a TI command packet to the calculator.
 *
 * Parameter:
 *    const uint8_t commandID: the command ID byte to send
 *
 * Note: Reuses packetHead.
 */

void sendTICommand(uint8_t commandID) {
  packetHead[0] = MACHINE_ID;
  packetHead[1] = commandID;
  packetHead[2] = 0;
  packetHead[3] = 0;
  par_put(packetHead, HEADER_LENGTH);

  Serial.print(F("Sent: "));
  printHex(packetHead, HEADER_LENGTH);
  Serial.println();

  //Receive the ACK
  //par_get(packetHead, HEADER_LENGTH);
}

/**
 * receive - Waits to receive the specified length of data successfully.
 *
 * Parameters:
 *    uint8_t *data: a pointer to store the received data
 *    uint16_t length: the length of data to receive
 *
 * Retries receiving until the transmission doesn't time out.
 * Also manages the timeouts and serial debug processing since this function
 * gets called about once per second.
 */

void receive(uint8_t *data, uint16_t length) {
  //Serial variables
  uint8_t byte = 0;
  uint16_t validCharsRead = 0;
  uint16_t validBytesRead = 0;

  while (par_get(data, length)) {
    manageTimeouts();

#if SERIAL_DEBUG_ENABLED
    //Get any serial data, converting chars to hex bytes
    while (Serial.available()) {
      char nextChar = Serial.read();
      if (nextChar >= '0' && nextChar <= '9') {
        byte <<= 4;
        byte |= (nextChar - '0');
        validCharsRead++;
      } else if (nextChar >= 'A' && nextChar <= 'F') {
        byte <<= 4;
        byte |= (nextChar - 'A' + 10);
        validCharsRead++;
      } else if (nextChar >= 'a' && nextChar <= 'f') {
        byte <<= 4;
        byte |= (nextChar - 'a' + 10);
        validCharsRead++;
      } else if (nextChar == '\n') {
        //This is the end of the transmission. Process the data.
        //Serial.print(F("Received command: "));
        //Serial.println(packetData[0], HEX);
        Serial.print(F("Debug: "));
        printHex(packetData, validBytesRead);
        Serial.println();
        setStatus(SERIAL_DIAGNOSTICS_STATUS);
        processCommand(packetData[0]);
        clearStatus(SERIAL_DIAGNOSTICS_STATUS);
        validBytesRead = 0;
        validCharsRead = 0;
        continue;
      } else {
        //Invalid character. Skip adding the byte.
        continue;
      }
      if (validCharsRead > 0 && validCharsRead % 2 == 0) {
        packetData[validBytesRead] = byte;
        validBytesRead++;
        byte = 0;
      }
    }
#endif
  }
}

/**
 * getPacket - Waits for a valid packet to be received and stores received
 * data in packetData.
 *
 * Returns:
 *    uint8_t cmd: the command byte received
 *
 * Also handles receiving the handshake.
 */

uint8_t getPacket(void) {
  bool keepWaiting = true;
  while (keepWaiting) {
    resetLines();
    
    //At first, only get the header so we know the length of the data (if any)
    receive(packetHead, HEADER_LENGTH);
    
    Serial.print(F("Received: "));
    printHex(packetHead, HEADER_LENGTH);

    uint16_t length = packetHead[2] | packetHead[3] << 8;
    
    if(packetHead[1] == CMD_RDY) { //Ready check - required once at startup
      Serial.println();
      setStatus(RECEIVED_HANDSHAKE_STATUS);
      sendTICommand(CMD_ACK);
    } else if (testStatus(RECEIVED_HANDSHAKE_STATUS) &&
        packetHead[1] == CMD_DATA) { //Data packet - everything after RDY
      //Data packet should always contain data, but check to be safe
      if (length) {
        receive(packetData, length);
        printHex(packetData, length);

        receive(packetChecksum, CHECKSUM_LENGTH);
        printHex(packetChecksum, CHECKSUM_LENGTH);

        uint16_t receivedChksm = packetChecksum[0] | packetChecksum[1] << 8;
        uint16_t calculatedChksm = checksum(packetData, length);

        Serial.println();

        if (calculatedChksm == receivedChksm) {
          //Checksum is valid. Acknowledge the packet.
          sendTICommand(CMD_ACK);
          //Stop waiting and return
          keepWaiting = false;
        } else {
          Serial.print(F("Error: expected checksum: "));
          Serial.println(calculatedChksm, HEX);
          sendTICommand(CMD_ERR);
        }
      }
    } else if (packetHead[1] == CMD_ACK) {
      //Somehow we are receiving an ACK when we aren't supposed to.
      //Accept it anyway (nothing to do).
    } else {
      /* Either we haven't received the handshake yet or the packet type wasn't
       * recognized.
       */
      //Eat the rest of the packet
      if (length) {
        receive(packetData, length);
        receive(packetChecksum, CHECKSUM_LENGTH);
      }
      //Send a NAK to indicate the packet was ignored.
      sendTICommand(CMD_SKIP_EXIT);
      Serial.println(F("Sent NAK"));
    }
  }

  Serial.println();

  return packetData[0];
}

/**
 * printHex - Prints some hex bytes to the serial port.
 *
 * Parameters:
 *    const uint8_t *data: A pointer to the data to print
 *    uint16_t length: The number of bytes to print
 */
static void printHex(const uint8_t *data, uint16_t length) {
#if SERIAL_DEBUG_ENABLED
  for (uint16_t i = 0; i < length; i++) {
    if (data[i] < 0x10) {
      Serial.print(F("0"));
    }
    Serial.print(data[i], HEX);
    Serial.print(F(" "));
  }
#endif
}

/**
 * Arduino to TI linking routines by Christopher "Kerm Martian" Mitchell
 * http://www.cemetech.net/forum/viewtopic.php?t=4771
 * Modified by ajcord to remove commented-out code, reduce oversized variables,
 * increase style consistency with the rest of the firmware, and add blinkLED().
 *
 * Do not modify any functionality below this line.
 */

/**
 * resetLines - Resets the ports used for TI linking
 */
static void resetLines() {
  pinMode(TI_RING_PIN, INPUT);           // set pin to input
  digitalWrite(TI_RING_PIN, HIGH);       // turn on pullup resistors
  pinMode(TI_TIP_PIN, INPUT);            // set pin to input
  digitalWrite(TI_TIP_PIN, HIGH);        // turn on pullup resistors
}

/**
 * par_put - Transmits a length of data over the link port.
 *
 * Parameters:
 *    const uint8_t *data: A pointer to the data to transmit
 *    uint16_t length: The number of bytes to transmit
 * Returns:
 *    uint16_t error: Zero if there was no error
 *                    Nonzero error data if there was an error
 */
static uint16_t par_put(const uint8_t *data, uint16_t length) {
  uint8_t bit;
  uint16_t j;
  uint32_t previousMillis = 0;
  uint8_t byte;

  for(j = 0; j < length; j++) {
    byte = data[j];

    for (bit = 0; bit < 8; bit++) {
      previousMillis = 0;
      while ((digitalRead(TI_RING_PIN) << 1 | digitalRead(TI_TIP_PIN)) != 0x03) {
        if (previousMillis++ > TIMEOUT)
          return ERR_WRITE_TIMEOUT + j + 100 * bit;
      };
      if (byte & 1) {
        pinMode(TI_RING_PIN, OUTPUT);
        digitalWrite(TI_RING_PIN, LOW);
        previousMillis = 0;
        while (digitalRead(TI_TIP_PIN) == HIGH) {
          if (previousMillis++ > TIMEOUT)
            return ERR_WRITE_TIMEOUT + 10 + j + 100 * bit;
        };

        resetLines();
        previousMillis = 0;
        while (digitalRead(TI_TIP_PIN) == LOW) {
          if (previousMillis++ > TIMEOUT)
            return ERR_WRITE_TIMEOUT + 20 + j + 100 * bit;
        };
      } else {
        pinMode(TI_TIP_PIN, OUTPUT);
        digitalWrite(TI_TIP_PIN, LOW); //should already be set because of the pullup resistor register
        previousMillis = 0;
        while (digitalRead(TI_RING_PIN) == HIGH) {
          if (previousMillis++ > TIMEOUT)
            return ERR_WRITE_TIMEOUT + 30 + j + 100 * bit;
        };

        resetLines();
        previousMillis = 0;
        while (digitalRead(TI_RING_PIN) == LOW) {
          if (previousMillis++ > TIMEOUT)
            return ERR_WRITE_TIMEOUT + 40 + j + 100 * bit;
        };
      }
      byte >>= 1;
    }
  }
  return 0;
}

/**
 * par_get - Receives a length of data over the link port.
 *
 * Parameters:
 *    uint8_t *data: A pointer to store the received data
 *    uint16_t length: The number of bytes to receive
 * Returns:
 *    uint16_t error: Zero if there was no error
 *                    Nonzero error data if there was an error
 */
static uint16_t par_get(uint8_t *data, uint16_t length) {
  uint8_t bit;
  uint16_t j;
  uint32_t previousMillis = 0;

  for(j = 0; j < length; j++) {
    uint8_t v, byteout = 0;
    for (bit = 0; bit < 8; bit++) {
      previousMillis = 0;
      while ((v = (digitalRead(TI_RING_PIN) << 1 | digitalRead(TI_TIP_PIN))) == 0x03) {
        blinkLED(); // (ajcord) Added blinkLED() here since it needs to be called frequently
        if (previousMillis++ > GET_ENTER_TIMEOUT)
          return ERR_READ_TIMEOUT + j + 100 * bit;
      }
      if (v == 0x01) {
        byteout = (byteout >> 1) | 0x80;
        pinMode(TI_TIP_PIN, OUTPUT);
        digitalWrite(TI_TIP_PIN, LOW); //should already be set because of the pullup resistor register
        previousMillis = 0;
        while (digitalRead(TI_RING_PIN) == LOW) { //wait for the other one to go low
          if (previousMillis++ > TIMEOUT)
            return ERR_READ_TIMEOUT + 10 + j + 100 * bit;
        }
        digitalWrite(TI_RING_PIN,HIGH);
      } else {
        byteout = (byteout >> 1) & 0x7F;
        pinMode(TI_RING_PIN, OUTPUT);
        digitalWrite(TI_RING_PIN, LOW); //should already be set because of the pullup resistor register
        previousMillis = 0;
        while (digitalRead(TI_TIP_PIN) == LOW) {
          if (previousMillis++ > TIMEOUT)
            return ERR_READ_TIMEOUT + 20 + j + 100 * bit;
        }
        digitalWrite(TI_TIP_PIN, HIGH);
      }
      pinMode(TI_RING_PIN, INPUT); //set pin to input
      digitalWrite(TI_RING_PIN, HIGH); //turn on pullup resistors
      pinMode(TI_TIP_PIN, INPUT); //set pin to input
      digitalWrite(TI_TIP_PIN, HIGH); //turn on pullup resistors
    }
    data[j] = byteout;
  }
  return 0;
}

/**
 * checksum - Calculates the 16-bit checksum of some data.
 *
 * Parameters:
 *    const uint8_t *data: A pointer to the data to sum
 *    uint16_t length: The length of data to sum
 * Returns:
 *    uint16_t checksum: The lower 16 bits of the sum of the data
 * Note: Checksum algorithm adapted from the TI Link Guide.
 * http://merthsoft.com/linkguide/ti83+/packet.html
 * Ported to Arduino by ajcord.
 */
static uint16_t checksum(const uint8_t *data, uint16_t length) {
  uint16_t checksum = 0;
  for(uint16_t x = 0; x < length; x++) {
    checksum += data[x]; //overflow automatically limits to 16 bits
  }
  return checksum;
}
