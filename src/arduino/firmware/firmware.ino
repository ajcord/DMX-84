/**
 * DMX-84
 * Arduino firmware v0.5.1
 *
 * This file contains the code that processes received commands and generally
 * manages the Arduino.
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
#include <avr/sleep.h>
#include <avr/power.h>
#include <DmxSimple.h>

#include "firmware.h"
#include "status.h"
#include "comm.h"
#include "LED.h"

/******************************************************************************
 * Internal constants
 ******************************************************************************/

//DMX
#define MAX_DMX               512
#define DEFAULT_MAX_CHANNELS  128

//System timeouts (milliseconds)
#define AUTO_SHUT_DOWN_TIME             21600000 //6 hours
#define AUTO_SHUT_DOWN_WARN_TIME        (AUTO_SHUT_DOWN_TIME - 60000) //5:59 hrs
#define RESTRICTED_MODE_TIMEOUT         1000

//Protocol version
#define PROTOCOL_VERSION_MAJOR          0
#define PROTOCOL_VERSION_MINOR          3
#define PROTOCOL_VERSION_PATCH          2

//Firmware version
#define FIRMWARE_VERSION_MAJOR          0
#define FIRMWARE_VERSION_MINOR          5
#define FIRMWARE_VERSION_PATCH          1

/******************************************************************************
 * Internal function prototypes
 ******************************************************************************/

static bool setMaxChannel(uint16_t newMaxChannel = DEFAULT_MAX_CHANNELS);
static void startTransmitDMX(void);
static void stopTransmitDMX(void);

/******************************************************************************
 * Internal global variables
 ******************************************************************************/

uint16_t maxChannel;

uint32_t enteredRestrictedMode = 0; //The time that restricted mode was entered
uint32_t lastCmdReceived = 0; //The time the last command was received

/******************************************************************************
 * Function definitions
 ******************************************************************************/

/**
 * setup - Sets up the I/O, status, DmxSimple, etc.
 *
 * Note: This function is called once at power up.
 */
void setup() {
  initLED(); //Initialize the LED
  initComm(); //Initialize communication

  resetStatus(); //No flags initially set

  //Set up DMX
  DmxSimple.usePin(DMX_OUT_PIN); //Set the pin to transmit DMX on
  startTransmitDMX(); //Enable DMX
  setMaxChannel(DEFAULT_MAX_CHANNELS); //Set the max channels to transmit

  //Tell the calculator we're up and running
  sendTICommand(CMD_CTS);

  Serial.println(F("Ready"));
}

/**
 * loop - Receives packets and processes the received commands.
 *
 * Note: This function is called in a forever loop in main().
 */
void loop() {
  uint8_t cmd = getPacket();

  /* We received a command, so remember the timestamp and clear the shutdown 
   * warning status.
   */
  lastCmdReceived = millis();
  if (testStatus(SENT_SHUT_DOWN_WARNING_STATUS)) {
    //We need to clear the status and reset the LED flashing since
    //auto shut down has been cancelled.
    clearStatus(SENT_SHUT_DOWN_WARNING_STATUS);
  }

  processCommand(cmd);
}

/**
 * processCommand - Processes a received command.
 *
 * Parameter:
 *    uint8_t cmd: the command byte received
 */
void processCommand(uint8_t cmd) {
  switch (cmd) {
    case 0x00: {
      //Heartbeat
      send(&cmd, 1); //Echo the command to acknowledge it
      Serial.println(F("Heartbeat"));
      break;
    }
    
    case 0x01: {
      //Enable restricted mode
      //Allows protected commands to be executed for one second.
      setStatus(RESTRICTED_MODE_STATUS);
      enteredRestrictedMode = millis();

      send(&cmd, 1); //Echo the command to acknowledge it
      Serial.println(F("/!\\ Now in restricted mode"));
      break;
    }
    
    case 0x10:
    case 0x11: {
      //Sets a single channel
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      uint16_t newValue = packetData[2];
      dmxBuffer[channel] = newValue;
      
      Serial.print(F("Updated channel "));
      Serial.print(channel);
      Serial.print(F(" to "));
      Serial.println(newValue);
      break;
    }
    
    case 0x12:
    case 0x13: {
      //Increments a single channel by 1
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      if (dmxBuffer[channel] < 0xFF) {
        dmxBuffer[channel]++;
      }
            
      Serial.print(F("Incremented channel "));
      Serial.println(channel);
      break;
    }
    
    case 0x14:
    case 0x15: {
      //Decrements a single channel by 1
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      if (dmxBuffer[channel] > 0x00) {
        dmxBuffer[channel]--;
      }
      
      Serial.print(F("Decremented channel "));
      Serial.println(channel);
      break;
    }
    
    case 0x16:
    case 0x17: {
      //Increments a single channel by a number
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      uint8_t incrementAmount = packetData[2];
      if (dmxBuffer[channel] + incrementAmount < 0xFF) { //Prevent overflow
        dmxBuffer[channel] += incrementAmount;
      }
            
      Serial.print(F("Incremented channel "));
      Serial.print(channel);
      Serial.print(F(" by "));
      Serial.println(incrementAmount);
      break;
    }
    
    case 0x18:
    case 0x19: {
      //Decrements a single channel by a number
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      uint8_t decrementAmount = packetData[2];
      if (decrementAmount <= dmxBuffer[channel]) { //Prevent underflow
        dmxBuffer[channel] -= decrementAmount;
      } else {
        dmxBuffer[channel] = 0;
      }
            
      Serial.print(F("Decremented channel "));
      Serial.print(channel);
      Serial.print(F(" by "));
      Serial.println(decrementAmount);
      break;
    }
    
    case 0x20:
    case 0x21: {
      //Sets 256 channels at once
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[i | (cmd & 1) << 8] = packetData[i + 1];
      }
      Serial.println(F("Updated 256 channels"));
      break;
    }
    
    case 0x22:
    case 0x23: {
      //Sets a block of channels at once
      uint16_t startChannel = packetData[1] | (packetData[0] & 1) << 8; //The first channel of the block
      uint8_t length = packetData[2]; //The number of channels to set
      //Check that the parameters won't go past the end of the universe
      if (startChannel + length > MAX_DMX) {
        setError(INVALID_VALUE_ERROR);
        length = MAX_DMX - startChannel; //Don't go above channel 512
      }
      for (uint16_t i = startChannel; i < length; i++) {
        dmxBuffer[i] = packetData[i + 2];
      }
      Serial.print(F("Updated channels "));
      Serial.print(startChannel);
      Serial.print(F("-"));
      Serial.println(startChannel + length);
      break;
    }
    
    case 0x24: {
      //Increments all channels by a value
      uint8_t incrementAmount = packetData[1];
      for (uint16_t i = 0; i < 512; i++) {
        if (dmxBuffer[i] + incrementAmount < 0xFF) { //Limit to ceiling
          dmxBuffer[i] += incrementAmount;
        }
      }
      Serial.print(F("Incremented all channels by "));
      Serial.println(incrementAmount);
      break;
    }
    
    case 0x25: {
      //Decrements all channels by a value
      uint8_t decrementAmount = packetData[1];
      for (uint16_t i = 0; i < 512; i++) {
        if (dmxBuffer[i] >= decrementAmount) { //Limit to floor
          dmxBuffer[i] -= decrementAmount;
        } else {
          dmxBuffer[i] = 0;
        }
      }
      Serial.print(F("Decremented all channels by "));
      Serial.println(decrementAmount);
      break;
    }
    
    case 0x26: {
      //Sets all channels to the same value
      uint8_t newValue = packetData[1];
      for (uint16_t i = 0; i < 512; i++) {
        dmxBuffer[i] = newValue; //Set each channel to the new value
      }
      Serial.print(F("Set all channels to "));
      Serial.println(newValue);
      break;
    }
    
    case 0x27: {
      //Sets 512 channels at once
      for (uint16_t i = 0; i < 512; i++) {
        dmxBuffer[i] = packetData[i + 1];
      }
      Serial.println(F("Updated 512 channels"));
      break;
    }
    
    case 0x30: {
      //Copy channel data from 256-511 to 0-255
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[i] = dmxBuffer[i + 256];
      }
      
      Serial.println(F("Copied 256-511 to 0-255"));
      break;
    }
    
    case 0x31: {
      //Copy channel data from 0-255 to 256-511
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[i + 256] = dmxBuffer[i];
      }
      
      Serial.println(F("Copied 0-255 to 256-511"));
      break;
    }
    
    case 0x32: {
      //Exchange channel data between 0-255 and 256-511
      uint8_t temp = 0;
      for (uint16_t i = 0; i < 256; i++) {
        //Swap the value of each low channel with its high counterpart
        temp = dmxBuffer[i];
        dmxBuffer[i] = dmxBuffer[i + 256];
        dmxBuffer[i + 256] = temp;
      }
      
      Serial.println(F("Exchanged 0-255 with 256-511"));
      break;
    }
    
    case 0x40:
    case 0x41: {
      //Reply with channel value for specific channel
      uint16_t channel = packetData[1] | (cmd & 1) << 8;
      uint8_t packet[] = {cmd, dmxBuffer[channel]};
      send(packet, 2);
      
      Serial.print(F("Channel "));
      Serial.print(channel);
      Serial.print(F(" is at "));
      Serial.println(dmxBuffer[channel]);
      break;
    }
    
    case 0x42: {
      //Reply with all channel values
      packetData[0] = cmd;
      for (uint16_t i = 0; i < MAX_DMX; i++) {
        packetData[i + 1] = dmxBuffer[i];
      }
      send(packetData, MAX_DMX);
      
      Serial.println(F("Channel values:"));
      for (uint16_t i = 0; i < MAX_DMX; i++) {
        Serial.print(dmxBuffer[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      
      break;
    }

    case 0xDB: {
      //Toggle the LED debug pattern
      toggleStatus(DEBUG_STATUS);
      break;
    }
    
    case 0xE0: {
      //Stop transmitting DMX
      stopTransmitDMX();
      
      Serial.println(F("Stopped DMX"));
      break;
    }
    
    case 0xE1: {
      //Start transmitting DMX
      startTransmitDMX();
      
      Serial.println(F("Started DMX"));
      break;
    }
    
    case 0xE2:
    case 0xE3: {
      //Set max channels to transmit
      uint16_t newMax = packetData[1] | (cmd & 1) << 8;
      if (newMax == 0) {
        //Since 512 can't be represented in 9 bits, 0 becomes 512.
        newMax = 512;
      }
      setMaxChannel(newMax); //Set the max number of channels
      
      Serial.print(F("Max channels now "));
      Serial.println(newMax);
      break;
    }
    
    case 0xE4: {
      //Start a digital blackout
      DmxSimple.startDigitalBlackout();
      setStatus(DIGITAL_BLACKOUT_ENABLED_STATUS);
      
      Serial.println(F("Started digital blackout"));
      break;
    }
    
    case 0xE5: {
      //Stop a digital blackout
      DmxSimple.stopDigitalBlackout();
      clearStatus(DIGITAL_BLACKOUT_ENABLED_STATUS);
      
      Serial.println(F("Stopped digital blackout"));
      break;
    }
    
    case 0xF0: {
      //Initiate safe shutdown sequence - only works in restricted mode
      send(&cmd, 1);
      initShutDown();
      break;
    }
    
    case 0xF1: {
      //Initiate soft reset - only works in restricted mode
      send(&cmd, 1);
      initShutDown(true);
      break;
    }
    
    case 0xF8: {
      //Reply with status flags
      uint8_t status = getStatus();
      uint8_t packet[] = {cmd, status};
      send(packet, 2);
      
      Serial.print(F("Status flags: "));
      Serial.println(status, HEX);
      break;
    }
    
    case 0xF9: {
      //Reply with error flags
      uint8_t errors = getErrors();
      uint8_t packet[] = {cmd, errors};
      send(packet, 2);
      
      Serial.print(F("Error flags: "));
      Serial.println(errors, HEX);
      
      resetErrors(); //Clear out the errors now that they have been reported
      break;
    }
    
    case 0xFA: {
      //Reply with version numbers
      uint8_t packet[] = {
        cmd,
        PROTOCOL_VERSION_PATCH,
        PROTOCOL_VERSION_MINOR,
        PROTOCOL_VERSION_MAJOR,
        FIRMWARE_VERSION_PATCH,
        FIRMWARE_VERSION_MINOR,
        FIRMWARE_VERSION_MAJOR
      };
      send(packet, 7);
      
      Serial.print(F("Protocol version: "));
      Serial.print(PROTOCOL_VERSION_MAJOR);
      Serial.print(F("."));
      Serial.print(PROTOCOL_VERSION_MINOR);
      Serial.print(F("."));
      Serial.println(PROTOCOL_VERSION_PATCH);
      
      Serial.print(F("Firmware version: "));
      Serial.print(FIRMWARE_VERSION_MAJOR);
      Serial.print(F("."));
      Serial.print(FIRMWARE_VERSION_MINOR);
      Serial.print(F("."));
      Serial.println(FIRMWARE_VERSION_PATCH);
      break;
    }
    
    case 0xFB: {
      //Reply with protocol version number
      uint8_t packet[] = {
        cmd,
        PROTOCOL_VERSION_PATCH,
        PROTOCOL_VERSION_MINOR,
        PROTOCOL_VERSION_MAJOR
      };
      send(packet, 4);
      
      Serial.print(F("Protocol version: "));
      Serial.print(PROTOCOL_VERSION_MAJOR);
      Serial.print(F("."));
      Serial.print(PROTOCOL_VERSION_MINOR);
      Serial.print(F("."));
      Serial.println(PROTOCOL_VERSION_PATCH);
      break;
    }
    
    case 0xFC: {
      //Reply with firmware version number
      uint8_t packet[] = {
        cmd,
        FIRMWARE_VERSION_PATCH,
        FIRMWARE_VERSION_MINOR,
        FIRMWARE_VERSION_MAJOR
      };
      send(packet, 4);
      
      Serial.print(F("Firmware version: "));
      Serial.print(FIRMWARE_VERSION_MAJOR);
      Serial.print(F("."));
      Serial.print(FIRMWARE_VERSION_MINOR);
      Serial.print(F("."));
      Serial.println(FIRMWARE_VERSION_PATCH);
      break;
    }
    
    case 0xFD: {
      //Reply with current temperature
      uint32_t temp = readTemp();
      uint8_t packet[] = {
        cmd,
        (temp & 0xFF),
        (temp & 0xFF00) >> 8,
        (temp & 0xFF0000) >> 16,
        (temp & 0xFF000000) >> 24
      };
      send(packet, 5);
      
      Serial.print(F("Current temperature: "));
      Serial.print(temp/1000.0);
      Serial.println(F(" C"));
      break;
    }
    
    case 0xFE: {
      //Reply with uptime in milliseconds
      uint32_t uptime = millis();
      uint8_t packet[] = {
        cmd,
        (uptime & 0xFF),
        (uptime & 0xFF00) >> 8,
        (uptime & 0xFF0000) >> 16,
        (uptime & 0xFF000000) >> 24
      };
      send(packet, 5);
      
      Serial.print(F("Uptime: "));
      Serial.print(uptime/1000.0);
      Serial.println(F(" seconds"));
      break;
    }
    
    default: {
      //Unknown command; set the error code
      setError(UNKNOWN_COMMAND_ERROR);
      
      Serial.println(F("Error: unknown command"));
      break;
    }
  }
}

/**
 * setMaxChannel - Sets the maximum number of channels to transmit
 * Parameter:
 *    uint16_t newMaxChannel: The maximum number of channels to transmit (1-512)
 */
static bool setMaxChannel(uint16_t newMaxChannel) {
  //Setting max channels to zero disables DMX, so prevent that
  if (newMaxChannel == 0 || newMaxChannel > MAX_DMX) {
    setError(INVALID_VALUE_ERROR); //Invalid value
    return false;
  } else if (!testStatus(DMX_ENABLED_STATUS)) {
    setError(DMX_DISABLED_ERROR); //DMX is disabled
    return false;
  } else {
    maxChannel = newMaxChannel;
    DmxSimple.maxChannel(newMaxChannel);
    return true;
  }
}

/**
 * startTransmitDMX - Enables DMX output
 */
static void startTransmitDMX(void) {
  DmxSimple.maxChannel(maxChannel); //Start transmitting DMX
  setStatus(DMX_ENABLED_STATUS); //Set the transmit enable flag
}

/**
 * stopTransmitDMX - Disables DMX output
 */
static void stopTransmitDMX(void) {
  DmxSimple.maxChannel(0); //Stop transmitting DMX
  clearStatus(DMX_ENABLED_STATUS); //Clear the transmit enable flag
}

/**
 * manageTimeouts - Handles checking if the timeout periods have passed
 *
 * This function should be called periodically at least once per second.
 */
void manageTimeouts() {
  if (testStatus(RESTRICTED_MODE_STATUS) &&
      (millis() - lastCmdReceived > RESTRICTED_MODE_TIMEOUT)) {
    clearStatus(RESTRICTED_MODE_STATUS); //Clear the no-op flag after 1 second
    Serial.println(F("Leaving restricted mode"));
  }

#if AUTO_SHUT_DOWN_ENABLED
  if (!testStatus(SENT_SHUT_DOWN_WARNING_STATUS) &&
      ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_WARN_TIME)) {
    //We haven't received a message in a while and haven't sent a warning yet.
    //Send the warning.
    uint8_t packet[] = {'S', 'O', 'S'};
    send(packet, 3);
    setStatus(SENT_SHUT_DOWN_WARNING_STATUS);
    Serial.println(F("Sent inactivity warning"));
  } else if (testStatus(SENT_SHUT_DOWN_WARNING_STATUS) &&
      ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_TIME)) {
    //It's been 1 minute since we sent a warning. Time to shut down.
    //Must enable restricted mode to shut down
    Serial.println(F("Shutting down..."));
    setStatus(RESTRICTED_MODE_STATUS);
    initShutDown();
  } else {
    //Do nothing
  }
#endif
}

/**
 * initShutDown - Shuts down or resets the Arduino
 * Parameter:
 *    bool reset: true to reset the firmware, false to power down
 *
 * Note: Cannot return successfully, for obvious reasons.
 */

void initShutDown(bool reset) {
  if (!testStatus(RESTRICTED_MODE_STATUS)) {
    setError(RESTRICTED_MODE_ERROR); //Last command wasn't no-op
    Serial.println(F("Error: must be in restricted mode"));
    return;
  }

  //Send EOT to calculator to let it know we are going down
  sendTICommand(CMD_EOT);
  
  //Prepare to shut down or reset
  if (reset) {
    Serial.println(F("The system is going down for reboot NOW!"));
  } else {
    Serial.println(F("The system is going down for system halt NOW!"));
  }

  //Some final cleanup
  stopTransmitDMX();
  digitalWrite(LED_PIN, LOW);
  Serial.flush(); //Wait on any last messages to go through
  Serial.end(); //Stop serial communication

  if (reset) {
    asm volatile ("jmp 0"); //Reset the software (i.e. reboot)
  }
  //(else shutdown)
  //Set up power down sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  power_all_disable();
  //sleep_bod_disable();
  sleep_cpu();
  //sleep_mode();
  while (1) {
    //Sit-n-spin. Shouldn't get here since there is no way to get out of sleep.
  }
}

/**
 * readTemp - Reads the approximate core temperatue using the built-in
 * temperature sensor.
 * Returns:
 *    int32_t temp: the core temperature in millidegrees Celsius
 *
 * Code from https://code.google.com/p/tinkerit/wiki/SecretThermometer
 * Note: The temperature returned is not very accurate and must be callibrated
 * to each Arduino.
 */
int32_t readTemp(void) {
  int32_t result;
  //Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(2); //Wait for Vref to settle
  ADCSRA |= _BV(ADSC); //Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = (result - 125) * 1075;
  result = result / 9; //Rough calibration factor
  return result;
}
