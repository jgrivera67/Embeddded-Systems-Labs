/**
 * @file networking_layer3.h
 *
 * Networking layer 3 interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_H_

#include <stdint.h>
#include <stdbool.h>
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "networking_layer2.h"
#include "networking_layer3_ipv4.h"
#include "networking_layer3_ipv6.h"

/**
 * Convert a 16-bit value from host byte order to network byte order
 * (Do byte swap since Cortex-M is little endian)
 */
#define hton16(_x)  byte_swap16(_x)

/**
 * Convert a 16-bit value from network byte order to host byte order
 * (Do byte swap since Cortex-M is little endian)
 */
#define ntoh16(_x)  byte_swap16(_x)

/**
 * Convert a 32-bit value from host byte order to network byte order
 * (Do byte swap since Cortex-M is little endian)
 */
#define hton32(_x)  byte_swap32(_x)

/**
 * Convert a 32-bit value from network byte order to host byte order
 * (Do byte swap since Cortex-M is little endian)
 */
#define ntoh32(_x)  byte_swap32(_x)

/**
 * Networking layer-3 (network layer) local end point
 */
struct net_layer3_end_point {
    /*
     * Signature for run-time type checking
     */
#   define NET_LAYER3_END_POINT_SIGNATURE    GEN_SIGNATURE('L', '3', 'E', 'P')
    uint32_t signature;

    /**
     * Flag indicating that net_layer3_end_point_init() has been called
     * for this layer-2 end point
     */
    bool initialized;

    /**
     * Associated Layer-2 end point
     */
    struct net_layer2_end_point *const layer2_end_point_p;

    /**
     * IPv4 end point (used if layer3_packet_type == FRAME_TYPE_IPv4_PACKET)
     */
    struct ipv4_end_point ipv4;

    /**
     * IPv6 end point (used if layer3_packet_type == FRAME_TYPE_IPv6_PACKET)
     */
    struct ipv6_end_point ipv6;
};

/**
 * Layer-3 end point configuration
 */
struct net_layer3_end_point_config {
    /**
     * Local IPv4 address
     */
    struct ipv4_address ipv4_addr;

    /**
     * Subnet mask in network byte order
     */
    uint32_t ipv4_subnet_mask;

    /**
     * Local IPv4 address
     */
    struct ipv4_address default_gateway_ipv4_addr;

    /**
     * Local IPv6 address
     */
    struct ipv6_address ipv6_addr;
};

/**
 * Networking layer-3 - global state variables
 */
struct net_layer3 {
    /**
     * Flag indicating if this layer has been initialized
     */
    bool initialized;

    /**
     * Flag indicating if tracing is currently enabled for this layer
     */
    bool tracing_on;

    /**
     * IPv4 specific
     */
    struct net_layer3_ipv4 ipv4;

    /**
     * IPv6 specific
     */
    struct net_layer3_ipv6 ipv6;

    /**
     * Local Layer-3 end points (IPv4 and IPv6 nodes)
     *
     * NOTE: There is one layer-3 end-point for each existing layer-2 end point.
     */
    struct net_layer3_end_point local_layer3_end_points[NUM_NET_LAYER2_END_POINTS];
} __attribute__ ((aligned(MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct net_layer3) % MPU_REGION_ALIGNMENT == 0);

/**
 * Ethernet frame layout
 */
struct ethernet_frame {
    struct ethernet_header ethernet_header;
    union {
        struct arp_packet arp_packet;
        struct ipv4_header ipv4_header;
        struct ipv6_header ipv6_header;
    };
}; // __attribute__((packed));

C_ASSERT(offsetof(struct ethernet_frame, arp_packet) ==
     sizeof(struct ethernet_header));
C_ASSERT(offsetof(struct ethernet_frame, ipv4_header) ==
     sizeof(struct ethernet_header));
C_ASSERT(offsetof(struct ethernet_frame, ipv6_header) ==
     sizeof(struct ethernet_header));

void net_layer3_init(void);

void net_layer3_start_tasks(void);

void net_layer3_start_tracing(void);

void net_layer3_stop_tracing(void);

extern struct net_layer3 g_net_layer3;

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_H_ */
