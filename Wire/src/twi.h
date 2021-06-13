/*
  Copyright (c) 2021 MX682X

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/


#ifndef TWI_H
#define TWI_H

#include "Arduino.h"



/*! For adding R/_W bit to address */
#ifndef ADD_READ_BIT
  #define ADD_READ_BIT(address)    (address | 0x01)
#endif
#ifndef ADD_WRITE_BIT
  #define ADD_WRITE_BIT(address)   (address & ~0x01)
#endif


//#define USING_WIRE1		//On devices with two TWIs, this identifies if the user wants to use Wire1
//#define TWI_DUALCTRL		//This identifies if the device supports dual mode, where slave pins are different from the master pins
//#define TWI_MANDS			//This enables the simultaneous use of the Master and Slave functionality - where supported

#ifndef BUFFER_LENGTH
  #if (RAMSIZE < 256)			/* Parts with 128b of RAM wince at pair of 16k buffers         */
    #define BUFFER_LENGTH 16	/* 2k tinyAVRs: 128b -  25% of available RAM                   */
  #elif (RAMSIZE < 512)			/* Parts with 256b of RAM shall allocate 24b buffers           */
    #define BUFFER_LENGTH 32	/* 4k tinyAVRs 0/1: 256b - 25% of available RAM - of           */
  #elif (RAMSIZE < 2048)		/* parts with 512b or 1024b of RAM get 32b buffers             */
    #define BUFFER_LENGTH 32	/* 8k tinyAVRs, 16k 0-series - 6-13% of available RAM          */
  #elif (RAMSIZE < 4096)		/* 16k AVR DD-series, 16k tinyAVR 1/2-series 48b buffers       */
    #define BUFFER_LENGTH 32	/* and 32k tinyAVR   - 3-5% of available RAM                   */
  #else							/* >=4k: Dx32/m320x (4k) m480x (6k),  Dx64 (8k) Dx128 (16k)    */
    #define BUFFER_LENGTH 130	/* 130 - 128b on all Dx with >= 4k RAM, to match official      */
	#define BUFFER_NOT_POWER_2	
  #endif						/* 4809 core plus that couple bytes mentioned above.           */
#endif

struct twiDataBools {					//using a struct so the compiler can use skip if bit is set/cleared
	uint8_t reserved:5;					//reserved for Future use
	bool _masterEnabled:1;
	bool _slaveEnabled:1;
	bool _ackMatters:1;					
	};

struct twiData {                        /*My original idea was to pass the whole TwoWire class as a  */
                                       /*Pointer to this functions but this didn't work of course.  */
   TWI_t *_module;                     /*But I had the idea: since the class is basically just a    */
									   /*struct, why not just put the relevant variables in one     */
   uint8_t _slaveAddress;              /*and pass that as a struct? So now this exists and it seems */
   uint8_t _txHead;                    /*to work.                                                   */
   uint8_t _txTail;                    
   uint8_t _rxHead;
   uint8_t _rxTail;
   
 #if defined (TWI_MANDS)
   uint8_t _incomingAddress;				
   uint8_t _txHeadS;                    
   uint8_t _txTailS;                    
   uint8_t _rxHeadS;
   uint8_t _rxTailS;
 #else										//Searching for a better way to do an alias for the variables
   #define _incomingAddress _slaveAddress
   #define _txHeadS			_txHead
   #define _txTailS			_txTail
   #define _rxHeadS			_rxHead
   #define _rxTailS			_rxTail
 #endif
   
   struct twiDataBools _bools;			//a structure to hold some bools
   
   void (*user_onRequest)(void);			
   void (*user_onReceive)(int);		
   
   uint8_t _txBuffer[BUFFER_LENGTH];
   uint8_t _rxBuffer[BUFFER_LENGTH];
   
 #if defined (TWI_MANDS)				//Putting the arrays in the end because the first 32 bytes can
   uint8_t _txBufferS[BUFFER_LENGTH];	//be accessed easier and faster
   uint8_t _rxBufferS[BUFFER_LENGTH];
 #else
   #define _txBufferS _txBuffer
   #define _rxBufferS _rxBuffer
 #endif
   };

uint8_t  TWI_advancePosition(uint8_t pos);	//returns the next Position with Round-Robin functionality

void     TWI_MasterInit(struct twiData *_data);
void     TWI_SlaveInit(struct twiData *_data, uint8_t address, uint8_t receive_broadcast, uint8_t second_address);
void     TWI_Flush(struct twiData *_data);
void     TWI_Disable(struct twiData *_data);
void     TWI_DisableMaster(struct twiData *_data);
void     TWI_DisableSlave(struct twiData *_data);

void     TWI_MasterSetBaud(struct twiData *_data, uint32_t frequency);
uint8_t  TWI_MasterCalcBaud(uint32_t frequency);

uint8_t  TWI_Available(struct twiData *_data);
uint8_t  TWI_AvailableSlave(struct twiData *_data);

uint8_t  TWI_MasterWrite(struct twiData *_data, bool send_stop);
uint8_t  TWI_MasterRead(struct twiData *_data, uint8_t bytesToRead, bool send_stop);

void     TWI_RegisterSlaveISRcallback(void (*function)(TWI_t *module));


void     TWI_SlaveInterruptHandler(TWI_t *module);
void     TWI_HandleSlaveIRQ(struct twiData *_data);


#endif