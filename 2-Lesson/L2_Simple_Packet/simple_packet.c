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

   LESSON 2: Simple packet
*/

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"	// Establish connections.
#include "net/netstack.h"
#include "lib/random.h"
#include "dev/leds.h"

// Standard C includes:
#include <stdio.h>
#include <stdint.h>


/*---------------------------------------------------------------------------*/
/*  MAIN PROCESS DEFINITION  												 */
/*---------------------------------------------------------------------------*/
PROCESS(simple_packet_process, "Lesson 2: Simple packet");
AUTOSTART_PROCESSES(&simple_packet_process);

/*---------------------------------------------------------------------------*/
/* Callback function for received packet processing. 						 */
/*---------------------------------------------------------------------------*/
static void print_packetbuffer()
{
    // Retrieve the pointer to the payload and the length
    void *payload = packetbuf_dataptr();
    uint16_t len = packetbuf_datalen();

    // Print the content of the payload
    printf("Received packet (print_packetbuffer): ");
	printf("\r\n");
    for (int i = 0; i < len; i++) {
        printf("%c", ((char *)payload)[i]);
    }
    printf("\r\n");
}

static void copy_and_print_packetbuffer()
{
    // Allocate memory for the copy_array of the payload
    char copy_array[packetbuf_datalen()];

    // Copy the payload of the packetbuffer
    memcpy(copy_array, packetbuf_dataptr(), packetbuf_datalen());

    // Print the content of the copied payload
    printf("Received packet (copy_and_print_packetbuffer): %s\n", copy_array);
}


static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {

	leds_on(LEDS_GREEN);

	print_packetbuffer();
    copy_and_print_packetbuffer();

	/* Get packet length */
	uint8_t len = strlen( (char *)packetbuf_dataptr() );
	/* Get packet's RSSI */
	int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
	/* Print address of sending node */
	printf("\r\nGot RX packet (broadcast) from: 0x%x%x, len: %d, RSSI: %d\r\n",from->u8[0], from->u8[1],len,rssi);

	leds_off(LEDS_GREEN);
}

/* Connection information
 */
static struct broadcast_conn broadcastConn;

/**
 * Assign callback functions to the connection
 */
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};

/*** CONNECTION DEFINITION END ***/



/*** MAIN THREAD ***/
PROCESS_THREAD(simple_packet_process, ev, data) {

	static struct etimer et;


	PROCESS_EXITHANDLER(broadcast_close(&broadcastConn));
	PROCESS_BEGIN();

	/*
	 * set your group's channel
	 */
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 15);

	/*
	 * OPen broadcast connection
	 */
	broadcast_open(&broadcastConn,129,&broadcast_callbacks);

	etimer_set(&et, CLOCK_SECOND + 0.1*random_rand()/RANDOM_RAND_MAX); //randomize the sending time a little

	while(1){

		PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

		leds_on(LEDS_RED);

		/*
		 * fill the packet buffer
		 */
		packetbuf_copyfrom("Hello",6);

		/*
		 * send the message
		 */
		broadcast_send(&broadcastConn);

		/*
		 * for debugging
		 */
		printf("Broadcast message sent\r\n");

		/*
		 * reset the timer
		 */
		etimer_reset(&et);

		leds_off(LEDS_RED);
	}

	PROCESS_END();
}

