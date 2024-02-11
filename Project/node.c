/**
 * @file node.c
 * @brief Wireless nodes code for player devices in the WSN Lab project.
 *
 * This file contains the implementation of wireless nodes used to track the health and performance of athletes
 * in real-time. The nodes communicate with each other and a central gateway through wireless sensor networks.
 *
 * @author Group 5
 * @date June 30, 2023
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

/**
 * @defgroup node SensorNode
 * @brief Implementation of sensor node functionality.
 *
 * This file contains the implementation of sensor node functionality. It includes
 * data structures, functions, and processes related to sensor node operations.
 */

/**@{*/

// MAC LAYER PARAMETERS
//#define NETSTACK_CONF_MAC nullmac_driver

/**
 * @def NETSTACK_CONF_RDC
 * @brief Definition for NullRDC driver.
 */

/**
 * @def MAX_RETRIES
 * @brief Maximum number of retries for a process.
 */

/**
 * @def MAX_NODES
 * @brief Maximum number of nodes in the network.
 */
#define NETSTACK_CONF_RDC nullrdc_driver
#define MAX_RETRIES 3
#define MAX_NODES 5

static uint16_t adc1_value, adc3_value, batteryvolt;
static int16_t max_rssi = -100;
static linkaddr_t best_rssi_switch;

PROCESS(example_unicast_process, "Runicast Example");
AUTOSTART_PROCESSES(&example_unicast_process);
int32_t node_number = 1;

/**
 * @struct sensor_message
 * @brief Represents a message from a sensor device.
 */
struct sensor_message {
  int16_t force;          /**< The force measurement value. */
  int16_t oximeter;       /**< The oximeter measurement value. */
  int32_t path;           /**< The path measurement value. */
  uint16_t batteryLevel;  /**< The battery level of the sensor device. */
};


/**
 * @struct routing_entry
 * @brief Holds an entry in the routing table.
 *
 * This structure represents a single entry in the routing table,
 * holding various details about a node in the network.
 */
struct routing_entry {
  linkaddr_t node_address; /**< Address of the node. */
  uint8_t hops; /**< Number of hops to reach the node. */
  linkaddr_t next_hop; /**< Address of the next hop to reach the node. */
  int8_t node_id; /**< Unique identifier of the node. */
  char node_type; /**< Type of the node (e.g., 'G' for Gateway, 'N' for normal node). */
  uint8_t still_active; /**< Flag to indicate if the node is still active in the network. */
};


/**
 * @brief Callback function for receiving unicast messages.
 *
 * This function is called when a unicast message is received by the node. 
 *
 * @param c The unicast connection.
 * @param from The address of the sender.
 * @param seqno The sequence number of the received message.
 */
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  printf("Received unicast message from %d.%d: '%s'\n",
         from->u8[0], from->u8[1], (char *)packetbuf_dataptr());
}

/**
 * @brief Callback function for receiving broadcast messages.
 *
 * This function is called when a broadcast message is received by the node. The node checks for the best rssi from the broadcast 
 * messages it gets from the switches and choses one with the least rssi. For demonstation the node doesn't directly send to the 
 * gateway even if it is within range due to room size.
 *
 * @param c The broadcast connection.
 * @param from The address of the sender.
 */
static void broadcast_recv(struct broadcast_conn *c, const linkaddr_t *from) {
  int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
 struct routing_entry received_table[MAX_NODES];
    packetbuf_copyto(&received_table);
	printf("Node type is : %c", received_table[0].node_type);
    if(received_table[0].node_type =='G')
    {
      return;
    }
    
  if (rssi > max_rssi) {
    max_rssi = rssi;
    linkaddr_copy(&best_rssi_switch, from);
    printf("best rssi from  %02x:%02x\n", best_rssi_switch.u8[0], best_rssi_switch.u8[1]);
  }

  leds_on(LEDS_GREEN);

  uint8_t len = strlen((char *)packetbuf_dataptr());
  printf("Got RX packet (broadcast) from: 0x%x%x, len: %d, RSSI: %d\r\n", from->u8[0], from->u8[1], len, rssi);
  printf("Received string: ");
  copy_and_print_packetbuffer();

  leds_off(LEDS_GREEN);
}

/**
 * @brief Unicast callbacks structure.
 */
static const struct unicast_callbacks unicast_callbacks = {recv_unicast};

/**
 * @brief Unicast connection structure.
 */
static struct unicast_conn unicast;

/**
 * @brief Broadcast connection structure.
 */
static struct broadcast_conn broadcastConn;

/**
 * @brief Broadcast callbacks structure.
 */
static const struct broadcast_callbacks broadcast_callbacks = {broadcast_recv};

/**
 * @brief Main process thread for the example unicast process.
 *
 * This process handles the periodic transmission of sensor data using unicast communication.
 *
 * @param ev The event being processed.
 * @param data Additional data for the event.
 */
PROCESS_THREAD(example_unicast_process, ev, data)
{
  PROCESS_EXITHANDLER(unicast_close(&unicast);)

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  /*
   * set the group's channel to 15
   */
  NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 15);
  // Set the max transmission power
  NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, 7);

  /*
   * Open broadcast connection
   */
  broadcast_open(&broadcastConn, 129, &broadcast_callbacks);

  // Configure the ADC ports 
  adc_zoul.configure(SENSORS_HW_INIT, ZOUL_SENSORS_ADC1 | ZOUL_SENSORS_ADC3);

  static struct etimer reset_timer;
 etimer_set(&reset_timer, CLOCK_SECOND * 10); // Reset every 10 seconds
  while (1)
  {
    static struct etimer et;
		/* The random interval from 0 to 128 clock ticks (0 to 1 second) */
	uint8_t randomInterval = random_rand() & 0x8F;

    etimer_set(&et, CLOCK_SECOND * 2 + randomInterval);

    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // Check if it's time to reset the best_rssi_switch
   
    if (etimer_expired(&reset_timer))
    {
      linkaddr_copy(&best_rssi_switch, NULL);
      max_rssi = -100;
      etimer_reset(&reset_timer);
      printf("reset timer and best_rssi_switch to NULL");
    }

    // Read ADC values. Data is in the 12 MSBs
    adc1_value = adc_zoul.value(ZOUL_SENSORS_ADC1) >> 4;
    adc3_value = adc_zoul.value(ZOUL_SENSORS_ADC3) >> 4;
	batteryvolt = vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);

    // Print Raw values
    printf("force_raw: %d\n\r", adc1_value); // force sensor
    printf("heartrate_value_raw: %d\n\r", adc3_value); // heart-rate sensor
	printf("battery voltage is : %d", batteryvolt);

    struct sensor_message message;
    message.force = adc1_value;
    message.oximeter = adc3_value;
    message.path = node_number;
	message.batteryLevel = batteryvolt;

    packetbuf_copyfrom(&message, sizeof(struct sensor_message));

    printf("best rssi sent %02x:%02x\n", best_rssi_switch.u8[0], best_rssi_switch.u8[1]);
    unicast_send(&unicast, &best_rssi_switch);
  }

  PROCESS_END();
}

/**
 * @brief Prints the content of the packet buffer as a string.
 */
void print_packetbuffer()
{
  char *payload_ptr = packetbuf_dataptr();
  uint8_t payload_len = strlen(payload_ptr);
  for (int i = 0; i < payload_len; ++i)
  {
    printf("%c", payload_ptr[i]);
  }
  printf("\n");
}

/**
 * @brief Copies and prints the content of the packet buffer.
 */
void copy_and_print_packetbuffer()
{
  // copy the payload of the packetbuffer to a given memory location
  char *my_buffer = (char *)malloc(packetbuf_datalen());
  uint16_t my_buffer_len = packetbuf_datalen();
  packetbuf_copyto(my_buffer);
  // print the content of the memory location
  for (int i = 0; i < my_buffer_len; ++i)
  {
    printf("%c", my_buffer[i]);
  }
  printf("\n");
  free(my_buffer);
}
/**
 * @brief Retrieves the current battery level.
 *
 * This function returns the current value of the battery level sensor after converting it into an appropriate format.
 *
 * @return uint16_t The converted sensor value, representing the current battery level.
 */
uint16_t battery_level_get(void)
{
    return vdd3_sensor.value(CC2538_SENSORS_VALUE_TYPE_CONVERTED);
}


/** @addtogroup node
 * @{
 */
