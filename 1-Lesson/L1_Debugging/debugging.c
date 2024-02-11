/*
   Wireless Sensor Networks Laboratory

   Technische Universität München
   Lehrstuhl für Kommunikationsnetze
   http://www.lkn.ei.tum.de

   copyright (c) 2017 Chair of Communication Networks, TUM

   contributors:
   * Thomas Szyrkowiec
   * Mikhail Vilgelm
   * Octavio Rodríguez Cervantes
   * Angel Corona

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, version 2.0 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

   LESSON 1: Debugging with Serial Port Terminal
*/

// Contiki-specific includes:
#include "contiki.h"
#include "dev/uart.h"
#include "dev/leds.h" // Use LEDs.
#include "dev/serial-line.h"

// Standard C includes:
#include <stdio.h> // For printf.

//--------------------- PROCESS CONTROL BLOCK ---------------------
PROCESS(debug_process, "Lesson 1: Debugging");
AUTOSTART_PROCESSES(&debug_process);

//------------------------ PROCESS' THREAD ------------------------
PROCESS_THREAD(debug_process, ev, data)
{
    PROCESS_BEGIN();

    printf("\nPlatform Zolertia RE-Mote\r\n");
    printf("\nHello World!\r\n");

    // Main process loop:
    for (int i = 5; i <= 3056; i++)
    {
        printf("%d\r\n", i);
        clock_delay_usec(1000); 
    }

    PROCESS_END();
}
