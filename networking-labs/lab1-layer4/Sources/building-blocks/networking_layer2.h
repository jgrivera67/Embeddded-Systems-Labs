/**
 * @file networking_layer2.h
 *
 * Networking layer 2 interface
 *
 * @author German Rivera
 */

#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_H_

#include <stdint.h>
#include <stdbool.h>
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "network_packet.h"
#include "microcontroller.h"
#include "networking_layer2_ethernet.h"

struct ethernet_phy_device;

/**
 * Number of layer-2 end points (NICs)
 */
#define NUM_NET_LAYER2_END_POINTS 1

/**
 * Types of layer-2 end points
 */
enum net_layer2_end_point_types {
    NET_LAYER2_ETHERNET,
};

/**
 * Networking layer-2 (data link layer) local end point
 */
struct net_layer2_end_point {
    /*
     * Signature for run-time type checking
     */
#   define NET_LAYER2_END_POINT_SIGNATURE    GEN_SIGNATURE('L', '2', 'E', 'P')
    uint32_t signature;

    /**
     * Type of layer-2 end point
     */
    enum net_layer2_end_point_types type;

    /**
     * Flag indicating that net_layer2_end_point_init() has been called
     * for this layer-2 end point
     */
    bool initialized;

    /**
     * Layer-3 end point associated to this layer-2 end point
     */
    struct net_layer3_end_point *const layer3_end_point_p;

    /**
     * Ethernet MAC device
     */
    const struct ethernet_mac_device *const ethernet_mac_p;

    /**
     * Ethernet MAC address of this layer-2 end-point.
     */
    struct ethernet_mac_address mac_address;

    /**
     * Queue of received Rx packets (non-empty Rx buffers)
     */
    struct net_packet_queue rx_packet_queue;

    /**
     * Rx packet buffers
     */
    struct network_packet rx_packets[NET_MAX_RX_PACKETS];

    /**
     * Layer-2 packet receiving task
     */
    struct rtos_task packet_receiver_task;

}  __attribute__ ((aligned(MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct net_layer2_end_point) % MPU_REGION_ALIGNMENT == 0);

/**
 * Pool of Tx packets
 */
struct net_tx_packet_pool {
    /**
     * Free list
     */
    struct net_packet_queue free_list;

    /**
     * Tx packet buffers
     */
    struct network_packet tx_packets[NET_MAX_TX_PACKETS];
};

/**
 * Networking layer-2 global state variables
 */
struct net_layer2 {
    /**
     * Flag indicating if this layer has been initialized
     */
    bool initialized;

    /**
     * Flag indicating if tracing is currently enabled for this layer
     */
    bool tracing_on;

    /**
     * Total number of received Ethernet frames accepted
     */
    volatile uint32_t rx_packets_accepted_count;

    /**
     * Total number of received Ethernet frames dropped
     */
    volatile uint32_t rx_packets_dropped_count;

    /**
     * Total number of Ethernet frames sent (placed for transmission)
     */
    volatile uint32_t sent_packets_count;

    /**
     * Global pool of free Tx packets shared among all layer-2 end points
     */
    struct net_tx_packet_pool free_tx_packet_pool;

    /**
     * Local layer-2 end points (network interfaces)
     */
    struct net_layer2_end_point local_layer2_end_points[NUM_NET_LAYER2_END_POINTS];
}  __attribute__ ((aligned(MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct net_layer2) % MPU_REGION_ALIGNMENT == 0);


void net_layer2_init(void);

void net_layer2_start(void);

bool net_layer2_end_point_link_is_up(const struct net_layer2_end_point *layer2_end_point_p);

void net_layer2_end_point_set_loopback(const struct net_layer2_end_point *layer2_end_point_p,
	                                   bool on);

void net_layer2_get_mac_addr(const struct net_layer2_end_point *layer2_end_point_p,
                             struct ethernet_mac_address *mac_addr_p);

struct network_packet *net_layer2_allocate_tx_packet(bool free_after_tx_complete);

void net_layer2_free_tx_packet(struct network_packet *tx_packet_p);

void net_layer2_dequeue_rx_packet(
        struct net_layer2_end_point *layer2_end_point_p,
        struct network_packet **rx_packet_pp);

void net_layer2_enqueue_rx_packet(
        struct net_layer2_end_point *layer2_end_point_p,
        struct network_packet *rx_packet_p);

void net_recycle_rx_packet(struct network_packet *rx_packet_p);

error_t net_layer2_send_ethernet_frame(
    const struct net_layer2_end_point *layer2_end_point_p,
    const struct ethernet_mac_address *dest_mac_addr_p,
    struct network_packet *tx_packet_p,
    uint16_t frame_type,
    size_t data_payload_length);

void net_layer2_start_tracing(void);

void net_layer2_stop_tracing(void);

extern struct net_layer2 g_net_layer2;

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_H_ */
