/* The DATA Project
 *
 * By Alex Cordonnier
 *
 * Arduino firmware v0.1
 *
 * After uploading to Arduino, switch to Serial Monitor and set the baud rate
 * to 9600.
 */

#include <DmxSimple.h>
#define ledPin 13

const byte VERSION = 0x01; //Stores the version number. High nibble = major version, low nibble = minor version. Please increment only on breaking changes.

word maxChannels;
byte DMX[512] = {0}; //Stores a buffer of the DMX universe

byte cmd = 0; //Stores the last command received

bool wasLastCmdNoOp = false; //Stores whether the last command was a no-op
bool isDigitalBlackoutEnabled = false; //Stores whether the digital blackout is active
bool isDmxEnabled = true; //Stores whether the DMX output is enabled

byte lastError = 0; //Stores the most recent error code
byte currentStatus = 0x80; //Stores the current Arduino status

word channel = 0; //Stores the channel the command is acting on (if applicable)
byte value = 0; //Stores the new value of the channel (if applicable)

void setup() {
  //Set the max channels
  SetMaxChannels(256);
  //Start the serial communication with the calculator or the Serial Monitor (or both), depending which is connected
  Serial.begin(9600);
  Serial.println("Ready!"); //This will eventually need changed to a single byte, but it works for now
  ResetStatus();
}

void loop() {
  while(!Serial.available()); //Wait for the command byte to be sent
  cmd = Serial.read();
  switch (cmd) {
    case 0x00: {
      //No-op
      Serial.write(0xFF);
      break;
    }
    case 0x01: {
      //Heartbeat
      Serial.write(0xFF);
      break;
    }
    case 0x10: //acts like an OR because it executes the code until a break
    case 0x11: {
      //Sets a single channel
      while(!Serial.available()); //Wait for the low channel byte to be sent
      channel = (cmd-0x10)*256 + Serial.read(); //The calculation with cmd is to use a 9-bit value for the channel (to address 512 channels)
      while(!Serial.available()); //Wait for the value byte to be sent
      UpdateChannel(channel, Serial.read());
      channel = 0;
      break;
    }
    case 0x20:
    case 0x21: {
      //Sets 256 channels at once
      for (word i = 0; i < 512; i++) { //Get 256 channel values
        while (!Serial.available()); //Wait for the next channel value
        DMX[(cmd-0x20)*256 + i] = Serial.read(); //Save the most recent DMX value
      }
      UpdateDMX();
      break;
    }
    case 0x22: {
      //Sets a block of channels at once
      while(!Serial.available()); //Wait for the length of data
      byte len = Serial.read(); //Stores the number of channels to set, starting from 0
      for (byte i = 0; i < len; i++) {
        while(!Serial.available()); //Wait for the next channel value
        DMX[i] = Serial.read();
      }
      UpdateDMX();
      break;
    }
    case 0x23: {
      //Sets all channels to the same value
      while(!Serial.available()); //Wait for the channel value
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
      while(!Serial.available()); //Wait for the increment value
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
      while(!Serial.available()); //Wait for the increment value
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
      while (!Serial.available()); //Wait for the channel value
      channel = Serial.read(); //Stores the low byte of the channel to read
      Serial.write(DMX[(cmd-0x40)*256 + channel]); //Reply with the channel value
      break;
    }
    case 0x42: {
      //Reply with all channel values
      Serial.write(DMX, 512);
      break;
    }
    
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
      while (!Serial.available()); //Wait for the low byte of the number of channels
      SetMaxChannels((cmd-0xE2)*256 + Serial.read()); //Set the max number of channels
      break;
    }
    case 0xF0: {
      //Initiate safe shutdown sequence - only works directly after a no-op
      if (wasLastCmdNoOp) {
      
        DmxSimple.maxChannel(0); //Disable DMX transmission
        
        currentStatus = 0xFF;
        Serial.write(0xFF);
      }
      break;
    }
    case 0xF1: {
      //Initiate soft reset - only works directly after a no-op
      if (wasLastCmdNoOp) {
        asm volatile ("jmp 0");
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
      //Reply with firmware version
      Serial.write(VERSION);
      break;
    }
    default: {
      //Unknown command; set the error code
      
      break;
    }
  }
  wasLastCmdNoOp = (cmd == 0x00);
}

void UpdateChannel(word chan, byte val) {
  //Updates a single channel in both the DMX buffer and DmxSimple
  DMX[chan] = val;
  DmxSimple.write(chan, val);
}

void UpdateDMX() {
  //Updates the DmxSimple channels with the stored values in DMX[]
  if (isDmxEnabled && !isDigitalBlackoutEnabled) { //Prevent updating the DMX output when digital blackout is enabled or DMX is disabled
    for (int i=0; i < maxChannels; i++) {
      DmxSimple.write(i, DMX[i]);
    }
  }
}

void SetMaxChannels(word maxCh) {
  //Sets the maximum number of channels to transmit
  if (isDmxEnabled && maxCh > 0) { //Setting max channels to zero disables DMX, so prevent doing that accidentally
    maxChannels = maxCh;
    DmxSimple.maxChannel(maxCh);
  }
}

void ResetStatus() {
  //Sets the current status based on the value of flags
  if (!isDmxEnabled) {
    currentStatus = 3;
  } else if (isDigitalBlackoutEnabled) {
    currentStatus = 2;
  } else {
    currentStatus = 1;
  }
}
