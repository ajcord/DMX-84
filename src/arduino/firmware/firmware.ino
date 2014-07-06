/**
 * The DATA Project
 * Arduino firmware v0.3.3
 *
 * By Alex Cordonnier (ajcord)
 *
 * This file contains the code that communicates with the calculator,
 * processes received commands, and generally manages the Arduino.
 *
 * Last modified July 6, 2014
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

#include <avr/sleep.h>
#include <avr/power.h>
#include <DmxSimple.h>

/******************************************************************************
 * Preprocessor definitions
 ******************************************************************************/

//DMX
#define MAX_DMX               512
#define DEFAULT_MAX_CHANNELS  128

//Pins and hardware
#define LED_PIN               9
#define DMX_OUT_PIN           10
#define TI_RING_PIN           4
#define TI_TIP_PIN            6

//Calculator communication
#define MACHINE_ID            0x23
#define CMD_CTS               0x09
#define CMD_DATA              0x15
#define CMD_SKIP_EXIT         0x36
#define CMD_ACK               0x56
#define CMD_ERR               0x5A
#define CMD_RDY               0x68
#define CMD_EOT               0x92
#define ERR_READ_TIMEOUT      1000
#define ERR_WRITE_TIMEOUT     2000
#define TIMEOUT               4000
#define GET_ENTER_TIMEOUT     30000
#define MACHINE_ID_BYTE       0
#define COMMAND_ID_BYTE       1
#define LENGTH_LOW_BYTE       2
#define LENGTH_HIGH_BYTE      (LENGTH_LOW_BYTE + 1)
#define HEADER_LENGTH         4
#define PACKET_DATA_LENGTH    519
#define CHECKSUM_LENGTH       2
#define CONTENTS_START        HEADER_LENGTH
#define CHECKSUM_LOW_BYTE     (HEADER_LENGTH + PACKET_DATA_LENGTH)
#define CHECKSUM_HIGH_BYTE    (CHECKSUM_LOW_BYTE + 1)

//Flag utilities
#define SET_STATUS(f)         (statusFlags |= (f))
#define CLEAR_STATUS(f)       (statusFlags &= ~(f))
#define TOGGLE_STATUS(f)      (statusFlags ^= (f))
#define TEST_STATUS(f)        (statusFlags & (f))
#define RESET_STATUS()        (statusFlags = 0)
#define SET_ERROR(f)          (errorFlags |= (f))
#define CLEAR_ERROR(f)        (errorFlags &= ~(f))
#define TOGGLE_ERROR(f)       (errorFlags ^= (f))
#define TEST_ERROR(f)         (errorFlags & (f))
#define RESET_ERRORS()        (errorFlags = 0)

//Status flags
#define LED_STATUS                      0x01
#define DMX_ENABLED_STATUS              0x02
#define DIGITAL_BLACKOUT_ENABLED_STATUS 0x04
#define RESTRICTED_MODE_STATUS          0x08
#define SENT_SHUT_DOWN_WARNING_STATUS   0x10
#define RECEIVED_HANDSHAKE_STATUS       0x20
#define ERROR_STATUS                    0x80

//Error flags
#define DMX_DISABLED_ERROR              0x01
#define DIGITAL_BLACKOUT_ERROR          0x02
#define RESTRICTED_MODE_ERROR           0x04
#define UNKNOWN_COMMAND_ERROR           0x08
#define BAD_PACKET_ERROR                0x10
#define INVALID_VALUE_ERROR             0x20
#define TIMEOUT_ERROR                   0x40
#define UNKNOWN_ERROR                   0x80
#define DMX_ERROR_MASK                  0x03
#define COMMUNICATION_ERROR_MASK        0x78

//System timeouts (milliseconds)
#define AUTO_SHUT_DOWN_TIME             21600000 //6 hours
#define AUTO_SHUT_DOWN_WARN_TIME        (AUTO_SHUT_DOWN_TIME - 60000) //5:59 hrs
#define RESTRICTED_MODE_TIMEOUT         1000

//Version numbers
#define PROTOCOL_VERSION_MAJOR          0
#define PROTOCOL_VERSION_MINOR          3
#define PROTOCOL_VERSION_PATCH          1
#define FIRMWARE_VERSION_MAJOR          0
#define FIRMWARE_VERSION_MINOR          3
#define FIRMWARE_VERSION_PATCH          3

//Compile-time options
#define AUTO_SHUT_DOWN_ENABLED      1

/******************************************************************************
 * Function declarations
 ******************************************************************************/

//DMX and device utility
bool setMaxChannel(uint16_t newMaxChannel = DEFAULT_MAX_CHANNELS);
void startTransmitDMX();
void stopTransmitDMX();
void initShutDown(bool reset = false);
int32_t readTemp();

//Calculator communication
void waitForPacket(void);
void resetLines(void);
void receive(const uint8_t *data, uint16_t length);
void reply(volatile uint8_t *data, uint16_t length);
static uint16_t par_put(const uint8_t *data, uint16_t length);
static uint16_t par_get(uint8_t *data, uint16_t length);
uint16_t checksum(uint8_t *data, uint16_t length);

/******************************************************************************
 * Global variables
 ******************************************************************************/

//DMX
uint16_t maxChannel;

//Calculator communication
/*
struct { //Input buffer from calculator. Contains one packet plus metadata.
  uint8_t header[HEADER_LENGTH];
  uint8_t contents[PACKET_DATA_LENGTH];
  uint8_t checksumBytes[CHECKSUM_LENGTH];
  uint16_t checksum;
  uint16_t length; //The length of the packet as specified in the header
  uint8_t cmd; //The command byte from the packet data, not the header
} packet;
*/
uint8_t cmd = 0; //The last command received
uint32_t lastCmdReceived = 0; //The time the last command was received
uint32_t enteredRestrictedMode = 0; //The time that restricted mode was entered
const uint8_t ACK[] = {MACHINE_ID, CMD_ACK, 0, 0}; //Acknowledge
const uint8_t NAK[] = {MACHINE_ID, CMD_SKIP_EXIT, 1, 0, 1, 1, 0}; //Cancel Tx
const uint8_t CTS[] = {MACHINE_ID, CMD_CTS, 0, 0}; //Clear to send
const uint8_t ERR[] = {MACHINE_ID, CMD_ERR, 0, 0}; //Checksum error, retransmit

//Buffers for input & output
uint8_t packetHead[HEADER_LENGTH] = {0};
uint8_t packetData[PACKET_DATA_LENGTH] = {0};
uint8_t packetChecksum[CHECKSUM_LENGTH] = {0};

//Flag variables
uint8_t errorFlags = 0; //Any errors that have not yet been reported
uint8_t statusFlags = 0; //The current firmware status

/******************************************************************************
 * Functions definitions
 ******************************************************************************/

void setup() {
  RESET_STATUS(); //No flags initially set
  resetLines(); //Set up the I/O lines

  //Set up DMX
  DmxSimple.usePin(DMX_OUT_PIN); //Set the pin to transmit DMX on
  startTransmitDMX(); //Enable DMX
  setMaxChannel(DEFAULT_MAX_CHANNELS); //Set the max channels to transmit

  //Not sure what I'll end up using the LED for. For now, it will be a power indicator.
  SET_STATUS(LED_STATUS);
  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, HIGH);

  //Start serial communication with the computer
  Serial.begin(115200);
  Serial.println(F("Ready"));

  //Tell the calculator we're up and running
  par_put(CTS, 4);
}

void loop() {
  waitForPacket();
  lastCmdReceived = millis();
  
  cmd = packetData[0];
  /*
  Serial.print(F("Cmd: "));
  Serial.println(cmd, HEX);
  */
  switch (cmd) {
    case 0x00: {
      //Heartbeat
      uint8_t packet[] = {0}; //Will not be sent
      reply(packet, 0); //Echo the command to acknowledge it
      Serial.println(F("Heartbeat"));
      break;
    }
    
    case 0x01: {
      //Enable restricted mode
      //Allows protected commands to be executed for one second.
      SET_STATUS(RESTRICTED_MODE_STATUS);
      enteredRestrictedMode = millis();
      uint8_t packet[] = {0}; //Will not be sent
      reply(packet, 0); //Echo the command to acknowledge it
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
      uint16_t length = packetData[2]; //The number of channels to set
      length = max(length, MAX_DMX - startChannel); //Don't go above channel 512
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
        if (dmxBuffer[i] + incrementAmount < 0xFF) { //Prevent overflow
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
        if (dmxBuffer[i] >= decrementAmount) { //Prevent underflow
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
      //FIXME: Seems to sometimes set channel 256 to 0x32. Can't seem to reproduce anymore, though.
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
      uint8_t packet[] = {dmxBuffer[channel]};
      reply(packet, 1);
      
      Serial.print(F("Channel "));
      Serial.print(channel);
      Serial.print(F(" is at "));
      Serial.println(dmxBuffer[channel]);
      break;
    }
    
    case 0x42: {
      //Reply with all channel values
      reply(dmxBuffer, 512);
      
      Serial.println(F("Channel values:"));
      for (uint16_t i = 0; i < 512; i++) {
        Serial.print(dmxBuffer[i], HEX);
        Serial.print(F(" "));
      }
      Serial.println();
      
      break;
    }
    
    case 0x5A: {
      //Toggle the LED (for debugging)
      TOGGLE_STATUS(LED_STATUS);
      digitalWrite(LED_PIN, TEST_STATUS(LED_STATUS));
      break;
    }
    
    /* Intermediate commands reserved for future use */
    
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
      setMaxChannel(newMax); //Set the max number of channels
      
      Serial.print(F("Max channels now "));
      Serial.println(newMax);
      break;
    }
    
    case 0xE4: {
      //Start a digital blackout
      DmxSimple.startDigitalBlackout();
      SET_STATUS(DIGITAL_BLACKOUT_ENABLED_STATUS);
      
      Serial.println(F("Started digital blackout"));
      break;
    }
    
    case 0xE5: {
      //Stop a digital blackout
      DmxSimple.stopDigitalBlackout();
      CLEAR_STATUS(DIGITAL_BLACKOUT_ENABLED_STATUS);
      
      Serial.println(F("Stopped digital blackout"));
      break;
    }
    
    case 0xF0: {
      //Initiate safe shutdown sequence - only works in restricted mode
      uint8_t packet[] = {0};
      reply(packet, 0);
      initShutDown();
      break;
    }
    
    case 0xF1: {
      //Initiate soft reset - only works in restricted mode
      uint8_t packet[] = {0};
      reply(packet, 0);
      initShutDown(true);
      break;
    }
    
    case 0xF8: {
      //Reply with status flags
      uint8_t packet[] = {statusFlags};
      reply(packet, 1);
      
      Serial.print(F("Status flags: "));
      Serial.println(statusFlags, HEX);
      break;
    }
    
    case 0xF9: {
      //Reply with error flags
      uint8_t packet[] = {errorFlags};
      reply(packet, 1);
      
      Serial.print(F("Error flags: "));
      Serial.println(errorFlags, HEX);
      
      RESET_ERRORS(); //Clear out the errors now that they have been reported
      CLEAR_STATUS(ERROR_STATUS);
      break;
    }
    
    case 0xFA: {
      //Reply with version numbers
      uint8_t packet[] = {PROTOCOL_VERSION_PATCH,
                          PROTOCOL_VERSION_MINOR,
                          PROTOCOL_VERSION_MAJOR,
                          FIRMWARE_VERSION_PATCH,
                          FIRMWARE_VERSION_MINOR,
                          FIRMWARE_VERSION_MAJOR};
      reply(packet, 6);
      
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
      uint8_t packet[] = {PROTOCOL_VERSION_PATCH,
                          PROTOCOL_VERSION_MINOR,
                          PROTOCOL_VERSION_MAJOR};
      reply(packet, 3);
      
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
      uint8_t packet[] = {FIRMWARE_VERSION_PATCH,
                          FIRMWARE_VERSION_MINOR,
                          FIRMWARE_VERSION_MAJOR};
      reply(packet, 3);
      
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
        (temp & 0xFF),
        (temp & 0xFF00) >> 8,
        (temp & 0xFF0000) >> 16,
        (temp & 0xFF000000) >> 24
      };
      reply(packet, 4);
      
      Serial.print(F("Current temperature: "));
      Serial.print(temp/1000.0);
      Serial.println(F(" C"));
      break;
    }
    
    case 0xFE: {
      //Reply with uptime in milliseconds
      uint32_t uptime = millis();
      uint8_t packet[] = {
        (uptime & 0xFF),
        (uptime & 0xFF00) >> 8,
        (uptime & 0xFF0000) >> 16,
        (uptime & 0xFF000000) >> 24
      };
      reply(packet, 4);
      
      Serial.print(F("Uptime: "));
      Serial.print(uptime/1000);
      Serial.println(F(" seconds"));
      break;
    }
    
    default: {
      //Unknown command; set the error code
      SET_ERROR(UNKNOWN_COMMAND_ERROR);
      uint8_t packet[] = {errorFlags};
      reply(packet, 1);
      
      Serial.println(F("Error: Unknown command"));
      break;
    }
  }
}

/**
 * setMaxChannel - Sets the maximum number of channels to transmit
 * Parameter:
 *    uint16_t newMaxChannel: The maximum number of channels to transmit (1-512)
 */

bool setMaxChannel(uint16_t newMaxChannel) {
  /* Setting max channels to zero disables DMX, so prevent doing that
     accidentally */
  if (newMaxChannel == 0 || newMaxChannel > MAX_DMX) {
    SET_ERROR(INVALID_VALUE_ERROR); //Invalid value
    return false;
  } else if (!TEST_STATUS(DMX_ENABLED_STATUS)) {
    SET_ERROR(DMX_DISABLED_ERROR); //DMX is disabled
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

void startTransmitDMX() {
  DmxSimple.maxChannel(maxChannel); //Start transmitting DMX
  SET_STATUS(DMX_ENABLED_STATUS); //Set the transmit enable flag
}

/**
 * stopTransmitDMX - Disables DMX output
 */

void stopTransmitDMX() {
  DmxSimple.maxChannel(0); //Stop transmitting DMX
  CLEAR_STATUS(DMX_ENABLED_STATUS); //Clear the transmit enable flag
}

/**
 * initShutDown - Shuts down or resets the Arduino
 * Parameter:
 *    bool reset: true to reset the firmware, false to power down
 *
 * Note: Cannot return successfully, for obvious reasons.
 */

void initShutDown(bool reset) {
  if (!TEST_STATUS(RESTRICTED_MODE_STATUS)) {
    SET_ERROR(RESTRICTED_MODE_ERROR); //Last command wasn't no-op
    Serial.println(F("Error: must be in restricted mode"));
    return;
  }

  //Prepare to shut down or reset
  if (reset) {
    Serial.println(F("The system is going down for reboot NOW!"));
  } else {
    Serial.println(F("The system is going down for system halt NOW!"));
  }
  Serial.end(); //Stop serial communication
  //Send EOT to calculator to let it know we are going down
  uint8_t EOT[] = {MACHINE_ID, CMD_EOT, 0, 0}; //End of transmission
  par_put(EOT, 4);
  //Some final cleanup
  stopTransmitDMX();
  digitalWrite(LED_PIN, LOW);

  if (reset) {
    asm volatile ("jmp 0"); //Reset the software (i.e. reboot)
  }
  //else shutdown
  //Set up power down sleep mode
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_bod_disable();
  power_all_disable();
  //sleep_cpu();
  sleep_mode();
  while (true) {
    //Sit-n-spin. Shouldn't get here since there is no way to get out of sleep.
  }
}

/**
 * readTemp - Reads the core temperatue using the built-in temperature sensor.
 * Returns:
 *    int32_t temp: the core temperature in millidegrees Celsius
 *
 * Note: Code from https://code.google.com/p/tinkerit/wiki/SecretThermometer
 */
int32_t readTemp() {
  int32_t result;
  // Read temperature sensor against 1.1V reference
  ADMUX = _BV(REFS1) | _BV(REFS0) | _BV(MUX3);
  delay(2); // Wait for Vref to settle
  ADCSRA |= _BV(ADSC); // Convert
  while (bit_is_set(ADCSRA,ADSC));
  result = ADCL;
  result |= ADCH << 8;
  result = (result - 125) * 1075;
  result = result / 9; //Rough calibration factor
  return result;
}

/******************************************************************************
 * Calculator communication functions
 ******************************************************************************/

/**
 * waitForPacket - Waits for a valid packet to be received and stores received
 * data in packetData.
 */

void waitForPacket(void) {
  bool keepWaiting = true;
  while (keepWaiting) {
    resetLines();
    
    //At first, only get the header so we know the length of the data (if any)
    receive(packetHead, HEADER_LENGTH);
    
    //uint32_t startTime = millis();
    Serial.println();
    Serial.println(F("Received header:"));
    
    for (uint8_t i = 0; i < HEADER_LENGTH; i++) {
      if (packetHead[i] < 0x10) {
        Serial.print(F("0"));
      }
      Serial.print(packetHead[i], HEX);
      Serial.print(F(" "));
    }
    Serial.println();
    
    uint16_t length = packetHead[2] | packetHead[3] << 8;
    
    if(packetHead[1] == CMD_RDY) {//Ready check - required once at startup
      //Serial.println(F("Packet was ready check"));
      SET_STATUS(RECEIVED_HANDSHAKE_STATUS);
      par_put(ACK, 4);
      //Serial.println(F("Sent ACK"));
    } else if (TEST_STATUS(RECEIVED_HANDSHAKE_STATUS) &&
        packetHead[1] == CMD_DATA) { //Data packet - everything after RDY
      //Data packet should always contain data, but this prevents infinite loops
      if (length != 0) {
        receive(packetData, length);
        
        Serial.println(F("Received data:"));
        for (uint16_t i = 0; i < length; i++) {
        if (packetData[i] < 0x10) {
          Serial.print(F("0"));
        }
          Serial.print(packetData[i], HEX);
          Serial.print(F(" "));
        }
        Serial.println();
        
        receive(packetChecksum, 2);
        uint16_t receivedChksm = packetChecksum[0] | packetChecksum[1] << 8;
        //Serial.println(F("Received checksum:"));
        //Serial.println(receivedChksm, HEX);
        uint16_t calculatedChksm = checksum(packetData, length);
        if (calculatedChksm == receivedChksm) {
          //Serial.println(F("Checksum is valid"));
          par_put(ACK, 4);
          //Serial.println(F("Sent ACK"));
          //Serial.print(F("Time elapsed (ms): "));
          //Serial.println(millis() - startTime);
          //Stop waiting and return
          keepWaiting = false;
        } else {
          Serial.print(F("ERROR: Expected checksum: "));
          Serial.println(calculatedChksm, HEX);
          par_put(ERR, 4);
          //Serial.println(F("Requested retransmission"));
        }
      }
    } else {
      /* Either we haven't received the handshake yet or the packet type wasn't
       * recognized.
       */
      //Eat the rest of the packet
      if (length) {
        receive(packetData, length);
        receive(packetChecksum, 2);
      }
      //Send a NAK to indicate the packet was ignored.
      par_put(NAK, 7);
      Serial.println(F("Sent NAK"));
    }
  }
  CLEAR_STATUS(SENT_SHUT_DOWN_WARNING_STATUS);
}

/**
 * receive - Waits to receive the specified length of data successfully.
 * Parameters:
 *    uint8_t *data: a pointer to store the received data
 *    uint16_t length: the length of data to receive
 *
 * Note: Retries until the transmission doesn't time out.
 * Also implements auto shutdown and clearing the restricted mode flag
 * since this is where the program spends a lot of its time waiting.
 * It spends more time in par_get, but I don't want to modify par_get's
 * functionality.
 */

void receive(uint8_t *data, uint16_t length) {
  CLEAR_STATUS(SENT_SHUT_DOWN_WARNING_STATUS);
  while (par_get(data, length)) {
    if (TEST_STATUS(RESTRICTED_MODE_STATUS) &&
        (millis() - lastCmdReceived > RESTRICTED_MODE_TIMEOUT)) {
      CLEAR_STATUS(RESTRICTED_MODE_STATUS); //Clear the no-op flag after 1 second
      Serial.println(F("Leaving restricted mode"));
    }
#if AUTO_SHUT_DOWN_ENABLED
    if (!TEST_STATUS(SENT_SHUT_DOWN_WARNING_STATUS) &&
        ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_WARN_TIME)) {
      //We haven't received a message in a while and haven't sent a warning yet.
      //Send the warning.
      uint8_t statusPacket[] = {MACHINE_ID, CMD_DATA, 1, 0, 0xFE, 0xFE, 0};
      par_put(statusPacket, 7);
      SET_STATUS(SENT_SHUT_DOWN_WARNING_STATUS);
      Serial.println(F("Sent inactivity warning"));
    } else if (TEST_STATUS(SENT_SHUT_DOWN_WARNING_STATUS) &&
        ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_TIME)) {
      //It's been 1 minute since we sent a warning. Time to shut down.
      //Must enable restricted mode to shut down
      Serial.println(F("Shutting down..."));
      SET_STATUS(RESTRICTED_MODE_STATUS);
      initShutDown();
    } else {
      //Do nothing
    }
#endif
  }
}

/**
 * reply - Sends data back to the calculator using the same command as the last
 * one received.
 * Parameters:
 *    volatile uint8_t *data: a pointer to the raw data to send (will be
 *                            formatted as a packet by this function)
 *    uint16_t length: the length of data to send
 *
 * Note: The *data parameter must be volatile so it can reply directly with the
 * dmxBuffer if necessary.
 * Note: Reuses packetHead, packetData, and packetChecksum to save RAM.
 */

void reply(volatile uint8_t *data, uint16_t length) {
  packetHead[0] = MACHINE_ID;
  packetHead[1] = CMD_DATA;
  packetHead[2] = (length + 1) & 0x00FF;
  packetHead[3] = (length + 1) >> 8;
  packetData[0] = cmd;
  for (uint16_t i = 0; i < length; i++) {
    packetData[i + 1] = data[i];
  }
  uint16_t chksm = checksum(packetData, length + 1);
  packetChecksum[0] = chksm & 0x00FF;
  packetChecksum[1] = chksm >> 8;

  /* These par_puts are in conditionals to prevent getting stuck receiving ACK
   * if there was a transmit error.
   */
  uint16_t err = 0;
  if (err = par_put(packetHead, 4)) {
    Serial.print(F("Error sending head: "));
    Serial.println(err);
  } else if (err = par_put(packetData, length + 1)) {
    Serial.print(F("Error sending data: "));
    Serial.println(err);
  } else if (err = par_put(packetChecksum, 2)) {
    Serial.print(F("Error sending checksum: "));
    Serial.println(err);
  } else {
    Serial.println(F("Sent reply"));

    //Receive the ACK
    receive(packetHead, 4);
    Serial.println(F("Received ACK"));
  }
}

/**
 * Arduino to TI linking routines by Christopher "Kerm Martian" Mitchell
 * http://www.cemetech.net/forum/viewtopic.php?t=4771
 * Modified by ajcord to remove commented-out code, reduce oversized variables,
 * and increase style consistency with the rest of the firmware.
 *
 * Do not modify any functionality below this line.
 */

/**
 * resetLines - Resets the ports used for TI linking
 */

void resetLines() {
  pinMode(TI_RING_PIN, INPUT);           // set pin to input
  digitalWrite(TI_RING_PIN, HIGH);       // turn on pullup resistors
  pinMode(TI_TIP_PIN, INPUT);            // set pin to input
  digitalWrite(TI_TIP_PIN, HIGH);        // turn on pullup resistors
}

/**
 * par_put - Transmits a length of data over the link port.
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

  //Serial.print(F("Sending "));
  for(j = 0; j < length; j++) {
    byte = data[j];
    //Serial.print(byte, HEX);
    //Serial.print(F(" "));

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
  //Serial.println(F(" "));
  return 0;
}

/**
 * par_get - Receives a length of data over the link port.
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
      pinMode(TI_RING_PIN, INPUT); // set pin to input
      digitalWrite(TI_RING_PIN, HIGH); // turn on pullup resistors
      pinMode(TI_TIP_PIN, INPUT); // set pin to input
      digitalWrite(TI_TIP_PIN, HIGH); // turn on pullup resistors
    }
    data[j] = byteout;
  }
  return 0;
}

/**
 * checksum - Calculates the 16-bit checksum of some data.
 * Parameters:
 *    uint8_t *data: A pointer to the data to sum
 *    uint16_t length: The length of data to sum
 * Returns:
 *    uint16_t checksum: The lower 16 bits of the sum of the data
 * Note: Checksum algorithm adapted from the TI Link Guide.
 * http://merthsoft.com/linkguide/ti83+/packet.html
 * Ported to Arduino by ajcord.
 */
uint16_t checksum(uint8_t *data, uint16_t length) {
  uint16_t checksum = 0;
  for(uint16_t x = 0; x < length; x++) {
    checksum += data[x];  //overflow automatically limits to 16 bits
  }
  return checksum;
}
