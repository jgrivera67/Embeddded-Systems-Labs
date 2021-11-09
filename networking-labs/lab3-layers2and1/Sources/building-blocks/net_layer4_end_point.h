/**
 * @file net_layer4_end_point.h
 *
 * Networking layer-4 end point declarations
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NET_LAYER4_END_POINT_H_
#define SOURCES_BUILDING_BLOCKS_NET_LAYER4_END_POINT_H_

#include <stdint.h>
#include "network_packet.h"

/**
 * Layer-4 protocol types
 */
enum net_layer4_protocols {
	NET_LAYER4_UDP = 0x0,
	NET_LAYER4_TCP,
};

/**
 * Networking layer-4 (transport layer) local end point
 *
 * NOTE: There is no explicit IP address associated with a local layer-4
 * end-point. The local IP address is implicitly "INADDR_ANY".
 */
struct net_layer4_end_point {
    /**
     * Transport protocol associated with this layer-4 end point
     */
	enum net_layer4_protocols protocol;

    /**
     * Protocol-specific port number in network byte order (big endian).
     * Must be different from 0, if the layer-4 end point is bound,
     * or 0 otherwise.
     */
    uint16_t layer4_port;

    /**
     * Queue of incoming network packets received for this
     * layer-4 end point
     */
    struct net_packet_queue rx_packet_queue;

    /**
     * Pointer to list of layer-4 end points that contains
     * this layer-4 end point.
     */
    struct net_layer4_end_point_list *list_p;

    /**
     * Pointer to next layer-4 end point in the list of existing
     * Layer-4 end points for the same layer-4 protocol.
     */
    struct net_layer4_end_point *next_p;
};

/**
 * List of layer-4 end points
 */
struct net_layer4_end_point_list {
   /**
	 * Transport protocol associated with this layer-4 end point list
	 */
	enum net_layer4_protocols protocol;

	/**
	 * Number of elements in the list
	 */
	uint16_t length;

    /**
     * Pointer to the first element in the list or NULL if the list is empty
     */
    struct net_layer4_end_point *head_p;

    /**
     * Pointer to the last element in the list or NULL if the list is empty
     */
    struct net_layer4_end_point  *tail_p;
};

#endif /* SOURCES_BUILDING_BLOCKS_NET_LAYER4_END_POINT_H_ */
