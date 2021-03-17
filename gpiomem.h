/*
   Copyright (C) 2017, Richard e Collins.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

#ifndef __GPIOMEM_H__
#define __GPIOMEM_H__

#include <assert.h>
#include <functional>

namespace gpio{
//////////////////////////////////////////////////////////////////////////
// Memory maps /dev/gpiomem
// Uses BCM pin numbering. See gadgetoid's site: http://pinout.xyz/
// https://www.raspberrypi.org/documentation/hardware/raspberrypi/gpio/README.md
// https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf
class GPIOMem
{
public:
    enum ePinMode
    {
        PINMODE_IN,
        PINMODE_OUT,
    };

    enum ePinPull
    {
        PINPULL_FLOATING = 0,
        PINPULL_DOWN = 1,
        PINPULL_UP = 2,
    };

    enum ePinEdgeDetect
    {
        PINPULL_NONE = 0,
        PINPULL_RISING = 1,
        PINPULL_FALLING = 2,
        PINPULL_BOTH = 3,
    };

    typedef std::function<void(int)> tPinEdgeDetectCB;

    // I don't support the pins 32 -> 53 to keep code simple. They are only for the compute module.
    // On the 40 pin header you have pins 2 -> 27 inclusive to play with.
    // Be carful as if you start playing with a pin that is being used for i2c for examples
    // things may stop working.
    // If pin number is out of the 2 -> 27 range code will assert in debug.

    GPIOMem();
    ~GPIOMem();
    
    bool Open();
    void Close();

    bool GetIsOpen(){return mGPIOMemFile != 0 && mGPIO != NULL;}

    // Return true if ok. ePinPull ignored if set to output mode.
    // Pull mode and edge detect only applicable if in INPUT mode.
    bool SetPinMode(int pPin,ePinMode pMode,ePinPull pPull = PINPULL_FLOATING,ePinEdgeDetect pEdge = PINPULL_NONE);

    // Set and get are inline to help preformance for the speed freeks.
    // Although it is down to the complier options .
    // It should, if you pass a constant, do the shift and AND op in the preprocessor
    // if optimisations are on.

    // Goes high if true or low if false. Will assert in debug if not in output mode.
    void SetPin(int pPin,bool pHigh)
    {
        assert( mGPIO != NULL );
        assert( pPin >= 2 && pPin <= 27 );
        // mGPIO is pointer to 32bit values, so 7 is 0x1C in byte offset.
        // To set use offset 7 and 8 (0x1C and 0x20)
        // To clear use offset 10 and 11 (0x28 and 0x2C)
        // This code don't support pins 32 -> 53. Compute module only.
        if( pHigh )
            mGPIO[7] = (1 << (pPin&31));
        else
            mGPIO[10] = (1 << (pPin&31));
    }

    // Returns false if pin is low (or an error) and true if pin is high. Asserts in debug if pin mode not set.
    bool GetPin(int pPin)
    {
        assert( mGPIO != NULL );
        assert( pPin >= 2 && pPin <= 27 );
        // 1 << (pPin&31) creates a mask.
        return (mGPIO[13] & (1 << (pPin&31))) != 0;
    }

    // Returns true if the pin detect HW has seen a change.
    // Will also clear the event so if no changes have been seen
    // since you last called this it will return false.
    // So act on it when you see it return true.
    // The edge it detects depends on the mode you set with SetPinMode.
    bool GetPinEdgeDetected(int pPin)
    {
        if( (mGPIO[16] & (1 << (pPin&31))) != 0 )
        {// Seen it, so clear it for the next one. As per the docs.
            mGPIO[16] |= 1 << (pPin&31);
            return true;
        }
        return false;
    }

private:
    int mGPIOMemFile;
    uint32_t* mGPIO;
};

//////////////////////////////////////////////////////////////////////////
};//	namespace gpio{

#endif /*__GPIOMEM_H__*/
