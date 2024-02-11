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

   LESSON 1: Timers and Threads
*/

// Contiki-specific includes:
#include "contiki.h"
#include "dev/leds.h" // Enables use of LEDs
#include "sys/ctimer.h"

// Standard C includes:
#include <stdio.h> // For printf

PROCESS(timers_and_threads_process, "Lesson 1: Timers and Threads");
AUTOSTART_PROCESSES(&timers_and_threads_process);

// Declare the callback timer
static struct ctimer freq_timer;

// Callback function for the timer
static void timer_callback(void *data)
{
    // Toggle the state of the red LED
    leds_toggle(LEDS_RED);

    // Reset the callback timer for the next 1 second
    ctimer_reset(&freq_timer);
}

//------------------------ PROCESS' THREAD ------------------------

// Main process:
PROCESS_THREAD(timers_and_threads_process, ev, data)
{

    PROCESS_EXITHANDLER(printf("main_process terminated!\n");)

    PROCESS_BEGIN();

    // Set the callback timer to trigger every 1 second
    ctimer_set(&freq_timer, CLOCK_SECOND, timer_callback, NULL);

    while (1)
    {
        PROCESS_WAIT_EVENT();
    }
    PROCESS_END();
}
