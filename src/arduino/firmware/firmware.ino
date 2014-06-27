/**
 * The DATA Project
 * Arduino firmware v0.3
 *
 * By Alex Cordonnier (ajcord)
 *
 * This file contains the code that communicates with the calculator
 * and processes received commands.
 *
 * Last modified June 25, 2014
 */

/******************************************************************************
 * Includes
 ******************************************************************************/

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
#define AUTO_SHUT_DOWN_TIME             21600000
#define AUTO_SHUT_DOWN_WARN_TIME        (AUTO_SHUT_DOWN_TIME - 60000)
#define RESTRICTED_MODE_TIMEOUT         1000

//Version numbers. High nibble = major version, low nibble = minor version.
#define PROTOCOL_VERSION      0x02
#define FIRMWARE_VERSION      0x03

//Compile-time options
#define AUTO_SHUT_DOWN_ENABLED      1

/******************************************************************************
 * Function declarations
 ******************************************************************************/

//DMX and device utility
bool updateChannel(uint16_t channel, uint8_t newValue);
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
static uint16_t par_put(const uint8_t *data, uint32_t length);
static uint16_t par_get(uint8_t *data, uint32_t length);
uint16_t checksum(uint8_t *data, uint16_t length);

/******************************************************************************
 * Global variables
 ******************************************************************************/

//DMX
uint16_t maxChannel;
//uint8_t DMX[512] = {0}; //Stores a byte buffer of the entire DMX universe

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
uint8_t cmd = 0; //Stores the last command received
uint32_t lastCmdReceived = 0; //Stores the time the last command was received
uint32_t noOpReceived = 0; //Stores the time the last no-op was received
const uint8_t ACK[] = {MACHINE_ID, 0x56, 0, 0}; //Acknowledge
const uint8_t NAK[] = {MACHINE_ID, 0x36, 1, 0, 1, 1, 0}; //Skip/Exit
const uint8_t CTS[] = {MACHINE_ID, 0x09, 0, 0}; //Clear to send
const uint8_t ERR[] = {MACHINE_ID, 0x5A, 0, 0}; //Checksum error
uint8_t packetHead[HEADER_LENGTH] = {0};
uint8_t packetData[PACKET_DATA_LENGTH] = {0}; //Input buffer for packet data from calculator, minus the header and the checksum
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
}

void loop() {
  
  waitForPacket();
  lastCmdReceived = millis();
  
  cmd = packetData[0];
  Serial.print(F("Cmd: "));
  Serial.println(cmd, HEX);
  
  switch (cmd) {
    case 0x00: {
      //No-op. Allows protected commands to be executed for one second.
      //Serial.println(F("Received no-op"));
      SET_STATUS(RESTRICTED_MODE_STATUS);
      uint8_t packet[] = {0}; //Contents will not be sent, but reply requires packet data
      reply(packet, 0); //Just echo the no-op to acknowledge it
      //free(packet);
      noOpReceived = millis();
      break;
    }
    case 0x01: {
      //Heartbeat. Same as no-op but doesn't enable protected commands.
      uint8_t packet[] = {0}; //Contents will not be sent, but reply requires packet data
      reply(packet, 0); //Just echo the request to acknowledge it
      //Serial.println(F("Sent heartbeat"));
      break;
    }
    case 0x10: //acts like an OR because switch executes the code until a break
    case 0x11: {
      //Sets a single channel
      updateChannel((cmd & 1)*256 + packetData[1], packetData[2]);
      //Serial.println(F("Updated channel"));
      break;
    }
    case 0x20:
    case 0x21: {
      //Sets 256 channels at once
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[(cmd & 1)*256 + i] = packetData[i + 1];
      }
      //Serial.println(F("Updated 256 channels"));
      break;
    }
    case 0x22: {
      //Sets 512 channels at once
      for (uint16_t i = 0; i < 512; i++) {
        dmxBuffer[i] = packetData[i + 1];
      }
      //Serial.println(F("Updated 512 channels"));
      break;
    }
    case 0x23: {
      //Sets a block of channels at once
      uint16_t length = packetData[1]; //Stores the number of channels to set, starting from 0
      for (uint16_t i = 0; i < length; i++) {
        dmxBuffer[i] = packetData[i + 2];
      }
      //Serial.print(F("Updated "));
      //Serial.print(length);
      //Serial.println(F(" channels"));
      break;
    }
    case 0x24: {
      //Sets all channels to the same value
      for (uint16_t i = 0; i < 512; i++) {
        dmxBuffer[i] = packetData[1]; //Set each channel to the new value
      }
      //Serial.print(F("Set all channels to "));
      //Serial.println(packetData[1]);
      break;
    }
    case 0x25: {
      //Increments all channels by a value
      uint16_t newValue = 0;
      for (uint16_t i = 0; i < 512; i++) {
        newValue = dmxBuffer[i] + packetData[1];
        if (newValue <= 255) { //Prevent an overflow
          dmxBuffer[i] = newValue;
        } else {
          dmxBuffer[i] = 0xFF; //If the value is too large, set it to FF
        }
      }
      //Serial.print(F("Incremented all channels by "));
      //Serial.println(packetData[1]);
      break;
    }
    case 0x26: {
      //Decrements all channels by a value
      uint16_t newValue = 0;
      for (uint16_t i = 0; i < 512; i++) {
        newValue = dmxBuffer[i] - packetData[1];
        if (newValue >= 0) { //Prevent an underflow
          dmxBuffer[i] = newValue;
        } else {
          dmxBuffer[i] = 0; //If the value is too small, set it to 0
        }
      }
      //Serial.print(F("Decremented all channels by "));
      //Serial.println(packetData[1]);
      break;
    }
    case 0x28: {
      //Start a digital blackout
      DmxSimple.startDigitalBlackout();
      SET_STATUS(DIGITAL_BLACKOUT_ENABLED_STATUS);
      //Serial.println(F("Started digital blackout"));
      break;
    }
    case 0x29: {
      //End a digital blackout
      DmxSimple.stopDigitalBlackout();
      CLEAR_STATUS(DIGITAL_BLACKOUT_ENABLED_STATUS);
      //Serial.println(F("Stopped digital blackout"));
      break;
    }
    case 0x30: {
      //Copy channel data from 256-511 to 0-255
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[i] = dmxBuffer[i + 256];
      }
      //Serial.println(F("Copied 256-511 to 0-255"));
      break;
    }
    case 0x31: {
      //Copy channel data from 0-255 to 256-511
      for (uint16_t i = 0; i < 256; i++) {
        dmxBuffer[i + 256] = dmxBuffer[i];
      }
      //Serial.println(F("Copied 0-255 to 256-511"));
      break;
    }
    case 0x32: {
      //Exchange channel data between 0-255 and 256-511
      //FIXME: Seems to sometimes set channel 256 to 0x32. Can't seem to reproduce anymore, though.
      uint8_t temp = 0;
      for (uint16_t i = 0; i < 256; i++) {
        //Swap the values of each low channel with its high counterpart
        temp = dmxBuffer[i];
        dmxBuffer[i] = dmxBuffer[i + 256];
        dmxBuffer[i + 256] = temp;
      }
      //Serial.println(F("Exchanged 0-255 with 256-511"));
      break;
    }
    case 0x40:
    case 0x41: {
      //Reply with channel value for specific channel
      uint8_t packet[] = {dmxBuffer[(cmd & 1)*256 + packetData[1]]};
      //Serial.print(F("Channel "));
      //Serial.print((cmd & 1)*256 + packetData[1]);
      //Serial.print(F(" is at "));
      //Serial.println(dmxBuffer[(cmd & 1)*256 + packetData[1]]);
      reply(packet, 1);
      break;
    }
    case 0x42: {
      //Reply with all channel values
      reply(dmxBuffer, 512);
      /*
      Serial.println(F("Channel values:"));
      for (uint16_t i = 0; i < 512; i++) {
        Serial.print(dmxBuffer[i]);
        Serial.print(",");
      }
      Serial.println();
      */
      break;
    }

    case 0x5A: {
      //Prints a newline. Used for debugging in the Serial Monitor.
      //Serial.println();
      //Now toggles the LED instead.
      TOGGLE_STATUS(LED_STATUS);
      digitalWrite(LED_PIN, TEST_STATUS(LED_STATUS));
      break;
    }

    /* Intermediate keywords reserved for future use */

    case 0xE0: {
      //Stop transmitting DMX
      stopTransmitDMX();
      //Serial.println(F("Stopped DMX"));
      break;
    }
    case 0xE1: {
      //Start transmitting DMX
      startTransmitDMX();
      //Serial.println(F("Started DMX"));
      break;
    }
    case 0xE2:
    case 0xE3: {
      //Set max channels to transmit
      setMaxChannel((cmd & 1)*256 + packetData[1]); //Set the max number of channels
      //Serial.print(F("Max channels now "));
      //Serial.println((cmd & 1)*256 + packetData[1]);
      break;
    }
    case 0xF0: {
      //Initiate safe shutdown sequence - only works directly after a no-op
      initShutDown();
      break;
    }
    case 0xF1: {
      //Initiate soft reset - only works directly after a no-op
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
      uint8_t packet[] = {PROTOCOL_VERSION, FIRMWARE_VERSION};
      reply(packet, 2);
      Serial.print(F("Protocol version: "));
      Serial.println(PROTOCOL_VERSION, HEX);
      Serial.print(F("Firmware version: "));
      Serial.println(FIRMWARE_VERSION, HEX);
      break;
    }
    case 0xFB: {
      //Reply with protocol version number
      uint8_t packet[] = {PROTOCOL_VERSION};
      reply(packet, 1);
      Serial.print(F("Protocol version: "));
      Serial.println(PROTOCOL_VERSION, HEX);
      break;
    }
    case 0xFC: {
      //Reply with firmware version number
      uint8_t packet[] = {FIRMWARE_VERSION};
      reply(packet, 1);
      Serial.print(F("Firmware version: "));
      Serial.println(FIRMWARE_VERSION, HEX);
      break;
    }
    case 0xFD: {
      //Reply with current temperature
      uint32_t temp = readTemp();
      uint8_t packet[] = {
        (temp & 0xFF000000) >> 24,
        (temp & 0xFF0000) >> 16,
        (temp & 0xFF00) >> 8,
        (temp & 0xFF)
      };
      reply(packet, 4);
      Serial.print(F("Current temperature: "));
      Serial.println(temp);
      break;
    }
    default: {
      //Unknown command; set the error code
      SET_ERROR(UNKNOWN_COMMAND_ERROR);
      uint8_t packet[] = {errorFlags};
      reply(packet, 1);
      //Serial.println(F("Unknown command"));
      break;
    }
  }
}

/**
 * updateChannel - Updates a single channel
 * Parameters:
 *    uint16_t channel: The channel to set (0-511)
 *    uint8_t newValue: The new channel value
 */

bool updateChannel(uint16_t channel, uint8_t newValue) {
  /* If DMX is disabled and we call DmxSimple.write, it will become re-enabled,
     so check before we call it. */
  if (channel >= MAX_DMX) {
    //The channel is invalid
    SET_ERROR(INVALID_VALUE_ERROR); //Invalid value
    return false;
  }
  if (TEST_STATUS(DMX_ENABLED_STATUS)) {
    dmxBuffer[channel] = newValue;
    //DmxSimple.write(channel+1, newValue); //DmxSimple starts counting at 1. How strange.
    Serial.print(F("Updated channel "));
    Serial.print(channel);
    Serial.print(F(" to "));
    Serial.println(newValue);
    return true;
  } else {
    SET_ERROR(DMX_DISABLED_ERROR); //DMX is disabled
    return false;
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
 * initShutDown - Shuts down the Arduino
 * Parameter:
 *    bool reset: true to reset the firmware, false to hang (shutdown)
 *
 * Note: Cannot return successfully, for obvious reasons.
 */

void initShutDown(bool reset) {
  if (!TEST_STATUS(RESTRICTED_MODE_STATUS)) {
    SET_ERROR(RESTRICTED_MODE_ERROR); //Last command wasn't no-op
    return;
  }
  //statusFlags = 0xFE; //Getting ready to power down

  stopTransmitDMX();

  //statusFlags = 0xFF; //Ready for power down
  uint8_t statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFF, 0xFF, 0};
  par_put(statusPacket, 7);
  uint8_t EOT[] = {MACHINE_ID, 0x92, 0, 0}; //End of transmission
  par_put(EOT, 4);
  Serial.println(F("System is going down NOW!"));
  Serial.end(); //Stop serial communication
  digitalWrite(LED_PIN, LOW);
  if (reset) {
    asm volatile ("jmp 0"); //Reset the software (i.e. reboot)
  }
  while (true) {
    //Go into an infinite loop (i.e. shutdown)
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
  result |= ADCH<<8;
  result = (result - 125) * 1075;
  return result;
}

/*******************************************************************************
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
      Serial.print(packetHead[i], HEX);
      Serial.print(",");
    }
    Serial.println();
    
    uint16_t length = packetHead[2] + packetHead[3]*256;
    
    if(packetHead[1] == 0x68) {//Ready check - only required in the initial handshake
      //Serial.println(F("Packet was ready check"));
      SET_STATUS(RECEIVED_HANDSHAKE_STATUS);
      par_put(ACK, 4);
      //Serial.println(F("Sent ACK"));
    } else if (TEST_STATUS(RECEIVED_HANDSHAKE_STATUS) &&
        packetHead[1] == 0x15) { //Data packet - used for everything except the handshake
      if (length != 0) {
        receive(packetData, length);
        
        Serial.println(F("Received data:"));
        for (uint16_t i = 0; i < length; i++) {
          Serial.print(packetData[i], HEX);
          Serial.print(",");
        }
        Serial.println();
        
        receive(packetChecksum, 2);
        uint16_t receivedChksm = packetChecksum[0] + packetChecksum[1]*256;
        //Serial.println(F("Received checksum:"));
        //Serial.println(receivedChksm, HEX);
        uint16_t calculatedChksm = checksum(packetData, length);
        if (calculatedChksm == receivedChksm) {
          //Serial.println(F("Checksum is valid"));
          par_put(ACK, 4);
          Serial.println(F("Sent ACK"));
          //Serial.print(F("Time elapsed (ms): "));
          //Serial.println(millis() - startTime);
          //Stop waiting and return
          keepWaiting = false;
        } else {
          //Serial.print(F("ERROR: Expected checksum: "));
          //Serial.println(calculatedChksm, HEX);
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
      //Serial.println(F("Sent NAK"));
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
 * It probably spends more time in par_get, but I don't want to modify par_get's
 * functionality.
 */

void receive(uint8_t *data, uint16_t length) {
  CLEAR_STATUS(SENT_SHUT_DOWN_WARNING_STATUS);
  while (par_get(data, length)) {
    if (millis() - lastCmdReceived > RESTRICTED_MODE_TIMEOUT) {
      CLEAR_STATUS(RESTRICTED_MODE_STATUS); //Clear the no-op flag after 1 second
    }
#if AUTO_SHUT_DOWN_ENABLED
    if (!TEST_STATUS(SENT_SHUT_DOWN_WARNING_STATUS) &&
        ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_WARN_TIME)) {
      //We haven't received a message in a while and haven't sent a warning yet.
      //Send the warning.
      uint8_t statusPacket[] = {MACHINE_ID, 0x15, 1, 0, 0xFE, 0xFE, 0};
      par_put(statusPacket, 7);
      free(&statusPacket);
      SET_STATUS(SENT_SHUT_DOWN_WARNING_STATUS);
      //Serial.println(F("Sent inactivity warning"));
    } else if (TEST_STATUS(SENT_SHUT_DOWN_WARNING_STATUS) &&
        ((millis() - lastCmdReceived) > AUTO_SHUT_DOWN_TIME)) {
      //It's been 1 minute since we sent a warning. Time to shut down.
      //Must enable restricted mode to shut down
      SET_STATUS(RESTRICTED_MODE_STATUS);
      //Serial.println(F("Shutting down..."));
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
 */

void reply(volatile uint8_t *data, uint16_t length) {
  packetHead[0] = MACHINE_ID;
  packetHead[1] = 0x15;
  packetHead[2] = length & 0x00FF;
  packetHead[3] = length >> 8;
  packetData[0] = cmd;
  for (uint16_t i = 0; i < length; i++) {
    packetData[i + 1] = data[i];
  }
  uint16_t chksm = checksum(packetData, length + 1);
  packetChecksum[0] = chksm & 0x00FF;
  packetChecksum[1] = chksm >> 8;
  par_put(packetHead, 4);
  par_put(packetData, length + 1);
  par_put(packetChecksum, 2);
}

/**
 * Arduino to TI linking routines by Christopher "Kerm Martian" Mitchell
 * http://www.cemetech.net/forum/viewtopic.php?t=4771
 * Modified by ajcord to remove commented-out code, reduce oversized variables,
 * and increase style consistency with the rest of the firmware.
 */

void resetLines() {
   pinMode(TI_RING_PIN, INPUT);           // set pin to input
   digitalWrite(TI_RING_PIN, HIGH);       // turn on pullup resistors
   pinMode(TI_TIP_PIN, INPUT);            // set pin to input
   digitalWrite(TI_TIP_PIN, HIGH);        // turn on pullup resistors
}

static uint16_t par_put(const uint8_t *data, uint32_t length) {
   uint8_t bit;
   uint32_t j;
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
            digitalWrite(TI_TIP_PIN, LOW);      //should already be set because of the pullup resistor register
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

static uint16_t par_get(uint8_t *data, uint32_t length) {
   uint8_t bit;
   uint32_t j;
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
            digitalWrite(TI_TIP_PIN, LOW);      //should already be set because of the pullup resistor register
            previousMillis = 0;
            while (digitalRead(TI_RING_PIN) == LOW) {            //wait for the other one to go low
               if (previousMillis++ > TIMEOUT)
                  return ERR_READ_TIMEOUT + 10 + j + 100 * bit;
            }
            digitalWrite(TI_RING_PIN,HIGH);
         } else {
            byteout = (byteout >> 1) & 0x7F;
            pinMode(TI_RING_PIN, OUTPUT);
            digitalWrite(TI_RING_PIN, LOW);      //should already be set because of the pullup resistor register
            previousMillis = 0;
            while (digitalRead(TI_TIP_PIN) == LOW) {
               if (previousMillis++ > TIMEOUT)
                  return ERR_READ_TIMEOUT + 20 + j + 100 * bit;
            }
            digitalWrite(TI_TIP_PIN, HIGH);
         }
         pinMode(TI_RING_PIN, INPUT);           // set pin to input
         digitalWrite(TI_RING_PIN, HIGH);       // turn on pullup resistors
         pinMode(TI_TIP_PIN, INPUT);            // set pin to input
         digitalWrite(TI_TIP_PIN, HIGH);        // turn on pullup resistors
      }
      data[j] = byteout;
   }
   return 0;
}

/**
 * Checksum algorithm from the TI Link Guide
 * http://merthsoft.com/linkguide/ti83+/packet.html
 * Adapted for Arduino by ajcord.
 */
uint16_t checksum(uint8_t *data, uint16_t length) {
  uint16_t checksum = 0;
  for(uint16_t x = 0; x < length; x++) {
    checksum += data[x];  //overflow automatically limits to 16 bits
  }
  return checksum;
}