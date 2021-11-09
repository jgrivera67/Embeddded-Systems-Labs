/**
 * @file network_packet.h
 *
 * Network packet services interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORK_PACKET_H_
#define SOURCES_BUILDING_BLOCKS_NETWORK_PACKET_H_

#include <stdint.h>
#include <stdbool.h>
#include "mem_utils.h"
#include "compile_time_checks.h"
#include "runtime_checks.h"
#include "rtos_wrapper.h"
#include "io_utils.h"

/**
 * Maximum transfer unit for Ethernet (frame size without CRC)
 */
#define ETHERNET_MAX_FRAME_DATA_SIZE    1500

/**
 * Maximum Ethernet frame size (in bytes) including CRC
 * (without using VLAN tag)
 */
#define ETHERNET_MAX_FRAME_SIZE        (ETHERNET_MAX_FRAME_DATA_SIZE + 18)

/**
 * Network packet data buffer alignment in bytes
 * (minimum 16-byte alignment required by the ENET device)
 */
#define NET_PACKET_DATA_BUFFER_ALIGNMENT    UINT32_C(16)

/**
 * Network packet data buffer size rounded-up to the required alignment
 */
#define NET_PACKET_DATA_BUFFER_SIZE \
    ROUND_UP(ETHERNET_MAX_FRAME_SIZE, NET_PACKET_DATA_BUFFER_ALIGNMENT)

/**
 * Maximum number of Tx packet buffers
 */
#define NET_MAX_TX_PACKETS   8

/**
 * Maximum number of Rx packet buffers per layer-2 end point
 */
#define NET_MAX_RX_PACKETS   8

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
 * Copies a MAC address. The source and destination must be at least
 * 2-byte aligned
 */
#define COPY_MAC_ADDRESS(_dest_p, _src_p) \
	do {								\
	    (_dest_p)->hwords[0] = (_src_p)->hwords[0];			\
	    (_dest_p)->hwords[1] = (_src_p)->hwords[1];			\
	    (_dest_p)->hwords[2] = (_src_p)->hwords[2];			\
	} while (0)

/**
 * Compares two MAC addresses. Their storage must be at least 2-byte aligned
 */
#define MAC_ADDRESSES_EQUAL(_mac_addr1_p, _mac_addr2_p) \
	 ((_mac_addr1_p)->hwords[0] == (_mac_addr2_p)->hwords[0] &&	\
	  (_mac_addr1_p)->hwords[1] == (_mac_addr2_p)->hwords[1] &&	\
	  (_mac_addr1_p)->hwords[2] == (_mac_addr2_p)->hwords[2])

/**
 * Check that a network packet is not in a queue
 */
#define NET_PACKET_NOT_IN_QUEUE(_net_packet_p) \
		((_net_packet_p)->queue_p == NULL && (_net_packet_p)->next_p == NULL)

/**
 * Network packet object
 */
struct network_packet {
	/*
	 * Signature for run-time type checking
	 */
#   define NET_TX_PACKET_SIGNATURE  GEN_SIGNATURE('T', 'X', 'B', 'U')
#   define NET_RX_PACKET_SIGNATURE  GEN_SIGNATURE('R', 'X', 'B', 'U')
    uint32_t signature;

    /**
     * Ethernet MAC buffer descriptor associated with the packet
     */
    union {
        volatile struct ethernet_tx_buffer_descriptor *tx_buf_desc_p;
        volatile struct ethernet_rx_buffer_descriptor *rx_buf_desc_p;
    };

    /**
     * Pointer to the local layer-2 end point that owns this network packet
     * object. Only meaningful for Rx packets.
     */
    struct net_layer2_end_point *layer2_end_point_p;

    uint16_t state_flags;
#   define NET_PACKET_IN_TX_TRANSIT             BIT(0)
#   define NET_PACKET_IN_RX_TRANSIT             BIT(1)
#   define NET_PACKET_IN_TX_USE_BY_APP          BIT(2)
#   define NET_PACKET_IN_RX_USE_BY_APP          BIT(3)
#   define NET_PACKET_FREE_AFTER_TX_COMPLETE    BIT(4)
#   define NET_PACKET_IN_RX_QUEUE               BIT(5)
#   define NET_PACKET_RX_FAILED                 BIT(6)
#   define NET_PACKET_IN_TX_POOL                BIT(7)
#   define NET_PACKET_IN_ICMP_QUEUE             BIT(8)
#   define NET_PACKET_IN_ICMPV6_QUEUE           BIT(9)

    /**
     * Total packet length, including L2 L3 and L4 headers
     */
    uint16_t total_length;

    /**
     * Pointer to the packet queue in which this packet is currently queued
     * or NULL if none.
     */
    struct net_packet_queue *queue_p;

    /**
     * Pointer to the next network packet in the same packet queue
     * in which this packet is currently queued. This field is
     * meaningful only if queue_p is not NULL.
     */
    struct network_packet *next_p;

    /*
     * Packet payload data buffer
     */
    uint8_t data_buffer[NET_PACKET_DATA_BUFFER_SIZE]
        __attribute__ ((aligned(NET_PACKET_DATA_BUFFER_ALIGNMENT)));
}  __attribute__ ((aligned(MPU_REGION_ALIGNMENT)));

C_ASSERT(sizeof(struct network_packet) % MPU_REGION_ALIGNMENT == 0);
C_ASSERT(sizeof(bool) == sizeof(uint8_t));
C_ASSERT(offsetof(struct network_packet, data_buffer) %
		 NET_PACKET_DATA_BUFFER_ALIGNMENT == 0);

/**
 * Get the network packet for a given data buffer
 */
#define BUFFER_TO_NETWORK_PACKET(_data_buf) \
    ((struct network_packet *) \
     ((uintptr_t)(_data_buf) - offsetof(struct network_packet, data_buffer)))

/**
 * Network packet queue
 */
struct net_packet_queue {
#   define NET_PACKET_QUEUE_SIGNATURE  GEN_SIGNATURE('N', 'P', 'Q', 'U')
    uint32_t signature;

    /**
     * Queue name (null-terminated string)
     */
    const char *name_p;

    /**
     * Flag indicating if serialization to access the queue is to be done
     * using a mutex (true), or by disabling interrupts (false)
     */
    bool use_mutex;

    /**
     * Number of elements in the queue
     */
    uint16_t length;

    /**
     * largest length that the queue has ever had
     */
    uint16_t length_high_water_mark;

    /**
     * Pointer to the first  packet in the queue or NULL if the queue is empty
     */
    struct network_packet *head_p;

    /**
     * Pointer to the last packet in the queue or NULL if the queue is empty
     */
    struct network_packet *tail_p;

    /**
     * Mutex to serialize access to the queue. It is only meaningful if
     * 'use_mutex' is true.
     */
    struct rtos_mutex mutex;

    /**
     * Counting semaphore to be signaled when a packet is added to the queue.
     */
    struct rtos_semaphore semaphore;
};


/**
 * Invert byte order of a 16-bit value
 */
static inline uint16_t byte_swap16(uint16_t value)
{
    uint16_t swapped_val;

    asm volatile (
            "rev16 %[swapped_val], %[value]\n\t"
            : [swapped_val] "=r" (swapped_val)
            : [value] "r" (value)
    );

    return swapped_val;
}


/**
 * Invert byte order of a 32-bit value
 */
static inline uint32_t byte_swap32(uint32_t value)
{
    uint32_t swapped_val;

    asm volatile (
            "rev %[swapped_val], %[value]\n\t"
            : [swapped_val] "=r" (swapped_val)
            : [value] "r" (value)
    );

    return swapped_val;
}


void net_packet_queue_init(const char *name_p,
                           bool use_mutex,
                           struct net_packet_queue *queue_p);

void net_packet_queue_add(struct net_packet_queue *queue_p,
                          struct network_packet *packet_p);

struct network_packet *net_packet_queue_remove(struct net_packet_queue *queue_p,
                                               uint32_t timeout_ms);

#endif /* SOURCES_BUILDING_BLOCKS_NETWORK_PACKET_H_ */
