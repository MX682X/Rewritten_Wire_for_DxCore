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

#include "Arduino.h"
#include "twi.h"
#include "twi_pins.h"

static void (*TWI_onSlaveISR) (TWI_t *module) __attribute__((unused));


// "Private" function declaration
void NotifyUser_onRequest(struct twiData *_data);	
void NotifyUser_onReceive(struct twiData *_data, uint8_t numBytes);



//When used with a Buffer that has a power of 2, an AND can be performed to save a couple of Bytes
//This might be important on tiny devices
uint8_t TWI_advancePosition(uint8_t pos)
{
	uint8_t nextPos = (pos + 1);
	
	#if defined (BUFFER_NOT_POWER_2) 
		if (nextPos > (BUFFER_LENGTH-1)) nextPos = 0;  //round-robin-ing
	#else
		nextPos &= (BUFFER_LENGTH-1);
	#endif
	
	return nextPos;
}


void TWI_MasterInit(struct twiData *_data)
{
#if defined (TWI_MANDS)						//Check if the user wants to use Master AND Slave
	if (_data->_bools._masterEnabled == 1) {	//Slave is allowed to be enabled, don't re-enable the master though
		return;
	}	
#else									//Master or Slave
	if (_data->_bools._masterEnabled == 1 ||	//If Master was enabled
	    _data->_bools._slaveEnabled == 1) {		//or Slave was enabled
		return;										//return and do nothing
	}
#endif


#if defined (TWI1)
   if      (&TWI0 == _data->_module) {
	   TWI0_ClearPins();
   }
   else if (&TWI1 == _data->_module) {
	   TWI1_ClearPins();
   }
#else 
	TWI0_ClearPins();
#endif	
	_data->_bools._masterEnabled = 1;
	_data->_module->MCTRLA =/* TWI_RIEN_bm | TWI_WIEN_bm |*/ TWI_ENABLE_bm; //Master Interrupts disabled, because we will poll the status bits
	_data->_module->MSTATUS = TWI_BUSSTATE_IDLE_gc;
}


void TWI_SlaveInit(struct twiData *_data, uint8_t address, uint8_t receive_broadcast, uint8_t second_address)
{
#if defined (TWI_MANDS)						//Check if the user wants to use Master AND Slave
	if (_data->_bools._slaveEnabled == 1) {	//Master is allowed to be enabled, don't re-enable the slave though
		return;
	}
#else									//Master or Slave
	if (_data->_bools._masterEnabled == 1 ||	//If Master was enabled
		_data->_bools._slaveEnabled == 1) {		//or Slave was enabled
		return;										//return and do nothing
}
#endif

#if defined (TWI1)
   if      (&TWI0 == _data->_module) {
	   TWI0_ClearPins();
   }
   else if (&TWI1 == _data->_module) {
	   TWI1_ClearPins();
   }
#else 
	TWI0_ClearPins();
#endif
   
   _data->_bools._slaveEnabled = 1;
   _data->_module->SADDR = address << 1 | receive_broadcast;
   _data->_module->SADDRMASK = second_address;
   _data->_module->SCTRLA = TWI_DIEN_bm | TWI_APIEN_bm | TWI_PIEN_bm  | TWI_ENABLE_bm;

   /* Bus Error Detection circuitry needs Master enabled to work */
   _data->_module->MCTRLA = TWI_ENABLE_bm;
}


void TWI_Flush(struct twiData *_data)
{
   _data->_module->MCTRLB |= TWI_FLUSH_bm;
}


void TWI_Disable(struct twiData *_data)
{
   TWI_DisableMaster(_data);
   TWI_DisableSlave(_data);
}
void TWI_DisableMaster(struct twiData *_data)
{
	_data->_module->MCTRLA = 0x00;
	_data->_module->MBAUD = 0x00;
	_data->_bools._masterEnabled = 0x00;
	
}
void TWI_DisableSlave(struct twiData *_data)
{
	_data->_module->SADDR = 0x00;
	_data->_module->SCTRLA = 0x00;
	_data->_module->SADDRMASK = 0x00;
	_data->_bools._slaveEnabled = 0x00;
}




void TWI_MasterSetBaud(struct twiData *_data, uint32_t frequency)
{
   uint8_t newBaud = TWI_MasterCalcBaud(frequency);   //get the new Baud value
   uint8_t oldBaud = _data->_module->MBAUD;            //load the old Baud value
   if (newBaud != oldBaud)                            //compare both, in case the code is issuing this before every transmission. (looking at you, u8g2)
   {
      uint8_t restore = _data->_module->MCTRLA;     //Save the old Master state
      _data->_module->MCTRLA = 0;                   //Disable Master
      _data->_module->MBAUD = newBaud;              //update Baudregister

      if (frequency > 800000){  _data->_module->CTRLA |=  TWI_FMPEN_bm;} //set   FM+
      else                   {  _data->_module->CTRLA &= ~TWI_FMPEN_bm;} //clear FM+

      if (restore & TWI_ENABLE_bm) {               //if the master was previously enabled
         _data->_module->MCTRLA  = restore;                   //restore the old register, thus enabling it again
         _data->_module->MSTATUS = TWI_BUSSTATE_IDLE_gc;      //Force the state machine into Idle according to the data sheet
      }
   }
}


uint8_t TWI_MasterCalcBaud(uint32_t frequency)
{
   uint16_t t_rise;
   int16_t baud;

   // The nonlinearity of the frequency coupled with the processor frequency a general offset has been calculated and tested for different frequency bands
   #if F_CPU > 16000000
   if (frequency <= 100000) {
      t_rise = 1000;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) + 6; // Offset +6
   } else if (frequency <= 400000) {
      t_rise = 300;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) + 1; // Offset +1
   } else if (frequency <= 800000) {
      t_rise = 120;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000));
   } else {
      t_rise = 120;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) - 1; // Offset -1
   }
   #else
   if (frequency <= 100000) {
      t_rise = 1000;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) + 8; // Offset +8
   } else if (frequency <= 400000) {
      t_rise = 300;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) + 1; // Offset +1
   } else if (frequency <= 800000) {
      t_rise = 120;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000));
   } else {
      t_rise = 120;
      baud = (F_CPU / (2 * frequency)) - (5 + (((F_CPU / 1000000) * t_rise) / 2000)) - 1; // Offset -1
   }
   #endif

   if (baud < 1) {
      baud = 1;
   } else if (baud > 255) {
      baud = 255;
   }
   return baud;  
}


uint8_t TWI_Available(struct twiData *_data)
{
   uint16_t i = (BUFFER_LENGTH + _data->_rxHead - _data->_rxTail);
   if (i <  BUFFER_LENGTH) return i;
   else                    return (i - BUFFER_LENGTH);
}


uint8_t TWI_AvailableSlave(struct twiData *_data)
{
	uint16_t i = (BUFFER_LENGTH + _data->_rxHeadS - _data->_rxTailS);
	if (i <  BUFFER_LENGTH) return i;
	else                    return (i - BUFFER_LENGTH);
}



uint8_t TWI_MasterWrite(struct twiData *_data, bool send_stop)              //returns the amount of successfully written bytes or zero if Arb/Buserror
{                        
	if ((_data->_module->MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_UNKNOWN_gc) {
		return 0;																//If the bus was not initialized, return
	}
	
    beginning:																	//Position to restart in case of an Arbitration error                      
    while ((_data->_module->MSTATUS & TWI_BUSSTATE_gm) == TWI_BUSSTATE_BUSY_gc)  {}   //Wait for Bus to be free again
	
    uint8_t dataWritten = 0;
    uint8_t writeAddress = ADD_WRITE_BIT(_data->_slaveAddress);		//Get slave address and clear the read bit
    _data->_module->MADDR = writeAddress;                           //write to the ADDR Register -> (repeated) Start condition is issued and slave address is sent 
   
    while (!(_data->_module->MSTATUS & TWI_WIF_bm)) {}				//Wait for the address/data transfer completion
	if (_data->_module->MSTATUS & TWI_ARBLOST_bm) {					//If another Master has started writing an Address at the same time, go back and wait 	  
			  goto beginning;										//Until the TWI bus has changed from BUSY to IDLE
	}
   
   //if the slave has acknowledged the address, data can be sent
   while (true)                                                   //Do the following until a break
   {      
      uint8_t currentStatus = _data->_module->MSTATUS;            //get a local copy of the Status for easier access
	  
      if (currentStatus & (TWI_ARBLOST_bm | TWI_BUSERR_bm)) {	  //Check for Bus error (state M4 in datasheet)
		  return 0;                                               //else: abort operation
      }
   
      if (currentStatus & TWI_RXACK_bm){                          //Address/Data was not Acknowledged (state M3 in datasheet)
         send_stop = true;                                        //make sure to send a stop bit         
         break;                                                   //break the loop and skip to the end
      }

      if (dataWritten > 0)                                        //This if condition should run every time except in the first iteration
      {
         //_data->_txTail++;                                         //This way it is possible to check if the data sent was acknowledged
         //if (_data->_txTail > (BUFFER_LENGTH-1)) _data->_txTail = 0; //and advance the tail based on that
		 _data->_txTail = TWI_advancePosition(_data->_txTail);
      }

      if (_data->_txHead != _data->_txTail)                        //check if there is data to be written
      { 
         _data->_module->MDATA = _data->_txBuffer[_data->_txTail];    //Writing to the register to send data
         dataWritten++;
      }     
      else                                                        //No data left to be written
      {
         break;                                                   //so break the while loop
      }
	  
	  while (!(_data->_module->MSTATUS & TWI_WIF_bm)) {}          //Wait for the address/data transfer completion
   }
   
   if (send_stop) _data->_module->MCTRLB = TWI_MCMD_STOP_gc;      //Send stop
   //else           _data->_module->MCTRLB = TWI_MCMD_REPSTART_gc;  //or Repeated start   //Repeated start just forces an ADDR write. 
   
   return dataWritten;
}


uint8_t TWI_MasterRead(struct twiData *_data, uint8_t bytesToRead, bool send_stop)
{
   
   uint8_t retVal = 0;
   uint8_t dataRead = 0;
   uint8_t currentStatus = _data->_module->MSTATUS;
   
   if ((currentStatus & TWI_BUSSTATE_gm) == TWI_BUSSTATE_UNKNOWN_gc){   //Bus deinitialized for some reason
      return retVal;													//return 0
   }
   while ((currentStatus & TWI_BUSSTATE_gm) == TWI_BUSSTATE_BUSY_gc) {} //Wait if another master is using the bus

   uint8_t readAddress = ADD_READ_BIT(_data->_slaveAddress);      //Get slave address and set the read bit
   _data->_module->MADDR = readAddress;                           //write to the ADDR Register -> (repeated) Start condition is issued and slave address is sent

   while (!(_data->_module->MSTATUS & TWI_RIF_bm)) {}             //Wait for the address/data receive completion
   
   if (_data->_module->MSTATUS & TWI_RXACK_bm)                    //Address was not Acknowledged (state M3 in datasheet)
   {    
      send_stop = true;                                           //Terminate the transaction, retVal is still '0'
      goto theEnd;                                                //skip the loop and go directly to the end
   }

   while (true)
   {
      uint8_t currentStatus = _data->_module->MSTATUS;            //get a local copy of the Status for easier access
      if (currentStatus & (TWI_ARBLOST_bm | TWI_BUSERR_bm))       //Check for Bus error (state M4 in datasheet)
      {
         return 0;                                                //abort operation
      }

      if (dataRead > (BUFFER_LENGTH-1))                           //Bufferoverflow with this incoming Byte
      {
         send_stop = true;                                        //make sure to end the transaction
         retVal = dataRead;                                       //prepare to return the amount of received bytes
         goto theEnd;                                             //break the loop and skip to the end
      }
      
      uint8_t data = _data->_module->MDATA;                       //Data is fine and we have space, so read out the data register
      _data->_rxBuffer[_data->_rxHead] = data;                    //and save it in the Buffer. 
      
      //_data->_rxHead++;                                           //advance to the next position
      //if (_data->_rxHead > (BUFFER_LENGTH-1)) _data->_rxHead = 0; //round-robin-ing
	  _data->_rxHead = TWI_advancePosition(_data->_rxHead);

      dataRead++;                                                 //Byte is read 
      if (dataRead < bytesToRead)                                 //expexting more bytes, so
      {
         _data->_module->MCTRLB = TWI_MCMD_RECVTRANS_gc;          //send an ACK so the Slave so it can sends the next byte
      }
      else                                                        //Otherwise,
      {
         retVal = dataRead;                                       //prepare to return the amount of received bytes
         break;                                                   //Break the loop, continue with the NACK and, if requested, STOP
      }

      while (!(_data->_module->MSTATUS & TWI_RIF_bm)) {}          //Wait for the address/data receive interrupt flag
   }

   theEnd:
   if (send_stop) _data->_module->MCTRLB = TWI_ACKACT_bm | TWI_MCMD_STOP_gc;      //Send NACK + STOP if requested
   return dataRead;                                               //return the amount of bytes read
}




//Interrupts and setting them up
void TWI_RegisterSlaveISRcallback(void (*function)(TWI_t *module))
{
   if (function != NULL) TWI_onSlaveISR = function;
   else badArg("Null pointer passed");
}


void TWI_SlaveInterruptHandler(TWI_t *module)
{
   if (NULL != TWI_onSlaveISR) TWI_onSlaveISR(module);
}


//Function that actually handles the Slave interrupt
//To hopefully increase readability, the function is written once for master and slave
//and once for master or slave.  The define decides.
void TWI_HandleSlaveIRQ(struct twiData *_data)
{
//#if defined (TWI_MANDS)
   uint8_t slaveStatus = _data->_module->SSTATUS;
   uint8_t payload;           //Declaration in switch case does not work
   uint8_t nextHead;
   uint8_t quantRxData;
   
   if (slaveStatus & (TWI_BUSERR_bm | TWI_COLL_bm)) //if Bus error/Collision was detected
   {
	   payload = _data->_module->SDATA;			//Read data to remove Status flags
	   _data->_rxTailS = _data->_rxHeadS;            //Abort
	   _data->_txTailS = _data->_txHeadS;
   }
   else												//No Bus error/Collision was detected
   {
		if (0 == _data->_bools._ackMatters) {
		   slaveStatus = slaveStatus & ~TWI_RXACK_bm;	//remove RXACK except on master Read
		}
	   
		switch (slaveStatus)
		{
//STOP Interrupt
//But no CLKHOLD
			case 0x40:		//APIF
			case 0x42:		//APIF|DIR						//No CLKHOLD, everything is already finished
			case 0x60:		//APIF|CLKHOLD					//STOP on master write / slave read
				_data->_module->SSTATUS = TWI_APIF_bm;					//Clear Flag, no further Action needed
			
				quantRxData = TWI_AvailableSlave(_data);				//if there is received data,
				NotifyUser_onReceive(_data, quantRxData);				//Notify user program with the amount of received data
				
				_data->_rxTailS = _data->_rxHeadS;						//User should have handled all data, if not, set available rxBytes to 0	   
				break;

		   
//Address Interrupt
			case 0x61:		//APIF|CLKHOLD|AP				//ADR with master write / slave read
				_data->_incomingAddress = _data->_module->SDATA;	//
				_data->_module->SCTRLB = TWI_SCMD_RESPONSE_gc;		//"Execute Acknowledge Action succeeded by reception of next byte"
				break;												//expecting data interrupt next (case 0x80). Fills rxBuffer
		   
			case 0x63:		//APIF|CLKHOLD|DIR|AP			//ADR with master read  / slave write
				_data->_incomingAddress = _data->_module->SDATA;	//saving address to pass to the user function 
				quantRxData = TWI_AvailableSlave(_data);			//There is no way to identify a REPSTART, so when a Master Read occurs after a master write
				NotifyUser_onReceive(_data, quantRxData);			//Notify user program "onReceive" with the amount of received data
		   
				NotifyUser_onRequest(_data);						//Notify user program "onRequest". User Program uses write() to fill buffer
				_data->_module->SCTRLB = TWI_SCMD_RESPONSE_gc;		//"Execute Acknowledge Action succeeded by slave data interrupt"
				break;												//expecting the TWI module to issue a data interrupt with DIR bit set. (case 0x82).


//Data Write Interrupt
			case 0xA0:		//DIF|CLKHOLD
			case 0xA1:		//DIF|CLKHOLD|AP
				payload = _data->_module->SDATA;
				nextHead = TWI_advancePosition(_data->_rxHeadS);
				if (nextHead == _data->_rxTailS) {					//if buffer is full
					_data->_module->SCTRLB = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;   //"Execute Acknowledge Action succeeded by waiting for any Start (S/Sr) condition"
					_data->_rxTailS = _data->_rxHeadS;                                //Dismiss all received Data since data integrity can't be guaranteed
				} else {											//if buffer is not full
					_data->_rxBufferS[_data->_rxHeadS] = payload;			//Load data into the buffer
					_data->_rxHeadS = nextHead;								//Advance Head
					_data->_module->SCTRLB = TWI_SCMD_RESPONSE_gc;			//"Execute Acknowledge Action succeeded by reception of next byte"
				}
				break;

//ACK Received
//Data Read Interrupt
			case 0xA2:		//DIF|CLKHOLD|DIR
			case 0xA3:		//DIF|CLKHOLD|DIR|AP
				_data->_bools._ackMatters = true;			//start checking for NACK
				if (_data->_txHeadS != _data->_txTailS) {		//Data is available
					_data->_module->SDATA = _data->_txBufferS[_data->_txTailS]; //Writing to the register to send data
					_data->_txTailS = TWI_advancePosition(_data->_txTailS);		//Advance tail   
					_data->_module->SCTRLB = TWI_SCMD_RESPONSE_gc;				//"Execute a byte read operation followed by Acknowledge Action"
				} else {                                         //No more data available
					_data->_module->SCTRLB = TWI_SCMD_COMPTRANS_gc;				//"Wait for any Start (S/Sr) condition"
				}
				break;



//NACK Received
//Data Read Interrupt
			//case 0x90:	//DIF|RXACK
			case 0xB2:		//DIF|CLKHOLD|RXACK|DIR			//data NACK on master read  / slave write
			case 0xB3:		//DIF|CLKHOLD|RXACK|DIR|AP
				_data->_bools._ackMatters = false;					//stop checking for NACK
				_data->_module->SCTRLB = TWI_SCMD_COMPTRANS_gc;     //"Wait for any Start (S/Sr) condition"
				_data->_txTailS = _data->_txHeadS;                  //Abort further data writes
				break;

			default:
				//Illegal state
				//Abort operation
				_data->_module->SCTRLB = TWI_ACKACT_bm | TWI_SCMD_COMPTRANS_gc;
				//while (true) {}	//while loop for debugging. Use this to see what SSTATUS is
				break;
		}
	}
}



void NotifyUser_onRequest(struct twiData *_data)
{
	if (_data->user_onRequest != NULL) {
		_data->user_onRequest();
	}
}
 
 
void NotifyUser_onReceive(struct twiData *_data, uint8_t numBytes)
{
	if (numBytes > 0){
		if (_data->user_onReceive != NULL) {
			_data->user_onReceive(numBytes);
		}
	}
 }

ISR(TWI0_TWIS_vect) {
  TWI_SlaveInterruptHandler(&TWI0);
}


#if defined (TWI1)
ISR(TWI1_TWIS_vect) {
  TWI_SlaveInterruptHandler(&TWI1);
}
#endif
