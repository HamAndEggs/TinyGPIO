# gpio
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
along with this program.  If not, see <http://www.gnu.org/licenses/>.

-----------------

This is my first github repo so will get better. 
This class is using /dev/gpiomem device and so can be used from userland without the need for sudo.
Also means that works on all version of the pi without having to change the address that is mapped to memory.

Has standard input and output pin control and also hardware edge detection. Means that a rising or falling edge
can be detected and not lost if the app is doing something else. Still requires polling but is more robust.
Not as good as real interrupts but is much better than just polling the pin state.

See page 100 for a description in the BCM2835 ARM Peripherals doc.
https://www.raspberrypi.org/documentation/hardware/raspberrypi/bcm2835/BCM2835-ARM-Peripherals.pdf

At some point i'll sortout a make file or something simular for compiling into a lib. For now just include the source into your app.

Example use:

	#include <iostream>
	#include <signal.h>
	#include <unistd.h>

	#include <chrono>
	#include <thread>
	#include <linux/i2c-dev.h>

	#include "gpiomem.h"


	bool KeepGoing = true;

	void static CtrlHandler(int SigNum)
	{
		static int numTimesAskedToExit = 0;
		std::cout << std::endl << "Asked to quit, please wait" << std::endl;
		if( numTimesAskedToExit > 2 )
		{
			std::cout << "Asked to quit to many times, forcing exit in bad way" << std::endl;
			exit(1);
		}
		KeepGoing = false;
	}

	int main(int argc, char *argv[])
	{
		signal (SIGINT,CtrlHandler);

		gpio::GPIOMem Pins;

		if( Pins.Open() )
		{
			Pins.SetPinMode(19,gpio::GPIOMem::PINMODE_OUT);
			Pins.SetPinMode(26,gpio::GPIOMem::PINMODE_OUT);
			Pins.SetPinMode(21,gpio::GPIOMem::PINMODE_IN,gpio::GPIOMem::PINPULL_DOWN,gpio::GPIOMem::PINPULL_BOTH);

			while(KeepGoing)
			{
				Pins.SetPin(19,false);
				Pins.SetPin(26,false);
				if( Pins.GetPinEdgeDetected(21) )
				{
					if( Pins.GetPin(21) )
						Pins.SetPin(19,true);
					else
						Pins.SetPin(26,true);
					// Because it is using the hardware the release of the button will not be lost
					// whilst it is sat here waiting.
					std::this_thread::sleep_for(std::chrono::milliseconds(1000));
				}
			}
		}

		// Turn them off.
		Pins.SetPin(19,false);
		Pins.SetPin(26,false);

		Pins.Close();

		return 0;
	}


