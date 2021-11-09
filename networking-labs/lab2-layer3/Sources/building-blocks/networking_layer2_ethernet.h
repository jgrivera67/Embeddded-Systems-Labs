/**
 * @file networking_layer2.h
 *
 * Networking layer 2 interface: Ethernet
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_ETHERNET_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_ETHERNET_H_

#include <stdint.h>
#include <stddef.h>
#include "compile_time_checks.h"

/*
 * Bit masks for first byte of a MAC address
 */
#define MAC_MULTICAST_ADDRESS_MASK UINT8_C(0x1)
#define MAC_PRIVATE_ADDRESS_MASK   UINT8_C(0x2)

/**
 * Copies a MAC address. The source and destination must be at least
 * 2-byte aligned
 */
#define COPY_MAC_ADDRESS(_dest_p, _src_p) \
    do {                                \
        (_dest_p)->hwords[0] = (_src_p)->hwords[0];            \
        (_dest_p)->hwords[1] = (_src_p)->hwords[1];            \
        (_dest_p)->hwords[2] = (_src_p)->hwords[2];            \
    } while (0)

/**
 * Compares two MAC addresses. Their storage must be at least 2-byte aligned
 */
#define MAC_ADDRESSES_EQUAL(_mac_addr1_p, _mac_addr2_p) \
     ((_mac_addr1_p)->hwords[0] == (_mac_addr2_p)->hwords[0] &&    \
      (_mac_addr1_p)->hwords[1] == (_mac_addr2_p)->hwords[1] &&    \
      (_mac_addr1_p)->hwords[2] == (_mac_addr2_p)->hwords[2])

/**
 * Ethernet MAC address in network byte order
 */
struct ethernet_mac_address {
    union {
        /**
         * bytes[0] = most significant byte
         * bytes[5] = less significant byte
         */
        uint8_t bytes[6];
        uint16_t hwords[3];
    };
};

C_ASSERT(sizeof(struct ethernet_mac_address) == 6);

/**
 * Ethernet header in network byte order
 */
struct ethernet_header {
    /**
     * Alignment padding so that data payload of the Ethernet frame
     * starts at a 32-bit boundary.
     */
    uint16_t alignment_padding;

    /**
     * Destination MAC address
     */
    struct ethernet_mac_address dest_mac_addr;

    /**
     * Source MAC address
     */
    struct ethernet_mac_address source_mac_addr;

    /**
     * Ethernet frame type
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t frame_type;
#   define FRAME_TYPE_ARP_PACKET        0x806
#   define FRAME_TYPE_IPv4_PACKET       0x800
#   define FRAME_TYPE_VLAN_TAGGED_FRAME 0x8100
#   define FRAME_TYPE_IPv6_PACKET       0x86dd
};

C_ASSERT(sizeof(struct ethernet_header) == 16);
C_ASSERT(offsetof(struct ethernet_header, dest_mac_addr) == 2);
C_ASSERT(offsetof(struct ethernet_header, source_mac_addr) == 8);
C_ASSERT(offsetof(struct ethernet_header, frame_type) == 14);

extern const struct ethernet_mac_address g_ethernet_broadcast_mac_addr;
extern const struct ethernet_mac_address g_ethernet_null_mac_addr;

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER2_ETHERNET_H_ */
