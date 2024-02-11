/**
 * @file switch_gateway.c
 * @brief Implementation of switch gateway functionality.
 */

// Contiki-specific includes:
#include "contiki.h"
#include "net/rime/rime.h" // Establish connections.
#include "net/netstack.h"  // Wireless-stack definitions
#include "dev/leds.h"	   // Use LEDs.
#include "core/net/linkaddr.h"
// Standard C includes:
#include <stdio.h> // For printf.


/**
 * @defgroup switch_gateway Switch/Gateway
 * @brief Implementation of switch/gateway functionality.
 *
 * This file contains the implementation of switch/gateway functionality. It includes
 * data structures, functions, and processes related to switch/gateway operations.
 */

/**@{*/

// Creates an instance of a broadcast connection.
#define MAX_NODES 5 /**< Maximum number of nodes expected in the network, used to define table size */
#define INFINITY_HOPS 255 /**< Value to represent infinity hops */
#define MAX_QUEUE_SIZE 10 /**< Maximum number of packets that the queue can hold */

// MAC LAYER PARAMETERS

/**
 * @def NETSTACK_CONF_RDC
 * @brief Definition for NullRDC driver.
 */
#define NETSTACK_CONF_RDC nullrdc_driver

/** Structure to hold routing table entry */
struct routing_entry {
  linkaddr_t node_address; /**< Node address */
  uint8_t hops; /**< Number of hops to the node */
  linkaddr_t next_hop; /**< Next hop address */
  int8_t node_id; /**< Node ID */
  char node_type; /**< Node type */
  uint8_t still_active; /**< Flag to indicate if entry is still active */
};

/** Sensor Message Structure */
struct sensor_message {
    int16_t force; /**< Force value */
    int16_t oximeter; /**< Oximeter value */  
    int32_t path; /**< Path of the packet as it reaches the gateway */
	int16_t batteryLevel;  /**< Battery Voltage value */
};

/** Unicast packet structure */
struct unicast_packet {
  linkaddr_t destination; /**< Destination address */
  struct sensor_message data; /**< Sensor data */
  uint8_t length; /**< Length of the packet */
};

struct unicast_packet unicast_queue[MAX_QUEUE_SIZE]; /**< Unicast packet queue */
int queue_size = 0;  /**< Current size of the queue */


struct routing_entry routing_table[MAX_NODES]; /**< Routing table */
int num_nodes = 0; /**< Number of nodes found in the network */
int last_entry = -1; /**< Index of the last non-zero entry in the routing table */
static char self_node_type = 'G'; /**< Storing the Node type */
int32_t node_number = 7; /**< Node number used to set current Node id in the path.*/
int8_t node_number2=7; /**< Node number used to set current node id in the routing table. */

/** Routing Process to hanlde network discovery*/
PROCESS(routing_process, "Routing Process");
/** Timeout Process which marks/checks at regular intervals for entries that are expired*/
PROCESS(timeout_process, "Timeout Process");
/** Unicast Forward Process which handles forwarding the packets in the queue */
PROCESS(unicast_forward_process, "Unicast Forward Process");
/** Autostart processes */
AUTOSTART_PROCESSES(
  &routing_process,
  &timeout_process,
  &unicast_forward_process
);

static struct broadcast_conn broadcast; /**< Declare the broadcast connection */
static struct unicast_conn unicast; /**< Declare the unicast connection */
static struct etimer timeout_timer; /**< Timer for handling timeout of entries */

/**
 * @brief Function to broadcast routing table information
 */
static void broadcast_routing_table();

/**
 * @brief Function to update routing table. It goes through the recieved routing table and checks if there are entries which are valid and puts them into the current routing rable
 * @param received_table Received routing table
 * @param sender_addr Sender's address
 */
static void update_routing_table(const struct routing_entry *received_table, const linkaddr_t *sender_addr);

/**
 * @brief Function to handle timeout of entries to mark old entries as expired
 */
static void handle_timeout();

/**
 * @brief Function to receive broadcast packets
 * @param c Broadcast connection
 * @param from Sender's address
 */
static void recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from);

/**
 * @brief Function to process received unicast packets and add them to the queue for forwarding
 * @param c Unicast connection
 * @param from Sender's address* @param c Unicast connection
 * @param from Sender's address
 */
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from);

/**
 * @brief Initializes the switch gateway functionality.
 *
 * This function initializes the necessary components and connections for the switch gateway functionality.
 */
void switch_gateway_init();

/**
 * @brief Starts the switch gateway processes.
 *
 * This function starts the processes required for the switch gateway functionality.
 */
void switch_gateway_start();
/**@}*/

/*---------------------------------------------------------------------------*/
/** @addtogroup switch_gateway
 * @{
 */


/*---------------------------------------------------------------------------*/
/**
 * @brief Function to broadcast routing table information
 *
 * This function broadcasts the routing table information to neighboring nodes.
 */
static void broadcast_routing_table() {
  packetbuf_copyfrom(&routing_table, sizeof(routing_table));
  for (int i = 0; i <= last_entry; i++) {
    // Check if the entry is still active
    if (routing_table[i].node_type == 'G' && routing_table[i].hops != 255) {
      // Entry is still active, mark it as inactive
      broadcast_send(&broadcast);
    }
  }
}

/**
 * @brief Function to update routing table. It goes through the entries of the recieved routing table and adds/modifies the valid entries to it's own routing table.
 * @param received_table Received routing table
 * @param sender_addr Sender's address
 */
static void update_routing_table(const struct routing_entry *received_table, const linkaddr_t *sender_addr) {
    int i;
    for (i = 0; i < MAX_NODES; i++) {
      // Check if the entry in the received table is valid
      if (linkaddr_cmp(&(received_table[i].node_address), &linkaddr_null) ||
          linkaddr_cmp(&(received_table[i].node_address), &linkaddr_node_addr) ||
          linkaddr_cmp(&(received_table[i].next_hop), &linkaddr_node_addr)) {
        continue;
      }

    int existing_entry = -1;
    // Check if the node already exists in our routing table
    for (int j = 0; j <= last_entry; j++) {
      if (linkaddr_cmp(&(received_table[i].node_address), &(routing_table[j].node_address))) {
        existing_entry = j;
        break;
      }
    }

    // Update or append the entry in our routing table
    if (existing_entry != -1) {
      // Node already exists, update the entry
      if (received_table[i].hops + 1 < routing_table[existing_entry].hops) {
        routing_table[existing_entry].hops = received_table[i].hops + 1;

        // Set the next hop address based on the sender's address
        routing_table[existing_entry].next_hop = *sender_addr;

        // Mark the entry as still active
        routing_table[existing_entry].still_active = 1;

      }
    } else {
      // Node doesn't exist, append the entry
      last_entry++;
      routing_table[last_entry].node_address = received_table[i].node_address;
      routing_table[last_entry].hops = received_table[i].hops + 1;
      routing_table[last_entry].node_type = received_table[i].node_type;
      routing_table[last_entry].node_id = received_table[i].node_id;

      // Set the next hop address based on the sender's address
      routing_table[last_entry].next_hop = *sender_addr;

      // Mark the entry as still active
      routing_table[last_entry].still_active = 1;

    }
  }
}

/**
 * @brief Function to handle timeout of entries
 *
 * This function handles the timeout of entries in the routing table. It marks inactive entries as inactive and modifies entries that were previously marked inactive to have hop count set to infinity.
 * and setsthem to infinity hops and null next hop.
 */
static void handle_timeout() {
  int i;
  for (i = 1; i <= last_entry; i++) {
    // Check if the entry is still active
    if (routing_table[i].still_active) {
      // Entry is still active, mark it as inactive
      routing_table[i].still_active = 0;
    } else {
      // Entry is inactive, set hop count to infinity and next hop to null
      routing_table[i].hops = INFINITY_HOPS;
      routing_table[i].next_hop = linkaddr_null;
    }
  }
}


/**
 * @brief Function to receive broadcast packets. 
 * @param c Broadcast connection
 * @param from Sender's address
 *
 * This function is called when a broadcast packet is received. Recieves the Routing tables and triggers the table update process.
 */
static void recv_broadcast(struct broadcast_conn *c, const linkaddr_t *from) {

  int16_t rssi = packetbuf_attr(PACKETBUF_ATTR_RSSI);
  printf("Received a table with RSSI: %d\n", rssi);

  // Check if the received broadcast is from a neighbor node
  if (!linkaddr_cmp(from, &linkaddr_node_addr)) {
    // Get the received routing table
    struct routing_entry received_table[MAX_NODES];
    packetbuf_copyto(&received_table);

    // Print the received packet
    printf("Received Routing Table from: %d.%d\n", from->u8[0], from->u8[1]);
    // printf("Received routing table:\n");
    int i;
    // Update the routing table
    update_routing_table(received_table, from);

    // Print the updated routing table
	printf("Updated routing table:\n");
	printf("{");
	for (int i = 0; i <= last_entry; i++) {
		printf("\"Entry %d\": {", i+1);
		printf("\"node_address\": \"%d.%d\",", routing_table->node_address.u8[0], routing_table->node_address.u8[1]);
		// printf("\"node_address\": \"%u\",", routing_table[i].node_address);
		printf("\"hops\": \"%u\",", routing_table[i].hops);
		printf("\"next_hop\": \"%d.%d\",", routing_table->next_hop.u8[0], routing_table->next_hop.u8[1]);
		printf("\"node_id\": \"%d\",", routing_table[i].node_id);
		printf("\"node_type\": \"%c\",", routing_table[i].node_type);
		printf("\"still_active\": \"%s\"}", routing_table[i].still_active ? "true" : "false");
		if (i < last_entry) {
			printf(",");
		}
	}
	printf("}\r\n");
  }
}

/**
 * @brief Function to process received unicast packets
 * @param c Unicast connection
 * @param from Sender's address
 *
 * This function is called when a unicast packet is received. If the self node type is 'G', it prints the sensor data.
 * If the self node type is not 'G', it finds the node with type 'G' in the routing table and adds the packet to the forwarding queue.
 */
static void recv_unicast(struct unicast_conn *c, const linkaddr_t *from) {
  printf("Received unicast packet from: %d.%d\n", from->u8[0], from->u8[1]);

  // Check if the self node type is 'G'
  if (self_node_type == 'G') {

    struct sensor_message data;
    packetbuf_copyto(&data);

    data.path = data.path*10 + node_number;

    printf("Sensor Packet received from: %d.%d\n", from->u8[0], from->u8[1]);
    // Print the sensor data in JSON format
    printf("{\"Force\": %d, \"Oximeter\": %d, \"Path\": %d,  \"Battery\": %d}\n", data.force, data.oximeter, data.path, data.batteryLevel);


    return;
  }

  // Find the node with type 'G' and forward the packet
  int i;
  for (i = 0; i <= last_entry; i++) {
    if (routing_table[i].node_type == 'G') {
      linkaddr_t next_hop = routing_table[i].next_hop;

      // Retrieve the packet from the receive buffer
      struct sensor_message data;
      packetbuf_copyto(&data);

      data.path = data.path*10 + node_number;
      // uint8_t* packet_data = packetbuf_dataptr();
      int packet_length = packetbuf_datalen();

      // Check if the queue is full
      if (queue_size < MAX_QUEUE_SIZE) {
        // Add the packet to the queue
        unicast_queue[queue_size].data =  data;
        unicast_queue[queue_size].length = packet_length;
        unicast_queue[queue_size].destination = next_hop;
        queue_size++;

        printf("Queued unicast packet for forwarding: %d.%d\n", next_hop.u8[0], next_hop.u8[1]);
      } else {
        printf("Warning: Unicast queue is full, packet dropped!\n");
      }
      break;
    }
  }
}



static const struct unicast_callbacks unicast_callbacks = {recv_unicast};

static const struct broadcast_callbacks broadcast_callbacks = {recv_broadcast};

/**
 * @brief Main routing process to inititite the network discovery and update.
 */
PROCESS_THREAD(routing_process, ev, data) {
  static struct etimer et; // Declare the etimer variable

  PROCESS_EXITHANDLER(broadcast_close(&broadcast);)

  PROCESS_BEGIN();

  NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_CHANNEL, 15);
  NETSTACK_CONF_RADIO.set_value(RADIO_PARAM_TXPOWER, 7);

  broadcast_open(&broadcast, 129, &broadcast_callbacks);

  // Initialize routing table for the current node
  routing_table[num_nodes].node_address = linkaddr_node_addr;
  routing_table[num_nodes].hops = 0;
  routing_table[num_nodes].next_hop = linkaddr_node_addr;
  routing_table[num_nodes].node_id = node_number2;
  routing_table[num_nodes].node_type = self_node_type;
  routing_table[num_nodes].still_active = 1;
  num_nodes++;
  last_entry++;

  etimer_set(&et, CLOCK_SECOND * 5); // Set the timer for 5 seconds

  while (1) {
    PROCESS_WAIT_EVENT();

    if (ev == PROCESS_EVENT_TIMER && etimer_expired(&et)) {

      // Broadcast the updated routing table
      broadcast_routing_table();

      etimer_reset(&et); // Reset the timer
    }
  }

  PROCESS_END();
}

/**
 * @brief Timeout process to handle network failures and timeout routing table entries that are no longer valid
 */
PROCESS_THREAD(timeout_process, ev, data) {
  PROCESS_BEGIN();

  while (1) {
    // Wait for 8 seconds before triggering the timeout event
    etimer_set(&timeout_timer, CLOCK_SECOND * 8);
    PROCESS_WAIT_EVENT_UNTIL(ev == PROCESS_EVENT_TIMER);

    // Handle timeout of entries
    handle_timeout();
  }

  PROCESS_END();
}

/**
 * @brief Unicast forward process handles forwarding the packets that are stored in the queue in a way that avoids collisons.
 */
PROCESS_THREAD(unicast_forward_process, ev, data) {
  static struct etimer et;

  PROCESS_BEGIN();

  unicast_open(&unicast, 146, &unicast_callbacks);

  etimer_set(&et, CLOCK_SECOND); // Set the timer interval to 1 second

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));

    // Check if there are packets in the queue
    if (queue_size > 0) {
      // Check if the node is currently receiving or sending a packet
      if (NETSTACK_RADIO.pending_packet()) {
        // If the node is busy, print a message and skip forwarding for now
        printf("Node busy, cannot forward unicast packet at the moment!\n");
      } else {
        // Retrieve the next packet from the queue
        struct unicast_packet packet = unicast_queue[0];
        printf("Force: %d\r\n", packet.data.force);
        printf("Oximeter: %d\r\n", packet.data.oximeter);
        printf("Path: %d\r\n", packet.data.path);
        printf("Battery: %d\r\n", packet.data.batteryLevel);

        // Copy the packet data to the packet buffer
        packetbuf_copyfrom(&packet.data, sizeof(packet.data));
        // memcpy(packetbuf_dataptr(), packet.data, packet.length);
        // packetbuf_set_datalen(packet.length);

        // Send the packet using unicast
        // Replace the following line with your own implementation of sending the packet
        printf("Forwarding unicast packet to: %d.%d\n", packet.destination.u8[0], packet.destination.u8[1]);
        unicast_send(&unicast, &packet.destination);

        // Remove the forwarded packet from the queue
        int i;
        for (i = 1; i < queue_size; i++) {
          unicast_queue[i - 1] = unicast_queue[i];
        }
        queue_size--;

        printf("Forwarded unicast packet removed from the queue.\n");
      }
    }

    etimer_reset(&et);
  }

  PROCESS_END();
}
