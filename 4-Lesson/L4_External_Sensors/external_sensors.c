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

   LESSON 3: External Sensors.
*/

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h"     // Establish connections.
#include "net/netstack.h"      // Wireless-stack definitions
#include "dev/leds.h"          // Use LEDs.
#include "dev/button-sensor.h" // User Button
#include "dev/adc-zoul.h"      // ADC
#include "dev/zoul-sensors.h"  // Sensor functions
#include "dev/sys-ctrl.h"
// Standard C includes:
#include <stdio.h>      // For printf.

// Reading frequency in seconds.
#define TEMP_READ_INTERVAL CLOCK_SECOND*1
#define TEMP_READ_INTERVAL_JOYSTICK CLOCK_SECOND/32

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

static int getLightSensorValue(float m, float b, uint16_t adc_input) {
    // Convert the ADC input voltage to lux using the provided formula
    float sensor_value = adc_input / 4.096;
    float lux = m * sensor_value + b;

    // Limit the maximum lux value to 1000
    int light_value = (int)lux;
    if (light_value > 1000) {
        light_value = 1000;
    }

    return light_value;
}


static char broadcast_packet[128];

//--------------------- PROCESS CONTROL BLOCK ---------------------
PROCESS (ext_sensors_process, "External Sensors process");
PROCESS (joystick_data_process, "Joystick Data process");
AUTOSTART_PROCESSES (&joystick_data_process);

//------------------------ PROCESS' THREAD ------------------------
PROCESS_THREAD (ext_sensors_process, ev, data) {

	/* variables to be used */
	static struct etimer temp_reading_timer;
	static uint16_t adc1_value, adc3_value;
	static int adc1_luminosity, adc3_luminosity;

	PROCESS_BEGIN ();


	printf("\r\nZolertia RE-Mote external sensors");
	printf("\r\n====================================");

	/*
	 * set your group's channel
	 */
	NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL,15);

	/*
	 * open the connection
	 */
	broadcast_open(&broadcastConn,129,&broadcast_callbacks);


	/* Configure the ADC ports */
	adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);


	etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);

	while (1) {

		PROCESS_WAIT_EVENT();  // let process continue

		/* If timer expired, pront sensor readings */
	    if(ev == PROCESS_EVENT_TIMER) {

	    	leds_on(LEDS_PURPLE);

	    	/*
	    	 * Read ADC values. Data is in the 12 MSBs
	    	 */
	    	adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4;
	    	adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;

	    	/*
	    	 * Print Raw values
	    	 */

	    	printf("\r\nADC1 value [Raw] = %d", adc1_value);
	        printf("\r\nADC3 value [Raw] = %d", adc3_value);

			float adc1_voltage = adc1_value * 3.3 * 0.2;
			int adc1_luminosity = getLightSensorValue(1.7261, 37.579, adc1_voltage); // Apply correction factor
			printf("\r\nSensor ADC1 luminosity [lux] = %d", adc1_luminosity);

			int adc3_luminosity = getLightSensorValue(1.7261, 37.579, adc3_value);
			printf("\r\nSensor ADC3 luminosity [lux] = %d", adc3_luminosity);


    		leds_off(LEDS_PURPLE);

    		etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL);
	    }
    }

	PROCESS_END ();
}


PROCESS_THREAD(joystick_data_process, ev, data) {
    /* variables to be used */
    static struct etimer temp_reading_timer;
    static uint16_t adc1_value, adc3_value;
    static uint8_t joystick_value;

    PROCESS_BEGIN();

    printf("\r\nZolertia RE-Mote Joystick Data");
    printf("\r\n====================================");

    /*
     * set your group's channel
     */
    NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 15);

    /*
     * open the connection
     */
    broadcast_open(&broadcastConn, 129, &broadcast_callbacks);

    /* Configure the ADC ports */
    adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);

    etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL_JOYSTICK);

    while (1) {
        PROCESS_WAIT_EVENT(); // let process continue

        /* If timer expired, print sensor readings */
        if (ev == PROCESS_EVENT_TIMER) {
            leds_on(LEDS_PURPLE);

            /*
             * Read ADC values. Data is in the 12 MSBs
             */
            adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4;
            adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;

            /*
             * Print Raw values
             */
            printf("adc1_value_raw: %d\n\r", adc1_value); // left (0) vs right (2047)
            printf("adc3_value_raw: %d\n\r", adc3_value); // down (0) vs up (1760)

            const char* packet_data = "";
            if (adc1_value < 50) {
                packet_data = "LEFT";
            } else if (adc3_value < 50) {
                packet_data = "DOWN";
            } else if (adc1_value > 2000) {
                packet_data = "RIGHT";
            } else if (adc3_value > 1700) {
                packet_data = "UP";
            } else {
                packet_data = "CENTER";
            }

            packetbuf_copyfrom(packet_data, strlen(packet_data));
            packetbuf_copyto(broadcast_packet);
            broadcast_send(&broadcastConn);
            printf("\r\nPacket Sent: %s\n\n", broadcast_packet);

            leds_off(LEDS_PURPLE);

            etimer_set(&temp_reading_timer, TEMP_READ_INTERVAL_JOYSTICK);
        }
    }

    PROCESS_END();
}

