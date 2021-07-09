/*
  (originally found in) TwoWire.cpp - TWI/I2C library for Wiring & Arduino
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
  Modified 2021 by MX682X for the Wire library rewriting project
*/






#ifndef TWI_PINS_H
#define TWI_PINS_H

#include <Arduino.h>

#include "avr/io.h"




/**
 *@brief              TWI_MasterCalcBaud calculates the baud for the desired frequency
 *
 *                    Moved temporarily to twi_pins.h due to licensing reasons
 *                    Can be moved back into twi.c on merge to the DxCore and license adjustment
 *
 *@param              uint32_t frequency is the desired frequency
 *
 *@return             uint8_t
 *@retval             the desired baud value
 */
 #define TWI_BAUD(freq, t_rise) ((F_CPU / freq) / 2) - (5 + (((F_CPU / 1000000) * t_rise) / 2000))
uint8_t TWI_MasterCalcBaud(uint32_t frequency) {
  uint16_t t_rise;
  int16_t baud;

  // The nonlinearity of the frequency coupled with the processor frequency a general offset has been calculated and tested for different frequency bands
  #if F_CPU > 16000000
  if (frequency <= 100000) {
    t_rise = 1000;
    baud = TWI_BAUD(frequency, t_rise) + 6; // Offset +6
  } else if (frequency <= 400000) {
    t_rise = 300;
    baud = TWI_BAUD(frequency, t_rise) + 1; // Offset +1
  } else if (frequency <= 800000) {
    t_rise = 120;
    baud = TWI_BAUD(frequency, t_rise);
  } else {
    t_rise = 120;
    baud = TWI_BAUD(frequency, t_rise) - 1; // Offset -1
  }
  #else
  if (frequency <= 100000) {
    t_rise = 1000;
    baud = TWI_BAUD(frequency, t_rise) + 8; // Offset +8
  } else if (frequency <= 400000) {
    t_rise = 300;
    baud = TWI_BAUD(frequency, t_rise) + 1; // Offset +1
  } else if (frequency <= 800000) {
    t_rise = 120;
    baud = TWI_BAUD(frequency, t_rise);
  } else {
    t_rise = 120;
    baud = TWI_BAUD(frequency, t_rise) 1; // Offset -1
  }
  #endif
  
  if (baud < 1) {
    baud = 1;
  } else if (baud > 255) {
    baud = 255;
  }
  return (uint8_t)baud;  
}


void TWI0_ClearPins() {
  #ifdef PORTMUX_TWIROUTEA
    if ((PORTMUX.TWIROUTEA & PORTMUX_TWI0_gm) == PORTMUX_TWI0_ALT2_gc) {
      // make sure we don't get errata'ed - make sure their bits in the
      // PORTx.OUT registers are 0.
      PORTC.OUTCLR = 0x0C; // bits 2 and 3 
    } else {
      PORTA.OUTCLR = 0x0C; // bits 2 and 3
    }
  #else // megaTinyCore
    #if defined(PORTMUX_TWI0_bm)
      if ((PORTMUX.CTRLB & PORTMUX_TWI0_bm)) {
        // make sure we don't get errata'ed - make sure their bits in the
        // PORTx.OUT registers are 0.
        PORTA.OUTCLR = 0x06; // if swapped it's on PA1, PA2
      } else {
        PORTB.OUTCLR = 0x03; // else PB0, PB1
      }
    #elif defined(__AVR_ATtinyxy2__)
      PORTA.OUTCLR = 0x06; // 8-pin parts always have it on PA1/2
    #else
      PORTB.OUTCLR = 0x03; // else, zero series, no remapping, it's on PB0, PB1
  #endif
#endif
}


bool TWI0_Pins(uint8_t sda_pin, uint8_t scl_pin) {
  #if defined(PORTMUX_CTRLB) /* tinyAVR 0/1 with TWI mux options */
    #if defined(PIN_WIRE_SDA_PINSWAP_1) && defined(PIN_WIRE_SCL_PINSWAP_1)
      if (__builtin_constant_p(sda_pin) && __builtin_constant_p(scl_pin)) {
        if (!((sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) || (sda_pin == PIN_WIRE_SDA_PINSWAP_1 && scl_pin == PIN_WIRE_SCL_PINSWAP_1)))
          badArg("Pins passed to Wire.pins() known at compile time to be invalid");
        }
        if (sda_pin == PIN_WIRE_SDA_PINSWAP_1 && scl_pin == PIN_WIRE_SCL_PINSWAP_1) {
          // Use pin swap
          PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
          return true;
        } else if (sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) {
          // Use default configuration
          PORTMUX.CTRLB &= ~PORTMUX_TWI0_bm;
          return true;
        } else {
          // Assume default configuration
          PORTMUX.CTRLB &= ~PORTMUX_TWI0_bm;
          return false;
        }
    #else /* tinyAVR 0/1 without TWI mux options */
      if (__builtin_constant_p(sda_pin) && __builtin_constant_p(scl_pin)) {
        /* constant case - error if there's no swap available and the pins they hope to use are known at compile time */
        if (sda_pin != PIN_WIRE_SDA || scl_pin != PIN_WIRE_SCL) {
          badCall("This part does not support alternate Wire pins, if Wire.pins() is called, it must be passed the default pins");
          return false;
        } else {
          return true;
        }
      } else { /* Non-constant case */
        return (sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL);
      }
      #endif
  #elif defined(PORTMUX_TWIROUTEA) && (defined(PIN_WIRE_SDA) || defined(PIN_WIRE_SDA_PINSWAP_2) || defined(PIN_WIRE_SDA_PINSWAP_3) )
    /* Dx-series, megaAVR only, not tinyAVR. If tinyAVR 2's supported alt TWI pins, they would probably have this too */
    if (__builtin_constant_p(sda_pin) && __builtin_constant_p(scl_pin)) {
      #if defined(PIN_WIRE_SDA)
        #if defined(PIN_WIRE_SDA_PINSWAP_2)
          #if defined(PIN_WIRE_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) || (sda_pin == PIN_WIRE_SDA_PINSWAP_2 && scl_pin == PIN_WIRE_SCL_PINSWAP_2) || (sda_pin == PIN_WIRE_SDA_PINSWAP_3 && scl_pin == PIN_WIRE_SCL_PINSWAP_3))) {
          #else
            if (!((sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) || (sda_pin == PIN_WIRE_SDA_PINSWAP_2 && scl_pin == PIN_WIRE_SCL_PINSWAP_2))) {
          #endif
        #else /* No pinswap 2 */
          #if defined(PIN_WIRE_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) || (sda_pin == PIN_WIRE_SDA_PINSWAP_3 && scl_pin == PIN_WIRE_SCL_PINSWAP_3))) {
          #else
            if (!(sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL)) {
          #endif
        #endif
      #else /* No pinswap 0 */
        #if defined(PIN_WIRE_SDA_PINSWAP_2)
          #if defined(PIN_WIRE_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE_SDA_PINSWAP_2 && scl_pin == PIN_WIRE_SCL_PINSWAP_2) || (sda_pin == PIN_WIRE_SDA_PINSWAP_3 && scl_pin == PIN_WIRE_SCL_PINSWAP_3))) {
          #else
            if (!((sda_pin == PIN_WIRE_SDA_PINSWAP_2 && scl_pin == PIN_WIRE_SCL_PINSWAP_2))) {
          #endif
        #else /* No pinswap 2 */
          #if defined(PIN_WIRE_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE_SDA_PINSWAP_3 && scl_pin == PIN_WIRE_SCL_PINSWAP_3))) {
          #else
            #error "Can't happen"
          #endif
        #endif
      #endif
      //badArg("Pins passed to Wire.pins() known at compile time to be invalid");
      return false;
      } /* end of error conditionally generated when pins requested known at compile time and wrong */
    } /* End of test for compile time known SDA and SCL pins requested */
   
    #if defined(PIN_WIRE_SDA_PINSWAP_3)
      if (sda_pin == PIN_WIRE_SDA_PINSWAP_3 && scl_pin == PIN_WIRE_SCL_PINSWAP_3) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | 0x03;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE_SDA_PINSWAP_2)
      if (sda_pin == PIN_WIRE_SDA_PINSWAP_2 && scl_pin == PIN_WIRE_SCL_PINSWAP_2) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | 0x02;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE_SDA)
      if (sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL) {
        // Use default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm);
        return true;
      } else {
        // Assume default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm);
        return false;
      }
#else /* DD with 14 pins has no default pins in the "default" "position! Default to alt=2 */
    {
      PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | 0x02;
      return false;
    }
#endif
#else // No TWI pin options - why call this?
  if (__builtin_constant_p(sda_pin) && __builtin_constant_p(scl_pin)) {
    /* constant case - error if there's no swap and the swap attempt is known at compile time */
    if (sda_pin != PIN_WIRE_SDA || scl_pin != PIN_WIRE_SCL) {
      badCall("This part does not support alternate Wire pins, if Wire.pins() is called, it must be passed the default pins");
      return false;
    } else {
      return true;
    }
  } else { /* Non-constant case */
    return (sda_pin == PIN_WIRE_SDA && scl_pin == PIN_WIRE_SCL);
  }
#endif
        return false;
}




bool TWI0_swap(uint8_t state) {
  #if defined(PORTMUX_CTRLB) /* tinyAVR 0/1-series */
    #if (defined(PIN_WIRE_SDA_PINSWAP_1))
      if (state == 1) {
        // Use pin swap
        PORTMUX.CTRLB |= PORTMUX_TWI0_bm;
        return true;
      } else if (state == 0) {
        // Use default configuration
        PORTMUX.CTRLB &= ~PORTMUX_TWI0_bm;
        return true;
      } else {
        // Assume default configuration
        PORTMUX.CTRLB &= ~PORTMUX_TWI0_bm;
        return false;
      }
    #else //keep compiler happy
      if (__builtin_constant_p(state)) {
        if (state != 0) {
          badCall("This part does not support alternate TWI pins. If Wire.swap() is called at all, it must be passed 0 only.");
          return false;
        } else {
          return true;
        }
      } else {
        return !state;
      }
      #endif
  #elif defined(PORTMUX_TWIROUTEA) /* AVR Dx-series */
    #if defined(PIN_WIRE_SDA_PINSWAP_3) && defined(PIN_WIRE_SCL_PINSWAP_3)
      if (state == 3) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | 0x03;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE_SDA_PINSWAP_2) && defined(PIN_WIRE_SCL_PINSWAP_2)
      if (state == 2) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm) | 0x02;
        return true;
      } else
    #endif
    #if (defined(PIN_WIRE_SDA) && defined(PIN_WIRE_SCL))
      if (state == 0) {
        // Use default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm);
        return true;
      } else {
        // Assume default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm);
        return false;
      }
    #else
    {
        // Assume default configuration
      PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & ~PORTMUX_TWI0_gm | 0x02);
      return false;
    }
    #endif
  #else /* tinyAVR 2-series with neither CTRLB nor TWIROUTEA*/
    if (__builtin_constant_p(state)) {
      if (state != 0) {
        badCall("This part does not support alternate TWI pins. If Wire.swap() is called at all, it must be passed 0 only.");
        return false;
      } else {
        return true;
      }
    } else {
      return !state;
    }
  #endif
  return false;
}



void TWI0_usePullups(){
  // make sure we don't get errata'ed - make sure their bits in the output registers are off!
  #ifdef DXCORE
      // if ((PORTMUX.TWIROUTEA & PORTMUX_TWI0_gm) == 0x02) {
      // below achieves same more efficiently, since only the master/slave pins are supported by Wire.h
      // and those are only ever on PA2/PA3, or PC2/PC3 for PORTMUX.TWIROUTEA & PORTMUX_TWI0_gm == 0x02.
      // but portToPortStruct takes a port number... and PC is 2 while PA is 0. So PORTMUX.TWIROUTEA& 0x02
      // is the number that portToPortStruct would want, directly, to get that all important port struct.
      // Slightly more complicated on DD-series since they added a fourth ooption to the portmux to help
      // with the constrained pinout.
    #ifndef __AVR_DD__
      PORT_t *port = portToPortStruct(PORTMUX.TWIROUTEA & 0x02);
    #else
      uint8_t temp = PORTMUX.TWIROUTEA & PORTMUX_TWI0_gm;
      PORT_t *port = portToPortStruct(temp==2?PC:PA);
      if (temp==3) {
        port->OUTCLR = 0x03; //bits 0 and 1
        port->PIN0CTRL |= PORT_PULLUPEN_bm;
        port->PIN1CTRL |= PORT_PULLUPEN_bm;
      } else {
    #endif
        port->OUTCLR = 0x0C; //bits 2 and 3
        port->PIN2CTRL |= PORT_PULLUPEN_bm;
        port->PIN3CTRL |= PORT_PULLUPEN_bm;
    #ifdef __AVR_DD__
      }
    #endif
  #else // megaTinyCore
    #if defined(PORTMUX_TWI0_bm)
      if ((PORTMUX.CTRLB & PORTMUX_TWI0_bm)) {
        PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
        PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
        PORTA.OUTCLR = 0x06;
      } else {
        PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;
        PORTB.PIN0CTRL |= PORT_PULLUPEN_bm;
        PORTB.OUTCLR = 0x03; //bits 1 and 0.
      }
    #elif defined(__AVR_ATtinyxy2__)
      PORTA.PIN2CTRL |= PORT_PULLUPEN_bm;
      PORTA.PIN1CTRL |= PORT_PULLUPEN_bm;
      PORTA.OUTCLR = 0x06; // bits 2 and 1.
    #else
      PORTB.PIN1CTRL |= PORT_PULLUPEN_bm;
      PORTB.PIN0CTRL |= PORT_PULLUPEN_bm;
      PORTB.OUTCLR = 0x03; //bits 1 and 0.
    #endif
  #endif
}

#if defined (TWI1)
void TWI1_ClearPins() {
  #ifdef PORTMUX_TWIROUTEA
    if ((PORTMUX.TWIROUTEA & PORTMUX_TWI1_gm) == PORTMUX_TWI1_ALT2_gc) {
      // make sure we don't get errata'ed - make sure their bits in the
      // PORTx.OUT registers are 0.
      PORTB.OUTCLR = 0x0C; // bits 2 and 3
    } else {
      PORTF.OUTCLR = 0x0C; // bits 2 and 3
    }
  #endif   //Only Dx-Series has 2 TWI
}


bool TWI1_Pins(uint8_t sda_pin, uint8_t scl_pin) {
  #if defined(PORTMUX_TWIROUTEA) && (defined(PIN_WIRE1_SDA) || defined(PIN_WIRE1_SDA_PINSWAP_2) || defined(PIN_WIRE1_SDA_PINSWAP_3) )
    /* Dx-series, megaAVR only, not tinyAVR. If tinyAVR 2's supported alt TWI pins, they would probably have this too */
    if (__builtin_constant_p(sda_pin) && __builtin_constant_p(scl_pin)) {
      #if defined(PIN_WIRE1_SDA)
        #if defined(PIN_WIRE1_SDA_PINSWAP_2)
          #if defined(PIN_WIRE1_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE1_SDA && scl_pin == PIN_WIRE1_SCL) || (sda_pin == PIN_WIRE1_SDA_PINSWAP_2 && scl_pin == PIN_WIRE1_SCL_PINSWAP_2) || (sda_pin == PIN_WIRE1_SDA_PINSWAP_3 && scl_pin == PIN_WIRE1_SCL_PINSWAP_3))) {
          #else
            if (!((sda_pin == PIN_WIRE1_SDA && scl_pin == PIN_WIRE1_SCL) || (sda_pin == PIN_WIRE1_SDA_PINSWAP_2 && scl_pin == PIN_WIRE1_SCL_PINSWAP_2))) {
          #endif
        #else /* No pinswap 2 */
          #if defined(PIN_WIRE_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE1_SDA && scl_pin == PIN_WIRE1_SCL) || (sda_pin == PIN_WIRE1_SDA_PINSWAP_3 && scl_pin == PIN_WIRE1_SCL_PINSWAP_3))) {
          #else
            if (!(sda_pin == PIN_WIRE1_SDA && scl_pin == PIN_WIRE1_SCL)) {
          #endif
        #endif
      #else /* No pinswap 0 */
        #if defined(PIN_WIRE1_SDA_PINSWAP_2)
          #if defined(PIN_WIRE1_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE1_SDA_PINSWAP_2 && scl_pin == PIN_WIRE1_SCL_PINSWAP_2) || (sda_pin == PIN_WIRE1_SDA_PINSWAP_3 && scl_pin == PIN_WIRE1_SCL_PINSWAP_3))) {
          #else
            if (!((sda_pin == PIN_WIRE1_SDA_PINSWAP_2 && scl_pin == PIN_WIRE1_SCL_PINSWAP_2))) {
          #endif
        #else /* No pinswap 2 */
          #if defined(PIN_WIRE1_SDA_PINSWAP_3)
            if (!((sda_pin == PIN_WIRE1_SDA_PINSWAP_3 && scl_pin == PIN_WIRE1_SCL_PINSWAP_3))) {
          #else
            #error "Can't happen"
          #endif
        #endif
      #endif
      // badArg("Pins passed to Wire.pins() known at compile time to be invalid");
      return false; 
    } /* end of error conditionally generated when pins requested known at compile time and wrong */
  } /* End of test for compile time known SDA and SCL pins requested */
   
    #if defined(PIN_WIRE1_SDA_PINSWAP_3)
      if (sda_pin == PIN_WIRE1_SDA_PINSWAP_3 && scl_pin == PIN_WIRE1_SCL_PINSWAP_3) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm) | PORTMUX_TWI1_ALT3_gc;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE1_SDA_PINSWAP_2)
      if (sda_pin == PIN_WIRE1_SDA_PINSWAP_2 && scl_pin == PIN_WIRE1_SCL_PINSWAP_2) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm) | PORTMUX_TWI1_ALT2_gc;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE1_SDA)
      if (sda_pin == PIN_WIRE1_SDA && scl_pin == PIN_WIRE1_SCL) {
        // Use default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm);
        return true;
      } else {
        // Assume default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm);
        return false;
      }
    #else /* DD with 14 pins has no default pins in the "default" "position! Default to alt=2 */
      {
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm) | PORTMUX_TWI1_ALT2_gc;
        return false;
      }
    #endif
  #endif
  return false;
}


bool TWI1_swap(uint8_t state) {
  #if defined(PORTMUX_TWIROUTEA) /* AVR Dx-series */
    #if defined(PIN_WIRE1_SDA_PINSWAP_3)
      if (state == 3) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm) | PORTMUX_TWI1_ALT3_gc;
        return true;
      } else
    #endif
    #if defined(PIN_WIRE1_SDA_PINSWAP_2) && defined(PIN_WIRE1_SCL_PINSWAP_2)
      if (state == 2) {
        // Use pin swap
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm) | PORTMUX_TWI1_ALT2_gc;
        return true;
      } else
    #endif
    #if (defined(PIN_WIRE1_SDA) && defined(PIN_WIRE1_SCL))
      if (state == 0) {
        // Use default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm);
        return true;
      } else {
        // Assume default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm);
        return false;
      }
    #else
      {
        // Assume default configuration
        PORTMUX.TWIROUTEA = (PORTMUX.TWIROUTEA & !PORTMUX_TWI1_gm | PORTMUX_TWI1_ALT2_gc);
        return false;
      }
    #endif
  #endif
  return false;
}

void TWI1_usePullups() {
  uint8_t temp = PORTMUX.TWIROUTEA & PORTMUX_TWI1_gm;
  PORT_t *port = portToPortStruct(temp==8?PB:PF);
  if (temp==3) {
    port->OUTCLR = 0x03; //bits 0 and 1
    port->PIN0CTRL |= PORT_PULLUPEN_bm;
    port->PIN1CTRL |= PORT_PULLUPEN_bm;
  } else {
    port->OUTCLR = 0x0C; //bits 2 and 3
    port->PIN2CTRL |= PORT_PULLUPEN_bm;
    port->PIN3CTRL |= PORT_PULLUPEN_bm;
  }
}
#endif

#endif /* TWI_DRIVER_H */
