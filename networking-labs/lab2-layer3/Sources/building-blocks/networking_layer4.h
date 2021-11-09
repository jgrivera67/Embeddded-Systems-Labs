/**
 * @file networking_layer4.h
 *
 * Networking layer 4 interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_H_

#include <stdint.h>
#include <stdbool.h>
#include "net_layer4_end_point.h"
#include "networking_layer4_udp.h"

/**
 * First ephemeral port. All port number greater or equal
 * to this value are ephemeral ports.
 */
#define NET_FIRST_EPHEMERAL_PORT    UINT16_C(49152)

/**
 * Networking layer-4 - global state variables
 */
struct net_layer4 {
    /**
     * Flag indicating if this layer has been initialized
     */
    bool initialized;

    /**
     * Flag indicating if tracing is currently enabled for this layer
     */
    bool tracing_on;

    /**
     * UDP specific
     */
    struct net_layer4_udp udp;

    /*
     * NOTE: Other protocol-specific layer-4 structures
     * can be added here as needed
     */

} __attribute__ ((aligned(MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct net_layer4) % MPU_REGION_ALIGNMENT == 0);

void net_layer4_init(void);

void net_layer4_end_point_init(struct net_layer4_end_point *layer4_end_point_p,
		                       enum net_layer4_protocols protocol);

void net_layer4_end_point_list_init(struct net_layer4_end_point_list *list_p,
                                    enum net_layer4_protocols protocol);

void net_layer4_end_point_list_add(struct net_layer4_end_point_list *list_p,
                                   struct net_layer4_end_point *elem_p);

void net_layer4_end_point_list_remove(struct net_layer4_end_point_list *list_p,
                                      struct net_layer4_end_point *elem_p);

struct net_layer4_end_point *
net_layer4_end_point_list_lookup(struct net_layer4_end_point_list *list_p,
                                 uint_fast16_t layer4_port);

void net_layer4_start_tracing(void);

void net_layer4_stop_tracing(void);

extern struct net_layer4 g_net_layer4;

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_H_ */
