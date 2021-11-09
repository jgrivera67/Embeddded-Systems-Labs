/**
 * @file networking_layer4_udp.h
 *
 * Networking layer 4 interface: UDP
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_UDP_H_
#define SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_UDP_H_

#include "networking_layer3.h"
#include "net_layer4_end_point.h"
#include "rtos_wrapper.h"

/**
 * Maximum size of the data payload of a UDP datagram over IPv4
 */
#define NET_MAX_IPV4_UDP_PACKET_PAYLOAD_SIZE \
        (NET_MAX_IPV4_PACKET_PAYLOAD_SIZE - sizeof(struct udp_header))

/**
 * Maximum size of the data payload of a UDP datagram over IPv6
 */
#define NET_MAX_IPV6_UDP_PACKET_PAYLOAD_SIZE \
        (NET_MAX_IPV6_PACKET_PAYLOAD_SIZE - sizeof(struct udp_header))

/**
 * UDP header layout
 * (A UDP datagram is encapsulated in an IP packet)
 */
struct udp_header {
    /**
     * Source port number
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t source_port;

    /**
     * Destination port number
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t dest_port;

    /**
     * UDP datagram length (header + data payload) in bytes
     * (hton16() must be invoked before writing this field.
     *  ntoh16() must be invoked after reading this field.)
     */
    uint16_t datagram_length;

    /**
     * UDP datagram checksum
     */
    uint16_t datagram_checksum;
}; //  __attribute__((packed));

C_ASSERT(sizeof(struct udp_header) == 8);

/**
 * Networking layer-4 for UDP
 */
struct net_layer4_udp {
	/**
	 * Next ephemeral port to assign to a local UDP end point.
	 */
	uint16_t next_ephemeral_port;

	/**
	 * Number of received UDP datagrams accepted
	 */
	volatile uint32_t rx_packets_accepted_count;

	/**
	 * Number of received UDP datagrams dropped
	 */
	volatile uint32_t rx_packets_dropped_count;

	/**
	 * Number of UDP datagrams sent over IPv4
	 */
	volatile uint32_t sent_packets_over_ipv4_count;

	/**
	 * List of existing local UDP end points
     */
	struct net_layer4_end_point_list local_udp_end_point_list;

    /**
     * Mutex to serialize access to this struct
     */
    struct rtos_mutex mutex;
};


/**
 * Returns pointer to the data payload area of an IPv4 UDP datagram
 */
static inline void *get_ipv4_udp_data_payload_area(
    struct network_packet *net_packet_p)
{
    if (net_packet_p->signature == NET_RX_PACKET_SIGNATURE) {
    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(net_packet_p);
        uint_fast8_t ip_version = GET_IP_VERSION(ipv4_header_p);

    D_ASSERT(ip_version == 4);
    D_ASSERT(ipv4_header_p->protocol_type == IP_PACKET_TYPE_UDP);
    } else {
        D_ASSERT(net_packet_p->signature == NET_TX_PACKET_SIGNATURE);
    }

    return (void *)((uint8_t *)GET_IPV4_DATA_PAYLOAD_AREA(net_packet_p) +
            sizeof(struct udp_header));
}


/**
 * Returns the data payload length of an IPv4 incoming UDP datagram
 */
static inline size_t get_ipv4_udp_data_payload_length(
    struct network_packet *net_packet_p)
{
    D_ASSERT(net_packet_p->signature == NET_RX_PACKET_SIGNATURE);

    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(net_packet_p);
    uint_fast8_t ip_version = GET_IP_VERSION(ipv4_header_p);

    D_ASSERT(ip_version == 4);
    D_ASSERT(ipv4_header_p->protocol_type == IP_PACKET_TYPE_UDP);

    struct udp_header *udp_header_p =
        (struct udp_header *)GET_IPV4_DATA_PAYLOAD_AREA(net_packet_p);

    return ntoh16(udp_header_p->datagram_length) - sizeof(struct udp_header);
}


void net_layer4_udp_init(struct net_layer4_udp *layer4_udp_p);

void net_layer4_udp_end_point_init(struct net_layer4_end_point *layer4_end_point_p);

error_t net_layer4_udp_end_point_bind(
	struct net_layer4_end_point *layer4_end_point_p,
    uint16_t layer4_port /* big endian */);

void net_layer4_udp_end_point_unbind(struct net_layer4_end_point *layer4_end_point_p);

error_t net_layer4_send_udp_datagram_over_ipv4(
    struct net_layer4_end_point *layer4_end_point_p,
    const struct ipv4_address *dest_ip_addr_p,
    uint16_t dest_port, /* big endian */
    struct network_packet *tx_packet_p,
    size_t data_payload_length);

error_t net_layer4_receive_udp_datagram_over_ipv4(
    struct net_layer4_end_point *layer4_end_point_p,
    uint32_t timeout_ms,
    struct ipv4_address *source_ip_addr_p,
    uint16_t *source_port_p,
    struct network_packet **rx_packet_pp);

void net_layer4_process_incoming_udp_datagram(struct network_packet *rx_packet_p);

#endif /* SOURCES_BUILDING_BLOCKS_NETWORKING_LAYER4_UDP_H_ */
