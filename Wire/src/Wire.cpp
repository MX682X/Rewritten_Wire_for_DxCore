/*
  TwoWire.cpp - TWI/I2C library for Wiring & Arduino
  Copyright (c) 2006 Nicholas Zambetti.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

  Modified 2012 by Todd Krein (todd@krein.org) to implement repeated starts
  Modified 2017 by Chuck Todd (ctodd@cableone.net) to correct Unconfigured Slave Mode reboot
  Modified 2019-2021 by Spence Konde for megaTinyCore and DxCore.
  This version is part of megaTinyCore and DxCore; it is not expected
  to work with other hardware or cores without modifications.
  Modified 2021 by MX682X for megaTinyCore and DxCore. 
  Added Support for Simultanious master/slave, dual mode and Wire1.
*/

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
}


#include "Arduino.h"
#include "Wire.h"
#include "twi_pins.h"
//#include "twiData_struct.h"


extern "C" {
#include "twi.h"
}

#ifndef DEFAULT_FREQUENCY
  #define DEFAULT_FREQUENCY 100000
#endif

// Initialize Class Variables //////////////////////////////////////////////////

// Constructors ////////////////////////////////////////////////////////////////
      
TwoWire::TwoWire(TWI_t *twi_module) {  
   vars._module = twi_module;
   vars.user_onRequest = NULL;  //Make sure to initialize this pointers
   vars.user_onReceive = NULL;  //This avoids weird jumps should something unexpected happen
}

// Public Methods //////////////////////////////////////////////////////////////
bool TwoWire::pins(uint8_t sda_pin, uint8_t scl_pin) {
   #if defined (TWI1)
      if      (&TWI0 == vars._module)  {return TWI0_Pins(sda_pin, scl_pin);}
      else if (&TWI1 == vars._module)  {return TWI1_Pins(sda_pin, scl_pin);}
      else                             {return false;}
   #else
      return TWI0_Pins(sda_pin, scl_pin); 
   #endif
}


bool TwoWire::swap(uint8_t state) {
   #if defined (TWI1)
      if      (&TWI0 == vars._module) {return TWI0_swap(state);}
      else if (&TWI1 == vars._module) {return TWI1_swap(state);}
      else                            {return false;}
   #else
      return TWI0_swap(state);
   #endif
}


bool TwoWire::swapModule(TWI_t *twi_module) {
   #if defined (TWI1)
      #if defined (USING_TWI1)
        badCall("swapModule() can only be used if TWI1 is not used");
      #else
         if (vars._module->MCTRLA == 0)   //slave and master inits enable MCTRLA, so just check for that
         { 
            vars._module = twi_module;   
            return true;         //Success
         }
      #endif
   #else
        badCall("Only one TWI module available, nothing to switch with");
    (void)twi_module; //Remove warning unused variable
   #endif
   return false;
}


void TwoWire::usePullups(void) {
   #if defined (TWI1)
      if      (&TWI0 == vars._module) {TWI0_usePullups();}
      else if (&TWI1 == vars._module) {TWI1_usePullups();}
   #else
      TWI0_usePullups();
   #endif
}


// *INDENT-ON* The rest is okay to stylecheck
void TwoWire::begin(void) {
   TWI_MasterInit(&vars);
   TWI_MasterSetBaud(&vars, DEFAULT_FREQUENCY);
}


void TwoWire::begin(uint8_t address, bool receive_broadcast, uint8_t second_address) {
  TWI_SlaveInit(&vars, address, receive_broadcast, second_address);
  TWI_RegisterSlaveISRcallback(onSlaveIRQ);                          //give the C part of the programm a pointer to call back to.
}


void TwoWire::setClock(uint32_t clock) {
  TWI_MasterSetBaud(&vars, clock);
}


void TwoWire::end(void) {
  TWI_Disable(&vars);
}
void TwoWire::endMaster(void) {
  TWI_DisableMaster(&vars);
}
void TwoWire::endSlave(void) {
  TWI_DisableSlave(&vars);
}


uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity)                       {return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)1);}
uint8_t TwoWire::requestFrom(uint8_t address, size_t  quantity, bool    sendStop)     {return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);}
uint8_t TwoWire::requestFrom(uint8_t address, size_t  quantity)                       {return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)1);}
uint8_t TwoWire::requestFrom(int     address, int     quantity, int     sendStop)     {return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)sendStop);}
uint8_t TwoWire::requestFrom(int     address, int     quantity)                       {return requestFrom((uint8_t)address, (uint8_t)quantity, (uint8_t)1);}

uint8_t TwoWire::requestFrom(uint8_t address, uint8_t quantity, uint8_t sendStop) {
  if (quantity > BUFFER_LENGTH) {
    quantity = BUFFER_LENGTH;
  }
  
  setSlaveAddress(address);

  return TWI_MasterRead(&vars, quantity, sendStop);
}


void TwoWire::beginTransmission(uint8_t address) {
  // set address of targeted slave
  setSlaveAddress(address);
  vars._txTail = vars._txHead;  //reset transmitBuffer 
}


//
//  Originally, 'endTransmission' was an f(void) function.
//  It has been modified to take one parameter indicating
//  whether or not a STOP should be performed on the bus.
//  Calling endTransmission(false) allows a sketch to
//  perform a repeated start.
//
//  WARNING: Nothing in the library keeps track of whether
//  the bus tenure has been properly ended with a STOP. It
//  is very possible to leave the bus in a hung state if
//  no call to endTransmission(true) is made. Some I2C
//  devices will behave oddly if they do not see a STOP.
//
uint8_t TwoWire::endTransmission(bool sendStop) {
  // transmit (blocking)
  return TWI_MasterWrite(&vars, sendStop);
}



// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(uint8_t data) {
#if defined (TWI_MANDS)
  if (vars._bools._toggleStreamFn == 0x01) {
    uint8_t nextHead = TWI_advancePosition(vars._txHeadS);
  
    if (nextHead == vars._txTailS) return 0;        //Buffer full, stop accepting data

    vars._txBufferS[vars._txHeadS] = data;      //Load data into the buffer
    vars._txHeadS = nextHead;                       //advancing the head

    return 1;
  }
#endif
    
  /* Put byte in txBuffer */
  uint8_t nextHead = TWI_advancePosition(vars._txHead);
  
  if (nextHead == vars._txTail) return 0;          //Buffer full, stop accepting data

  vars._txBuffer[vars._txHead] = data;             //Load data into the buffer
  vars._txHead = nextHead;                         //advancing the head

  return 1;
}


// must be called in:
// slave tx event callback
// or after beginTransmission(address)
size_t TwoWire::write(const uint8_t *data, size_t quantity) {

  for (size_t i = 0; i < quantity; i++) {
    write(*(data + i));
  }

  return quantity;
}



/**
 *@brief      available returns the amount of bytes that are available to read in the master or slave buffer
 *
 *
 *@return     uint8_t
 *@retval     amount of bytes available to read from the master buffer
 */
int TwoWire::available(void) {
  return TWI_Available(&vars);
}



// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::read(void) {
#if defined (TWI_MANDS)
  if (vars._bools._toggleStreamFn == 0x01) {  
    if (vars._rxHeadS == vars._rxTailS) {
      return -1;
    }
    else {
      uint8_t c = vars._rxBufferS[vars._rxTailS];
      vars._rxTailS = TWI_advancePosition(vars._rxTailS);
      return c;
    }
  }
#endif
  if (vars._rxHead == vars._rxTail) { // if the head isn't ahead of the tail, we don't have any characters
    return -1;
  } 
  else {
    uint8_t c = vars._rxBuffer[vars._rxTail];
    vars._rxTail = TWI_advancePosition(vars._rxTail);
    return c;
  }
}


// must be called in:
// slave rx event callback
// or after requestFrom(address, numBytes)
int TwoWire::peek(void) {
#if defined (TWI_MANDS)
  if (vars._bools._toggleStreamFn == 0x01) {  
    if (vars._rxHeadS == vars._rxTailS) {return -1;}
    else                                {return vars._rxBufferS[vars._rxTailS];}
  }
#endif
  if (vars._rxHead == vars._rxTail) {return -1;} 
  else                              {return vars._rxBuffer[vars._rxTail];}
}


// can be used to get out of an error state in TWI module
// e.g. when MDATA register is written before MADDR
void TwoWire::flush(void) {
  vars._rxTail = vars._rxHead;
  vars._txHead = vars._txHead;
  #if defined (TWI_MANDS)
    vars._rxTailS = vars._rxHeadS;
    vars._txHeadS = vars._txHeadS;                               
  #endif
  
  /* Turn off and on TWI module */
  TWI_Flush(&vars);
}

uint8_t TwoWire::getIncomingAddress(void) {
  return vars._incomingAddress;
}


#if defined (TWI_DUALCTRL)
void TwoWire::enableDualMode(bool fmp_enable) {
  vars._module->DUALCTRL = ((fmp_enable << TWI_FMPEN_bp) | TWI_ENABLE_bm);
}
#endif






void TwoWire::onSlaveIRQ(TWI_t *module){                 //This function is static and is, thus, the only one for both
                                                         //Wire interfaces. Here is decoded which interrupt was fired.

#if defined (TWI1)                                       //Two TWIs avaialble
   #if defined (USING_WIRE1)                             //User wants to use Wire and Wire1. Need to check the interface
      if      (module == &TWI0)     
      {
         TWI_HandleSlaveIRQ(&(Wire.vars));
      }
      else if (module == &TWI1)                
      {
         TWI_HandleSlaveIRQ(&(Wire1.vars));
      }  
   #else                                                 //User uses only Wire but can use TWI0 and TWI1
       TWI_HandleSlaveIRQ(&(Wire.vars));                 //Only one possible SlaveIRQ source/Target Class                  
   #endif
#else                                                    //Only TWI0 available, IRQ can only have been issued by that interface
   TWI_HandleSlaveIRQ(&(Wire.vars));                     //No need to check for it
#endif
   (void)module;
}



// sets function called on slave write
void TwoWire::onReceive(void (*function)(int)) {
  vars.user_onReceive = function;
}


// sets function called on slave read
void TwoWire::onRequest(void (*function)(void)) {
  vars.user_onRequest = function;
}


void TwoWire::setSlaveAddress(uint8_t slave_address){
   vars._slaveAddress = slave_address << 1;
}  


#if defined(TWI0)
  TwoWire Wire(&TWI0);
#endif

#if defined (TWI1)
#if defined (USING_WIRE1)
   TwoWire Wire1(&TWI1);
#endif
#endif
