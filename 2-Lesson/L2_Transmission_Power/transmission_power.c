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

   LESSON 2: Transmission Power
*/

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"	// Establish connections.
#include "lib/random.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include "dev/cc2538-rf.h"

// Standard C includes:
#include <stdio.h>
#include <stdint.h>


/*** DEFINE STATIC VARIABLES ***/

static radio_value_t tx_powers[14] = {7, 5, 3, 1, 0 -1, -3, -5, -7, -9, -11, -13, -15, -24};
static int tx_powers_index = 0;
static int tx_power;


/*** CONNECTION DEFINITION***/

/**
 * Callback function for received packet processing.
 *
 */
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {

	leds_on(LEDS_GREEN);

	uint8_t len = strlen( (char *)packetbuf_dataptr() );
	int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);

	printf("Got RX packet (broadcast) from: 0x%x%x, len: %d, RSSI: %d\r\n",from->u8[0], from->u8[1],len,rssi);

	printf("Received TX Power: ");
	for (int i = 0; i < len; i++) {
        printf("%c", ((char *)packetbuf_dataptr())[i]);
    }
    printf("\r\n");

	
	leds_off(LEDS_GREEN);
}

/**
 * Connection information
 */
static struct broadcast_conn broadcastConn;

/**
 * Assign callback functions to the connection
 */
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};

/*** CONNECTION DEFINITION END ***/


/*** MAIN PROCESS DEFINITION ***/
PROCESS(transmission_power_process, "Lesson 2: Transmission Power");
PROCESS(button_process, "Button Process");
AUTOSTART_PROCESSES(&transmission_power_process, &button_process);


/*** MAIN THREAD ***/
PROCESS_THREAD(transmission_power_process, ev, data) {

	static struct etimer et;

	PROCESS_EXITHANDLER(broadcast_close(&broadcastConn));
	PROCESS_BEGIN();

	/*
	 * set your group's channel
	 */
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 15);

	/*
	 * Change the transmission power here
	 */
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER,7);

	/*
	 * open broadcast connection
	 */
	broadcast_open(&broadcastConn,129,&broadcast_callbacks);

	etimer_set(&et, CLOCK_SECOND + 0.1*random_rand()/RANDOM_RAND_MAX); //randomize the sending time a little

	while(1){

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		leds_on(LEDS_RED);


		/*
		 * for debugging
		 */
		NETSTACK_CONF_RADIO.get_value(RADIO_PARAM_TXPOWER, &tx_power);
		printf("Broadcast message sent with power: %d\r\n",tx_power); // or the configured Power

		char buffer[4];

		// Convert the integer to a string and store it in the buffer
		snprintf(buffer, sizeof(buffer), "%d", tx_power);
		/*
		 * fill the packet buffer
		 */
		packetbuf_copyfrom(&buffer,4);

		/*
		 * send the message
		 */
		broadcast_send(&broadcastConn);

		/*
		 * reset the timer
		 */
		etimer_reset(&et);

		leds_off(LEDS_RED);
	}

	PROCESS_END();
}


PROCESS_THREAD(button_process, ev, data)
{
	PROCESS_BEGIN();

	button_sensor.configure(BUTTON_SENSOR_CONFIG_TYPE_INTERVAL, 3*CLOCK_SECOND);

	leds_off(LEDS_ALL);

	while (1)
	{
		PROCESS_WAIT_EVENT();

		if (ev == button_press_duration_exceeded)
		{

			// Set the new Tx Power value
			printf("\r\nSetting Tx Power: %d dBm\r\n", tx_powers[tx_powers_index]);
			NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, tx_powers[tx_powers_index]);

			tx_powers_index = (tx_powers_index+1) % 14;
			
		}
	}

	PROCESS_END();
}