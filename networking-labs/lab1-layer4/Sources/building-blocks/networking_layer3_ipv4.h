/**
 * @file networking_layer3_ipv4.h
 *
 * Networking layer-3 interface: IPv4
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV4_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV4_H_

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "rtos_wrapper.h"
#include "io_utils.h"
#include "compile_time_checks.h"
#include "network_packet.h"
#include "net_layer4_end_point.h"

/**
 * Null IPv4 address (0.0.0.0)
 */
#define IPV4_NULL_ADDR    UINT32_C(0x0)

/**
 * Broadcast IPv4 address (255.255.255.255)
 */
#define IPV4_BROADCAST_ADDR UINT32_C(0xffffffff)

/*
 * Mask for first octet of an IPv4 multicast address
 * (224.0.0.0 to 239.255.255.255)
 */
#define IPV4_MULTICAST_ADDRESS_MASK UINT8_C(0xe0)

/**
 * Number of entries for the IPv4 ARP cache table
 */
#define ARP_CACHE_NUM_ENTRIES    16

/**
 * ARP cache entry lifetime in ticks (20 minutes)
 */
#define ARP_CACHE_ENTRY_LIFETIME_IN_TICKS \
    MILLISECONDS_TO_TICKS(20u * 60 * 1000)

/**
 * Timeout in milliseconds to wait for an ARP reply after sending a
 * non-gratuitous ARP request (3 minutes)
 */
#define ARP_REPLY_WAIT_TIMEOUT_IN_MS    (3u * 60 * 1000)

/**
 * Maximum number of ARP requests to be sent for a given destination
 * IP address, before failing with "unreachable destination".
 */
#define ARP_REQUEST_MAX_RETRIES    64

/**
 * Maximum data payload size of an IPv4 packet
 */
#define NET_MAX_IPV4_PACKET_PAYLOAD_SIZE \
        (NET_PACKET_DATA_BUFFER_SIZE - \
         (sizeof(struct ethernet_header) + sizeof(struct ipv4_header)))

/**
 * Copies an IPv4 address, where the source or destination are not 4-byte
 * aligned, but they must be at least 2-byte aligned.
 */
#define COPY_UNALIGNED_IPv4_ADDRESS(_dest_p, _src_p) \
    do {                                \
        (_dest_p)->hwords[0] = (_src_p)->hwords[0];            \
        (_dest_p)->hwords[1] = (_src_p)->hwords[1];            \
    } while (0)

/**
 * Compares two IPv4 addresses stored at locations that are not 4-byte
 * aligned, but they must be at least 2-byte aligned.
 */
#define UNALIGNED_IPv4_ADDRESSES_EQUAL(_ip_addr1_p, _ip_addr2_p) \
    ((_ip_addr1_p)->hwords[0] == (_ip_addr2_p)->hwords[0] &&    \
     (_ip_addr1_p)->hwords[1] == (_ip_addr2_p)->hwords[1])

/**
 * Build an IPv4 subnet mask in network byte order (big endian),
 * assuming that the target CPU runs in little endian.
 *
 * NOTE: This macro can be used in static initializers, if
 * '_num_bits' is a constant,
 */
#define IPv4_SUBNET_MASK(_num_bits) \
    ((_num_bits) % 8 == 0 ?                                             \
         MULTI_BIT_MASK((_num_bits) - 1, 0) :                           \
         (MULTI_BIT_MASK((((_num_bits) / 8) * 8) - 1, 0) |              \
          (MULTI_BIT_MASK(7, 7 - ((_num_bits) % 8)) <<                  \
           (((_num_bits) / 8) * 8))))

/**
 * Check if two IPv4 addresses are in the same subnet, for a given
 * subnet mask
 */
#define SAME_IPv4_SUBNET(_local_ip_addr_p, _dest_ip_addr_p, _subnet_mask) \
    (((_local_ip_addr_p)->value & (_subnet_mask)) == \
     ((_dest_ip_addr_p)->value & (_subnet_mask)))

/**
 * Returns pointer to the IPv4 header of an IPv4 packet
 */
#define GET_IPV4_HEADER(_net_packet_p) \
        ((struct ipv4_header *)((_net_packet_p)->data_buffer +    \
                sizeof(struct ethernet_header)))

/**
 * Return IP version number for a given network packet:
 * - 4 for IPv4
 * - 6 for IPv6
 */
#define GET_IP_VERSION(_ipv4_header_p) \
        GET_BIT_FIELD((_ipv4_header_p)->version_and_header_length, \
                      IP_VERSION_MASK, IP_VERSION_SHIFT)

/**
 * Returns pointer to the data payload area of an IPv4 packet
 */
#define GET_IPV4_DATA_PAYLOAD_AREA(_net_packet_p)   \
        ((void *)((_net_packet_p)->data_buffer +    \
          (sizeof(struct ethernet_header) + \
                   sizeof(struct ipv4_header))))

/**
 * Tell if a given IPv4 address is a multicast address
 */
#define IPV4_ADDR_IS_MULTICAST(_ipv4_addr_p) \
        (((_ipv4_addr_p)->bytes[0] & IPV4_MULTICAST_ADDRESS_MASK) == \
         IPV4_MULTICAST_ADDRESS_MASK)

/**
 * IPv4 address in network byte order
 */
struct ipv4_address {
    union {
    /**
     * bytes[0] = most significant byte
     * bytes[3] = least significant byte
     */
    uint8_t bytes[4];

    uint16_t hwords[2];

    /**
     * IP address seen as a 32-bit value in big endian
     */
    uint32_t value;
    } __attribute__((packed));
};
C_ASSERT(sizeof(struct ipv4_address) == sizeof(uint32_t));

/**
 * Header of an IPv4 packet in network byte order
 * (An IPv4 packet is encapsulated in an Ethernet frame)
 */
struct ipv4_header {
    /**
     * IP version and header length
     * - Version is 4 for IPv4
     * - Header length is in 32-bit words and if there are
     *   no options, its value is 5
     */
    uint8_t version_and_header_length;
#   define IP_VERSION_MASK        MULTI_BIT_MASK(7, 4)
#   define IP_VERSION_SHIFT        4
#   define IP_HEADER_LENGTH_MASK    MULTI_BIT_MASK(3, 0)
#   define IP_HEADER_LENGTH_SHIFT   0

    /**
     * type of service
     */
    uint8_t type_of_service;
#   define TOS_PRECEDENCE_MASK            MULTI_BIT_MASK(7, 5)
#   define TOS_PRECEDENCE_SHIFT            5
#   define TOS_MINIMIZE_DELAY_MASK        BIT(4)
#   define TOS_MAXIMIZE_THROUGHPUT_MASK        BIT(3)
#   define TOS_MAXIMIZE_RELIABILITY_MASK    BIT(2)
#   define TOS_MINIMIZE_MONETARY_COST_MASK  BIT(1)

    /**
     * total packet length (header + data payload) in bytes
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t total_length;

    /**
     * Identification number for the IP packet
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t identification;

    /**
     * Flags and fragment_offset
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t flags_and_fragment_offset;
#   define IP_FLAG_RESERVED_MASK    BIT(15)
#   define IP_FLAG_DONT_FRAGMENT_MASK    BIT(14)
#   define IP_FLAG_MORE_FRAGMENTS_MASK    BIT(13)
#   define IP_FRAGMENT_OFFSET_MASK    MULTI_BIT_MASK(12, 0)
#   define IP_FRAGMENT_OFFSET_SHIFT    0

    /**
     * Packet time to live
     */
    uint8_t time_to_live;

    /**
     * Encapsulated protocol type (IP packet type)
     */
    uint8_t protocol_type;
#	define IP_PACKET_TYPE_ICMP       UINT8_C(0x1)
#	define IP_PACKET_TYPE_TCP        UINT8_C(0x6)
#	define IP_PACKET_TYPE_UDP        UINT8_C(0x11)
#	define IP_PACKET_TYPE_ICMPV6     UINT8_C(0x3a)

    /**
     * Header checksum
     */
    uint16_t header_checksum;

    /**
     * Source (sender) IPv4 address
     */
    struct ipv4_address source_ip_addr;

    /**
     * Destination (receiver) IPv4 address
     */
    struct ipv4_address dest_ip_addr;
}; //  __attribute__((packed));

C_ASSERT(sizeof(struct ipv4_header) == 20);

/**
 * ARP packet layout in network byte order
 * (An ARP packet is encapsulated in an Ethernet frame)
 */
struct arp_packet {
    /**
     * Link address type is 0x1 for Ethernet
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t link_addr_type;

    /**
     * Network address type is 0x800 for IPv4
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t network_addr_type;

    /**
     * link address size is 6 for MAC addresses
     */
    uint8_t link_addr_size;

    /**
     * Network address size is 4 for IPv4 addresses
     */
    uint8_t network_addr_size;

    /**
     * ARP operation (ARP_REQUEST or ARP_REPLY)
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t operation;
#   define ARP_REQUEST    0x1
#   define ARP_REPLY    0x2

    /**
     * Source (sender) MAC address
     */
    struct ethernet_mac_address source_mac_addr;

    /**
     * Source (sender) IPv4 address
     */
    struct ipv4_address source_ip_addr;

    /**
     * Destination (target) MAC address
     */
    struct ethernet_mac_address dest_mac_addr;

    /**
     * Destination (target) IPv4 address
     */
    struct ipv4_address dest_ip_addr;
}; // __attribute__((packed));

C_ASSERT(offsetof(struct arp_packet, source_mac_addr) == 8);
C_ASSERT(offsetof(struct arp_packet, source_ip_addr) == 14);
C_ASSERT(offsetof(struct arp_packet, dest_mac_addr) == 18);
C_ASSERT(offsetof(struct arp_packet, dest_ip_addr) == 24);

/**
 * IPv4 ICMPv4 header layout
 * (An ICMPv4 message is encapsulated in an IPv4 packet)
 */
struct icmpv4_header {
    /**
     * Message type
     */
    uint8_t msg_type;
#   define ICMP_TYPE_PING_REPLY     0
#   define ICMP_TYPE_PING_REQUEST   8
    /**
     * Message code
     */
    uint8_t msg_code;
#   define ICMP_CODE_PING_REPLY     0
#   define ICMP_CODE_PING_REQUEST   0

    /**
     * message checksum
     */
    uint16_t msg_checksum;
}; //  __attribute__((packed));

C_ASSERT(sizeof(struct icmpv4_header) == 4);

/**
 * ICMPv4 echo request/reply message layout
 */
struct icmpv4_echo_message {
    struct icmpv4_header header;
    uint16_t identifier;
    uint16_t seq_num;
};

C_ASSERT(offsetof(struct icmpv4_echo_message, identifier) ==
         sizeof(struct icmpv4_header));

/**
 * IPv4 DHCP message layout
 */
struct dhcp_message {
    uint8_t op;
    uint8_t hardware_type;
    uint8_t hw_addr_len;
    uint8_t hops;
    uint32_t transaction_id;
    uint16_t seconds;
    uint16_t flags;
    struct ipv4_address client_ip_addr;
    struct ipv4_address your_ip_addr;
    struct ipv4_address next_server_ip_addr;
    struct ipv4_address relay_agent_ip_addr;
    struct ethernet_mac_address client_mac_addr;
    uint8_t zero_filled[10 + 192];
    uint32_t magic_cookie;
    uint8_t options[];
};

/**
 * States of an ARP cache entry
 */
enum arp_cache_entry_states {
    ARP_ENTRY_INVALID = 0,
    ARP_ENTRY_HALF_FILLED, /* arp request sent but no reply received yet */
    ARP_ENTRY_FILLED,
};

/**
 * IPv4 ARP cache entry
 */
struct arp_cache_entry {
    struct ipv4_address dest_ip_addr;
    struct ethernet_mac_address dest_mac_addr;
    enum arp_cache_entry_states state;

    /**
     * Timestamp in ticks when the last ARP request for this entry was sent.
     * It is used to determine if we have waited too long for the ARP reply,
     * and need to send another ARP request.
     */
    uint32_t arp_request_time_stamp;

    /**
     * Timestamp in ticks when this entry was last filled.
     * It is used to determine when the entry has expired and a new ARP request
     * must be sent.
     */
    uint32_t entry_filled_time_stamp;

    /**
     * Timestamp in ticks when the last lookup was done for this entry. It is
     * used to determine the least recently used entry, for cache entry
     * replacement.
     */
    uint32_t last_lookup_time_stamp;
};

/**
 * IPv4 ARP cache
 */
struct arp_cache {
    /**
     * Mutex to serialize access to the ARP cache
     */
    struct rtos_mutex mutex;

    /**
     * Semaphore signaled when the ARP cache is updated
     */
    struct rtos_semaphore cache_updated_semaphore;

    /**
     * Array of cache entries
     */
    struct arp_cache_entry entries[ARP_CACHE_NUM_ENTRIES];
};

/**
 * IPv4 network end point
 */
struct ipv4_end_point {
    /**
     * Local IPv4 address
     */
    struct ipv4_address local_ip_addr;

    /**
     * Subnet mask in network byte order
     */
    uint32_t subnet_mask;

    /**
     * Local IPv4 address
     */
    struct ipv4_address default_gateway_ip_addr;

    /**
     * DHCP lease time in seconds
     */
    uint32_t dhcp_lease_time;

    /**
     * Sequence number to use as the 'identification' field of the next
     * IP packet transmitted out of this network end-point
     */
    uint16_t next_tx_ip_packet_seq_num;

    /**
     * Queue of received ICMPv4 packets
     */
    struct net_packet_queue rx_icmpv4_packet_queue;

    /**
     * ARP cache
     */
    struct arp_cache arp_cache;

    /**
     * DHCPv4 UDP client end point
     */
    struct net_layer4_end_point dhcpv4_client_end_point;

    /**
     * ICMPv4 packet receiver task
     */
    struct rtos_task icmpv4_packet_receiver_task;

    /**
     * DHCPv4 client task
     */
    struct rtos_task dhcpv4_client_task;
};


/**
 * Networking layer-3 for IPv4 - global state variables
 */
struct net_layer3_ipv4 {
    /**
     * Flag indicating if there is an outstanding IPv4 ping request for which
     * no reply has been received yet
     */
    volatile bool expecting_ping_reply;

    /**
     * Number of received IPv4 packets accepted
     */
    volatile uint32_t rx_packets_accepted_count;

    /**
     * Number of received IPv4 packets dropped
     */
    volatile uint32_t rx_packets_dropped_count;

    /**
     * Number of IPv4 packets sent
     */
    volatile uint32_t sent_packets_count;

    /**
     * Queue of received IPPv4 ping replies
     */
    struct net_packet_queue rx_ipv4_ping_reply_packet_queue;

    /**
     * Mutex to serialize access to expecting_ping_reply
     */
    struct rtos_mutex expecting_ping_reply_mutex;

    /**
     * Semaphore to be signal when the ping reply for an outstanding ping
     * request has been received.
     */
    struct rtos_semaphore ping_reply_received_semaphore;
};


void net_layer3_ipv4_init(struct net_layer3_ipv4 *layer3_ipv4_p);

void net_layer3_ipv4_end_point_init(struct ipv4_end_point *ipv4_end_point_p);

void net_layer3_ipv4_end_point_start_tasks(struct ipv4_end_point *ipv4_end_point_p);

void net_layer3_receive_arp_packet(struct network_packet *rx_packet_p);

void net_layer3_receive_ipv4_packet(struct network_packet *rx_packet_p);

error_t net_layer3_send_ipv4_packet(const struct ipv4_address *dest_ip_addr_p,
                                    struct network_packet *tx_packet_p,
                                    size_t data_payload_length,
                                    uint_fast8_t ip_packet_type);

error_t net_layer3_send_ipv4_icmp_message(const struct ipv4_address *dest_ip_addr_p,
                                          struct network_packet *tx_packet_p,
                                          uint8_t msg_type,
                                          uint8_t msg_code,
                                          size_t data_payload_length);

void net_layer3_join_ipv4_multicast_group(const struct ipv4_address *multicast_addr_p);

void net_layer3_set_local_ipv4_address(const struct ipv4_address *ip_addr_p,
			                           uint8_t subnet_prefix);

void net_layer3_get_local_ipv4_address(struct ipv4_address *ip_addr_p,
		                               struct ipv4_address *subnet_mask_p);

bool net_layer3_parse_ipv4_addr(const char *ipv4_addr_string,
		                        struct ipv4_address *ipv4_addr_p,
							    uint8_t *subnet_prefix_p);

error_t net_layer3_send_ipv4_ping_request(
    const struct ipv4_address *dest_ip_addr_p,
    uint16_t identifier,
    uint16_t seq_num);


error_t net_layer3_receive_ipv4_ping_reply(
	uint32_t timeout_ms,
    struct ipv4_address *remote_ip_addr_p,
    uint16_t *identifier_p,
    uint16_t *seq_num_p);

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV4_H_ */
