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

#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include <iostream>
#include <chrono>
#include <thread>

#include "gpiomem.h"

namespace gpio{
//////////////////////////////////////////////////////////////////////////
// Used wiringpi code as reference for the setup phase.
// http://wiringpi.com
// 
#define	BLOCK_SIZE		(4*1024)
const off_t GPIO_BASE = (off_t)0x00200000;


GPIOMem::GPIOMem():
    mGPIOMemFile(0),
    mGPIO(NULL)
{

}

GPIOMem::~GPIOMem()
{
    Close();
}

bool GPIOMem::Open()
{
	mGPIOMemFile = open("/dev/gpiomem", O_RDWR | O_SYNC | O_CLOEXEC);
	if( mGPIOMemFile > 0 )
	{
		mGPIO = (uint32_t *)mmap(0, BLOCK_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, mGPIOMemFile, GPIO_BASE);
		if( (void*)mGPIO != MAP_FAILED )
		{
#ifndef NDEBUG
			std::cout << "/dev/gpiomem open Address: " << std::hex << mGPIO << std::dec << std::endl;
#endif			
		}
		else
		{
			Close();
		}
	}
    return mGPIOMemFile != 0;
}

void GPIOMem::Close()
{
    if( mGPIO != NULL )
    {
#ifndef NDEBUG
		std::cout << "Un mapping /dev/gpiomem" << std::endl;
#endif
        munmap(mGPIO,BLOCK_SIZE);
        mGPIO = NULL;
    }

	if( mGPIOMemFile > 0 )
	{
#ifndef NDEBUG
		std::cout << "Closing /dev/gpiomem" << std::endl;
#endif
		close(mGPIOMemFile);
		mGPIOMemFile = 0;
	}
}

bool GPIOMem::SetPinMode(int pPin,GPIOMem::ePinMode pMode,GPIOMem::ePinPull pPull,ePinEdgeDetect pEdge)
{// Don't need speed here so going for readablity instead. Still going to be very fast so stop looking at the if's. ;)
	assert( mGPIO != NULL );
	assert( pPin >= 2 && pPin <= 27 );
	assert( pMode == PINMODE_IN || pMode == PINMODE_OUT );
	assert( pPull == PINPULL_FLOATING || pPull == PINPULL_DOWN || pPull == PINPULL_UP );
	assert( pEdge == PINPULL_NONE || pEdge == PINPULL_RISING || pEdge == PINPULL_FALLING || pEdge == PINPULL_BOTH );

    if( mGPIO != NULL &&
		pPin >= 2 && pPin <= 27 &&
		(pMode == PINMODE_IN || pMode == PINMODE_OUT) &&
		(pPull == PINPULL_FLOATING || pPull == PINPULL_DOWN || pPull == PINPULL_UP) &&
		(pEdge == PINPULL_NONE || pEdge == PINPULL_RISING || pEdge == PINPULL_FALLING || pEdge == PINPULL_BOTH) )
    {
		int offset = pPin / 10;
		int shift  = (pPin % 10)*3;

		// Remeber, if called on pins that are being used for spi or i2c things will stop working.
		if( pMode == PINMODE_OUT )
		{
			mGPIO[offset] &= ~(7<<shift);	// Clear all bits for this pin. Removing old config.
			mGPIO[offset] |= (1<<shift);	// Set bit 0. Out mode.
#ifndef NDEBUG
			std::cout << "pin: " << pPin
					<< " Offset: " << offset
					<< " Shift: " << shift
					<< " Mode: OUTPUT"
					<< " Pull: IGNORED"
					<< " Edge: IGNORED"
					<< std::endl;
#endif
		}
		else
		{
			mGPIO[offset] &= ~(7<<shift);	// Clear all bits. In mode.
			// Now set the pull up/down changes.
			// The GPIO Pull-up/down Clock Registers control the actuation of internal pull-downs on
			// the respective GPIO pins. These registers must be used in conjunction with the GPPUD
			// register to effect GPIO Pull-up/down changes. The following sequence of events is
			// required:
			// 1. Write to GPPUD to set the required control signal (i.e. Pull-up or Pull-Down or neither
			//    to remove the current Pull-up/down)
			// 2. Wait 150 cycles – this provides the required set-up time for the control signal
			// 3. Write to GPPUDCLK0/1 to clock the control signal into the GPIO pads you wish to
			//    modify – NOTE only the pads which receive a clock will be modified, all others will
			//    retain their previous state.
			// 4. Wait 150 cycles – this provides the required hold time for the control signal
			// 5. Write to GPPUD to remove the control signal
			// 6. Write to GPPUDCLK0/1 to remove the clock 
			mGPIO[37] = (pPull&3);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
			mGPIO[38] = (1<<pPin);
			std::this_thread::sleep_for(std::chrono::milliseconds(1));

			mGPIO[37] = 0;
			mGPIO[38] = 0;

			// Do the rising and falling edge setup.
			if( pEdge&PINPULL_RISING )
				mGPIO[19] |= 1<<pPin;
			else
				mGPIO[19] &= ~(1<<pPin);

			if( pEdge&PINPULL_FALLING )
				mGPIO[22] |= 1<<pPin;
			else
				mGPIO[22] &= ~(1<<pPin);

#ifndef NDEBUG
			std::cout << "pin: " << pPin
					<< " Offset: " << offset
					<< " Shift: " << shift
					<< " Mode: INPUT"
					<< " Pull: " << (pPull==PINPULL_FLOATING?"FLOATING":pPull==PINPULL_DOWN?"DOWN":"UP")
					<< " Edge: " << (pEdge==PINPULL_NONE?"NONE":pEdge==PINPULL_RISING?"RISING":pEdge==PINPULL_FALLING?"FALLING":"BOTH")
					<< std::endl;
#endif
		}
		
        return true;
    }

    return false;
}

//////////////////////////////////////////////////////////////////////////
};//	namespace gpio{
