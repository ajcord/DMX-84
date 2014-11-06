/**
 * DMX-84
 * Link header
 *
 * This file contains the external defines and prototypes for communicating
 * with the calculator and the serial port.
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

#ifndef LINK_H
#define LINK_H

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <inttypes.h>

/******************************************************************************
 * External constants
 ******************************************************************************/

//Some of the valid TI packet commands
#define CMD_CTS               0x09
#define CMD_DATA              0x15
#define CMD_SKIP_EXIT         0x36
#define CMD_ACK               0x56
#define CMD_ERR               0x5A
#define CMD_RDY               0x68
#define CMD_EOT               0x92

//Buffer lengths
#define HEADER_LENGTH         4
#define PACKET_DATA_LENGTH    513
#define CHECKSUM_LENGTH       2

//Serial parameters
#define SERIAL_SPEED          9600

/******************************************************************************
 * Class definition
 ******************************************************************************/

class LinkClass {
    public:
        void begin(void);
        void send(const uint8_t *data, uint16_t length);
        void send(uint8_t commandID);
        uint8_t receive(void);

        /* These communication buffers are public so they can be reused as
         * temporary buffers by other files. Collectively, they use over a
         * quarter of the SRAM.
         */
        uint8_t packetHead[HEADER_LENGTH];
        uint8_t packetData[PACKET_DATA_LENGTH];
        uint8_t packetChecksum[CHECKSUM_LENGTH];

    private:
        void receive(uint8_t *data, uint16_t length);
        void printHex(const uint8_t *data, uint16_t length);
        void resetLines(void);
        uint16_t par_put(const uint8_t *data, uint16_t length);
        uint16_t par_get(uint8_t *data, uint16_t length);
        uint16_t checksum(const uint8_t *data, uint16_t length);
};

extern LinkClass Link;

#endif
