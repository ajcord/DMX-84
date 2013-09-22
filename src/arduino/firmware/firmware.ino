/* The DATA Project
 *
 * By Alex Cordonnier
 *
 * Arduino firmware v0.1
 *
 * After uploading to the Arduino, switch to Serial Monitor and set the baud
 * rate to 9600.
 */

#include <DmxSimple.h>
#define ledPin 13

/******************************************************************************
 * Function declarations
 ******************************************************************************/

bool WaitToReceive(int timeout = 1000);
bool UpdateChannel(word chan, byte val);
bool UpdateDMX();
bool SetMaxChannels(word maxCh = 128);
void ResetStatus();

/******************************************************************************
 * Variable declarations
 ******************************************************************************/

const byte PROTOCOL_VERSION = 0x01; //Stores the protocol version number. High nibble = major version, low nibble = minor version.
const byte FIRMWARE_VERSION = 0x01; //Stores the firmware version number. High nibble = major version, low nibble = minor version.

word maxChannels;
byte DMX[512] = {0}; //Stores a byte buffer of the DMX universe

byte cmd = 0; //Stores the last command received
unsigned long lastCmdReceived; //Stores the time the last command was received

bool wasLastCmdNoOp = false; //Stores whether the last command was a no-op
bool isDigitalBlackoutEnabled = false; //Stores whether the digital blackout is active
bool isDmxEnabled = true; //Stores whether the DMX output is enabled

byte lastError = 0; //Stores the most recent error code
byte currentStatus = 0x80; //Stores the current Arduino status

word channel = 0; //Stores the channel the command is acting on (if applicable)
byte value = 0; //Stores the new value of the channel (if applicable)

/******************************************************************************
 * Functions
 ******************************************************************************/

void setup() {
  //Set the max channels
  SetMaxChannels(); //Set the max channels to transmit
  
  //Start the serial communication with the calculator or the Serial Monitor (or both), depending which is connected
  Serial.begin(9600);
  Serial.setTimeout(1000);
  ResetStatus();
  
  
  Serial.println("Ready"); //This will eventually need changed to a single byte, but it works for now
}

void loop() {
  
  bool sentWarning = false; //Stores whether the calculator has been warned
  
  while(!Serial.available()) { //Wait for the command byte to be sent
    if (millis() - lastCmdReceived > 1000) wasLastCmdNoOp = false; //Clear the no-op flag after 1 second
    if (!sentWarning && millis() - lastCmdReceived > 21600000) { //If the last command received was over 6 hours ago
      Serial.write(0x2A); //Warn the calculator to verify activity
      sentWarning = true;
    } else if (sentWarning && millis() - lastCmdReceived > 21660000) { //After the first warning, wait 60 seconds and then get ready to power down
      InitShutDown();
    }
  }
  
  lastCmdReceived = millis();
  cmd = Serial.read();
  
  switch (cmd) {
    case 0x00: {
      //No-op. Allows protected command to be executed next.
      Serial.write(0xFF);
      break;
    }
    case 0x01: {
      //Heartbeat. Same as no-op but doesn't enable protected commands.
      Serial.write(0xFF);
      break;
    }
    case 0x10: //acts like an OR because switch executes the code until a break
    case 0x11: {
      //Sets a single channel
      WaitToReceive(); //Wait for the low channel byte to be sent
      channel = (cmd-0x10)*256 + Serial.read(); //The calculation with cmd is to use a 9-bit value for the channel (to address 512 channels)
      WaitToReceive(); //Wait for the value byte to be sent
      UpdateChannel(channel, Serial.read());
      channel = 0;
      break;
    }
    case 0x20:
    case 0x21: {
      //Sets 256 channels at once
      if (Serial.readBytes((char *)(DMX + (cmd-0x20)*256), 256)) { //Must cast DMX byte buffer as a char buffer because Serial.readBytes is not overloaded
        UpdateDMX();
      } else {
        lastError = 0x11; //No valid data found
      }
      /*
      for (word i = 0; i < 512; i++) { //Get 256 channel values
        while (!Serial.available()); //Wait for the next channel value
        DMX[(cmd-0x20)*256 + i] = Serial.read(); //Save the most recent DMX value
      }
      UpdateDMX();*/
      break;
    }
    case 0x22: {
      //Sets a block of channels at once
      WaitToReceive(); //Wait for the length of data
      int len = Serial.read(); //Stores the number of channels to set, starting from 0
      if (Serial.readBytes((char *)DMX, len)) {
        UpdateDMX();
      } else {
        lastError = 0x11; //No valid data found
      }
      /*
      for (byte i = 0; i < len; i++) {
        while(!Serial.available()); //Wait for the next channel value
        DMX[i] = Serial.read();
      }
      UpdateDMX();*/
      break;
    }
    case 0x23: {
      //Sets all channels to the same value
      WaitToReceive(); //Wait for the channel value
      value = Serial.read(); //Get the channel value
      for (word i = 0; i < 512; i++) {
        DMX[i] = value; //Set each channel to the new value
      }
      UpdateDMX();
      value = 0;
      break;
    }
    case 0x24: {
      //Increments all channels by a value
      WaitToReceive(); //Wait for the increment value
      value = Serial.read(); //Get the increment amount
      for (word i = 0; i < 512; i++) {
        if (DMX[i] < (255 - value)) { //Prevent an overflow
          DMX[i] += value;
        } else {
          DMX[i] = 255; //If the value is too large, set it to 255
        }
      }
      UpdateDMX();
      value = 0;
      break;
    }
    case 0x25: {
      //Decrements all channels by a value
      WaitToReceive(); //Wait for the increment value
      value = Serial.read(); //Get the increment amount
      for (word i = 0; i < 512; i++) {
        if (DMX[i] >= value) { //Prevent an underflow
          DMX[i] -= value;
        } else {
          DMX[i] = 0; //If the value is too small, set it to 0
        }
      }
      UpdateDMX();
      value = 0;
      break;
    }
    case 0x28: {
      //Start a digital blackout
      for (int i=0; i < maxChannels; i++) {
        DmxSimple.write(i, 0);
      }
      isDigitalBlackoutEnabled = true;
      ResetStatus();
      break;
    }
    case 0x29: {
      //End a digital blackout
      isDigitalBlackoutEnabled = false;
      UpdateDMX();
      ResetStatus();
      break;
    }
    case 0x30: {
      //Copy channel data from 256-512 to 0-255
      for (byte i = 0; i < 256; i++) {
        DMX[i] = DMX[i+256];
      }
      UpdateDMX();
      break;
    }
    case 0x31: {
      //Copy channel data from 0-255 to 256-512
      for (byte i = 0; i < 256; i++) {
        DMX[i+256] = DMX[i];
      }
      UpdateDMX();
      break;
    }
    case 0x32: {
      //Exchange channel data between 0-255 and 256-512
      byte temp = 0;
      for (int i = 0; i < 256; i++) {
        //Swap the values of each low channel with its high counterpart
        temp = DMX[i];
        DMX[i] = DMX[i + 256];
        DMX[i+ 256] = temp;
      }
      UpdateDMX();
      break;
    }
    case 0x40:
    case 0x41: {
      //Reply with channel value for specific channel
      WaitToReceive(); //Wait for the channel value
      channel = Serial.read(); //Stores the low byte of the channel to read
      Serial.write(DMX[(cmd-0x40)*256 + channel]); //Reply with the channel value
      break;
    }
    case 0x42: {
      //Reply with all channel values
      Serial.write(DMX, 512);
      break;
    }
    
    /* Intermediate keywords reserved for future use */
    
    case 0xE0: {
      //Stop transmitting DMX
      DmxSimple.maxChannel(0);
      ResetStatus();
      break;
    }
    case 0xE1: {
      //Start transmitting DMX
      DmxSimple.maxChannel(maxChannels);
      ResetStatus();
      break;
    }
    case 0xE2:
    case 0xE3: {
      //Set max channels to transmit
      WaitToReceive(); //Wait for the low byte of the number of channels
      SetMaxChannels((cmd-0xE2)*256 + Serial.read()); //Set the max number of channels
      break;
    }
    case 0xF0: {
      //Initiate safe shutdown sequence - only works directly after a no-op
      InitShutDown();
      break;
    }
    case 0xF1: {
      //Initiate soft reset - only works directly after a no-op
      if (wasLastCmdNoOp) {
        asm volatile ("jmp 0");
      } else {
        lastError = 0x03; //Last command wasn't no-op
      }
      break;
    }
    case 0xF8: {
      //Reply with current status
      Serial.write(currentStatus);
      break;
    }
    case 0xF9: {
      //Reply with last error code
      Serial.write(lastError);
      break;
    }
    case 0xFA: {
      //Reply with version numbers
      Serial.write(PROTOCOL_VERSION);
      Serial.write(FIRMWARE_VERSION);
      break;
    }
    default: {
      //Unknown command; set the error code
      lastError = 0x21;
      break;
    }
  }
  wasLastCmdNoOp = (cmd == 0x00);
}




bool WaitToReceive(int timeout) {
  //Waits for either a timeout or for data to be available
  unsigned long startTime = millis();
  while ((millis() - startTime < timeout) && !Serial.available());
  if (!Serial.available()) lastError = 0x10; //Transmission timeout occurred
  return Serial.available();
}

bool UpdateChannel(word chan, byte val) {
  //Updates a single channel in both the DMX buffer and DmxSimple
  if (isDmxEnabled) { //If DMX is disabled and we call DmxSimple.write, it will become re-enabled, so check before we call it
    DMX[chan] = val;
    DmxSimple.write(chan+1, val); //DmxSimple starts counting at 1. How strange.
    return true;
  } else {
    lastError = 0x02; //DMX is disabled
    return false;
  }
}

bool UpdateDMX() {
  //Updates the DmxSimple channels with the stored values in DMX[]
  if (isDmxEnabled && !isDigitalBlackoutEnabled) { //Prevent updating the DMX output when digital blackout is enabled or DMX is disabled
    for (int i=0; i < maxChannels; i++) {
      DmxSimple.write(i, DMX[i]);
    }
    return true;
  } else {
    if (isDigitalBlackoutEnabled) lastError = 0x01; //Digital blackout is enabled
    if (!isDmxEnabled) lastError = 0x02; //DMX is disabled
    return false;
  }
}

bool SetMaxChannels(word maxCh) {
  //Sets the maximum number of channels to transmit
  if (isDmxEnabled && maxCh > 0) { //Setting max channels to zero disables DMX, so prevent doing that accidentally
    maxChannels = maxCh;
    DmxSimple.maxChannel(maxCh);
    return true;
  } else {
    if (maxCh <= 0) lastError = 0x20; //Invalid value
    if (!isDmxEnabled) lastError = 0x02; //DMX is disabled
    return false;
  }
}

void ResetStatus() {
  //Sets the current status based on the value of flags
  if (!isDmxEnabled) {
    currentStatus = 0x03;
  } else if (isDigitalBlackoutEnabled) {
    currentStatus = 0x02;
  } else {
    currentStatus = 0x01;
  }
}

void InitShutDown() {
if (wasLastCmdNoOp) {
    currentStatus = 0xFE; //Getting ready to power down

    DmxSimple.maxChannel(0); //Disable DMX transmission
    
    currentStatus = 0xFF; //Ready for power down
    Serial.write(0xFF); //Tell the calculator that we're ready for power down
    Serial.end(); //Stop serial communication
    while(false); //Go into an infinite loop
  } else {
    lastError = 0x03; //Last command wasn't no-op
  }
}