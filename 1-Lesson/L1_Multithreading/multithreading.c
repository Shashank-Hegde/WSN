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

   LESSON 1: Multithreading
*/

// Contiki-specific includes:
#include "contiki.h"
#include "dev/leds.h" // Enables use of LEDs.

// Standard C includes:
#include <stdio.h>    // For printf.


//--------------------- PROCESS CONTROL BLOCK ---------------------
PROCESS(red_process, "Red LED Process");
PROCESS(green_process, "Green LED Process");
PROCESS(blue_process, "Blue LED Process");
AUTOSTART_PROCESSES(&red_process, &green_process, &blue_process);

//------------------------ PROCESS' THREAD ------------------------
static struct etimer red_timer;
static struct etimer green_timer;
static struct etimer blue_timer;
static struct etimer kill_timer;

PROCESS_THREAD(red_process, ev, data)
{
    PROCESS_BEGIN();

    printf("\r\nRed LED process started.\r\n");
    etimer_set(&red_timer, CLOCK_SECOND);
    etimer_set(&kill_timer, 2 * CLOCK_SECOND);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (etimer_expired(&kill_timer))
        {
            printf("\r\nRed LED process terminated.\r\n");
			leds_off(LEDS_RED);
            PROCESS_EXIT();
        }

        if (ev == PROCESS_EVENT_TIMER && data == &red_timer)
        {
            printf("\r\nToggling Red LED.\r\n");
            leds_toggle(LEDS_RED);
            etimer_reset(&red_timer);
        }
    }

    PROCESS_END();
}

PROCESS_THREAD(green_process, ev, data)
{
    PROCESS_BEGIN();

    printf("\r\nGreen LED process started.\r\n");
    etimer_set(&green_timer, CLOCK_SECOND / 2);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == PROCESS_EVENT_TIMER && data == &green_timer)
        {
            printf("\r\nToggling Green LED.\r\n");
            leds_toggle(LEDS_GREEN);
            etimer_reset(&green_timer);
        }
    }

    PROCESS_END();
}

PROCESS_THREAD(blue_process, ev, data)
{
    PROCESS_BEGIN();

    printf("\r\nBlue LED process started.\r\n");
    etimer_set(&blue_timer, CLOCK_SECOND / 4);

    while (1)
    {
        PROCESS_WAIT_EVENT();

        if (ev == PROCESS_EVENT_TIMER && data == &blue_timer)
        {
            printf("\r\nToggling Blue LED.\r\n");
            leds_toggle(LEDS_BLUE);
            etimer_reset(&blue_timer);
        }
    }

    PROCESS_END();
}