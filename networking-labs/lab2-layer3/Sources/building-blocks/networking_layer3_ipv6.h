/**
 * @file networking_layer3_ipv6.h
 *
 * Networking layer 3 interface: IPv6
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV6_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV6_H_

#include <stdint.h>
#include <stddef.h>
#include "rtos_wrapper.h"
#include "io_utils.h"
#include "compile_time_checks.h"
#include "network_packet.h"

/**
 * Number of entries for the IPv6 Neighbor cache table
 */
#define NEIGHBOR_CACHE_NUM_ENTRIES  16

/**
 * Timeout in milliseconds to wait for a neighbor advertisement after sending a
 * non-gratuitous neighbor solicitation (3 minutes)
 */
#define NEIGHBOR_ADVERTISEMENT_WAIT_TIMEOUT_IN_MS	(3u * 60 * 1000)

/**
 * Maximum number of neighbor solicitations to be sent for a given destination
 * IPv6 address, before failing with "unreachable destination".
 */
#define NEIGHBOR_SOLICITATION_MAX_RETRIES	64

#define NET_MAX_IPV6_PACKET_PAYLOAD_SIZE \
        (NET_PACKET_DATA_BUFFER_SIZE - \
         (sizeof(struct ethernet_header) + sizeof(struct ipv6_header)))

/**
 * IPv6 address in network byte order
 */
struct ipv6_address {
    union {
	/**
	 * bytes[0] = most significant byte
	 * bytes[15] = less significant byte
	 */
	uint8_t bytes[16];

	/**
	 * Address seen as eight 16-bit half-words
	 */
        uint16_t hwords[8];

	/**
	 * Address seen as four 32-bit words
	 */
        uint32_t words[4];

	/**
	 * Address seen as two 64-bit double-words
	 */
	uint64_t dwords[2];
    };
}  __attribute__ ((aligned(sizeof(uint64_t))));

C_ASSERT(sizeof(struct ipv6_address) == sizeof(uint64_t) * 2);

/**
 * Header of an IPv6 packet in network byte order
 * (An IPv6 packet is encapsulated in an Ethernet frame)
 */
struct ipv6_header {
    /**
     * First 32-bit word:
     * - Top 4 bits of first byte is IP version, which is 6 for IPv6
     */
    union {
        uint32_t value;
        uint8_t bytes[4];
#       define IPv6_VERSION_MASK	    MULTI_BIT_MASK(7, 4)
#       define IPv6_VERSION_SHIFT	    4
    } first_word;

    /**
     * Payload length
     */
    uint16_t payload_length;

    /**
     * Next header type (values from enum l4_protocols)
     */
    uint8_t next_header;

    /**
     * Hop limit
     */
    uint8_t hop_limit;

    /**
     * Source (sender) IPv6 address
     */
    struct ipv6_address source_ipv6_addr;

    /**
     * Destination (receiver) IPv6 address
     */
    struct ipv6_address dest_ipv6_addr;
};

C_ASSERT(sizeof(struct ipv6_header) == 40);
C_ASSERT(offsetof(struct ipv6_header, source_ipv6_addr) % sizeof(uint64_t) == 0);
C_ASSERT(offsetof(struct ipv6_header, dest_ipv6_addr) % sizeof(uint64_t) == 0);

/**
 * IPv6 ICMPv6 header layout
 * (An ICMPv6 message is encapsulated in an IPv6 packet)
 */
struct icmpv6_header {
    /**
     * Message type
     */
    uint8_t msg_type;
#   define ICMPV6_TYPE_ECHO_REQ                  128
#   define ICMPV6_TYPE_ECHO_REPLY                129
#   define ICMPV6_TYPE_MULTICAST_LISTENER_QUERY  130
#   define ICMPV6_TYPE_MULTICAST_LISTENER_REPORT 131
#   define ICMPV6_TYPE_MULTICAST_LISTENER_DONE   132
#   define ICMPV6_TYPE_ROUTER_SOLICITATION       133
#   define ICMPV6_TYPE_ROUTER_ADVERTISEMENT      134
#   define ICMPV6_TYPE_NEIGHBOR_SOLICITATION     135
#   define ICMPV6_TYPE_NEIGHBOR_ADVERTISEMENT    136
#   define ICMPV6_TYPE_REDIRECT                  137

    /**
     * Message code
     */
    uint8_t msg_code;

    /**
     * Message checksum
     */
    uint16_t msg_checksum;
}; //  __attribute__((packed));

C_ASSERT(sizeof(struct icmpv6_header) == 4);

/**
 * ICMPv6 neighbor solicitation message layout
 */
struct icmpv6_neighbor_solicitation {
    struct icmpv6_header header;
    uint32_t reserved; /* = 0 */
    struct ipv6_address target_ip_addr;
    uint16_t options[];
};

C_ASSERT(sizeof(struct icmpv6_neighbor_solicitation) == 24);
C_ASSERT(offsetof(struct icmpv6_neighbor_solicitation, target_ip_addr) == 8);
C_ASSERT(offsetof(struct icmpv6_neighbor_solicitation, options) == 24);

/**
 * ICMPv6 neighbor advertisement message layout
 */
struct icmpv6_neighbor_advertisement {
    struct icmpv6_header header;
    union {
        uint32_t reserved_and_flags;
        struct {
            uint8_t reserved[3];
            uint8_t flags;
#           define NEIGHBOR_ADVERT_FLAG_R   BIT(0)
#           define NEIGHBOR_ADVERT_FLAG_S   BIT(1)
#           define NEIGHBOR_ADVERT_FLAG_O   BIT(2)
        };
    };

    struct ipv6_address target_ip_addr;
    uint16_t options[];
};

C_ASSERT(sizeof(struct icmpv6_neighbor_advertisement) == 24);
C_ASSERT(offsetof(struct icmpv6_neighbor_advertisement, target_ip_addr) == 8);
C_ASSERT(offsetof(struct icmpv6_neighbor_advertisement, options) == 24);

/**
 * ICMPv6 echo request/reply message layout
 */
struct icmpv6_echo_message {
	struct icmpv6_header header;
	uint16_t identifier;
	uint16_t seq_num;
};

/**
 * States of an entry in the neighbor discovery cache
 */
enum neighbor_cache_entry_states {
    NEIGHBOR_ENTRY_INVALID = 0,
    NEIGHBOR_ENTRY_INCOMPLETE,
    NEIGHBOR_ENTRY_REACHABLE,
    NEIGHBOR_ENTRY_STALE,
    NEIGHBOR_ENTRY_DELAY,
    NEIGHBOR_ENTRY_PROBE,
};

/**
 * IPv6 neighbor cache entry
 */
struct neighbor_cache_entry {
    struct ipv6_address dest_ipv6_addr;
    struct ethernet_mac_address dest_mac_addr;
    enum neighbor_cache_entry_states state;

    /**
     * Timestamp in ticks when the last neighbor solicitation for this entry was sent.
     * It is used to determine if we have waited too long for the neighbor advertisement,
     * and need to send another neighbor solicitation.
     */
    uint32_t neighbor_solicitation_time_stamp;

    /**
     * Timestamp in ticks when the last lookup was done for this entry. It is
     * used to determine the least recently used entry, for cache entry
     * replacement.
     */
    uint32_t last_lookup_time_stamp;
};

/**
 * IPv6 Neighbor cache
 */
struct neighbor_cache {
    /**
     * Mutex to serialize access to the Neighbor cache
     */
    struct rtos_mutex mutex;

    /**
     * Condvar signaled when the neighbor cache is updated
     */
    struct rtos_semaphore cache_updated_semaphore;

    /**
     * Array of cache entries
     */
    struct neighbor_cache_entry entries[NEIGHBOR_CACHE_NUM_ENTRIES];
};

/**
 * IPv6 network end point
 */
struct ipv6_end_point {
    /**
     * Link-local unicast IPv6 address
     */
    struct ipv6_address link_local_ip_addr;

    /**
     * Inteface Id (modified EUI-64 Id)
     */
    uint64_t interface_id;

    /**
     * Flags
     */
    volatile uint32_t flags;
#   define IPV6_DETECT_DUP_ADDR             BIT(0)
#   define IPV6_DUP_ADDR_DETECTED           BIT(1)
#   define IPV6_LINK_LOCAL_ADDR_READY       BIT(2)

    /**
     * Mutex to serialize access to this object
     */
    struct rtos_mutex mutex;

    /**
     * Condvar signaled when 'flags' get changed
     */
    struct rtos_semaphore flags_changed_semaphore;

    /**
     * Queue of received ICMPv6 packets
     */
    struct net_packet_queue rx_icmpv6_packet_queue;

    /**
     * Neighbor cache
     */
    struct neighbor_cache neighbor_cache;

    /**
     * IPv6 address autoconfiguration task
     */
    struct rtos_task address_autoconfiguration_task;

    /**
     * ICMPv6 packet receiver task
     */
    struct rtos_task icmpv6_packet_receiver_task;
};

/**
 * Networking layer-3 for IPv6 - global state variables
 */
struct net_layer3_ipv6 {
    /**
     * Flag indicating if there is an outstanding IPv6 ping request for which
     * no reply has been received yet
     */
    volatile bool expecting_ping_ipv6_reply;

    /**
     * Number of IPv6 packets received
     */
    uint32_t received_ipv6_packets_count;

    /**
     * Queue of received IPPv6 ping replies
     */
    struct net_packet_queue rx_ipv6_ping_reply_packet_queue;

    /**
     * Mutex to serialize access to expecting_ping_ipv6_reply
     */
    struct rtos_mutex expecting_ping_ipv6_reply_mutex;

    /**
     * Semaphore to be signal when the ping IPv6 reply for an outstanding ping
     * request has been received.
     */
    struct rtos_semaphore ping_ipv6_reply_received_semaphore;
};

void net_layer3_ipv6_init(struct net_layer3_ipv6 *layer3_ipv6_p);

void net_layer3_ipv6_end_point_init(struct ipv6_end_point *ipv6_end_point_p);

void net_layer3_ipv6_end_point_start_tasks(struct ipv6_end_point *ipv6_end_point_p);

void net_layer3_receive_ipv6_packet(struct network_packet *rx_packet_p);

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER3_IPV6_H_ */
