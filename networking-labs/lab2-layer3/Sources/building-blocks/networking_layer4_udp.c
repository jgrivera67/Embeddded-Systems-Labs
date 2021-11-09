/**
 * @file networking_layer4_udp.c
 *
 * Networking layer 4 implementation: UDP
 *
 * @author German Rivera
 */
#include "networking_layer4.h"
#include "runtime_checks.h"
#include "runtime_log.h"

/**
 * Initializes Networking layer-4 for UDP
 *
 * @param layer4_udp_p: Pointer to UDP-specific networking layer-4
 */
void net_layer4_udp_init(struct net_layer4_udp *layer4_udp_p)
{
    layer4_udp_p->next_ephemeral_port = NET_FIRST_EPHEMERAL_PORT;
    net_layer4_end_point_list_init(&layer4_udp_p->local_udp_end_point_list,
                                   NET_LAYER4_UDP);

    rtos_mutex_init(&layer4_udp_p->mutex, "layer-4 UDP mutex");
}


/**
 * Initializes a UDP end point
 *
 * @param layer4_end_point_p    Pointer to UDP end point to initialize
 */
void net_layer4_udp_end_point_init(struct net_layer4_end_point *layer4_end_point_p)
{
    net_layer4_end_point_init(layer4_end_point_p, NET_LAYER4_UDP);
}


/**
 * Binds a UDP end point to a given UDP port number
 *
 * @param layer4_end_point_p    Pointer to UDP end point to bind
 * @param udp_port              UDP port number to use, or 0 if an ephemeral
 *                              port is to be chosen.
 *
 * @return 0, on success
 * @return error code, on failure
 */
error_t net_layer4_udp_end_point_bind(
    struct net_layer4_end_point *layer4_end_point_p,
    uint16_t udp_port /* big endian */)
{
    error_t error;

    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer4,
                                sizeof g_net_layer4,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(layer4_end_point_p, sizeof *layer4_end_point_p,
                               0);
#   endif

    D_ASSERT(g_net_layer4.initialized);
    D_ASSERT(layer4_end_point_p->protocol == NET_LAYER4_UDP);
    D_ASSERT(layer4_end_point_p->layer4_port == 0);

    struct net_layer4_udp *layer4_udp_p = &g_net_layer4.udp;

    rtos_mutex_lock(&layer4_udp_p->mutex);

    if (udp_port == 0) {
        if (layer4_udp_p->next_ephemeral_port == 0) {
            error = CAPTURE_ERROR("No more UDP ephemeral ports available",
                                  0, 0);
            goto common_exit;
        }

        udp_port = hton16(layer4_udp_p->next_ephemeral_port);
        layer4_udp_p->next_ephemeral_port ++;
    } else {
        if (udp_port >= NET_FIRST_EPHEMERAL_PORT) {
            error = CAPTURE_ERROR("Non-zero UDP port cannot be in the ephemeral ports range",
                                  udp_port, layer4_end_point_p);
            goto common_exit;
        }

        struct net_layer4_end_point *existing_udp_end_point_p =
            net_layer4_end_point_list_lookup(&layer4_udp_p->local_udp_end_point_list, udp_port);

        if (existing_udp_end_point_p != NULL) {
            error = CAPTURE_ERROR("UDP port already in use", udp_port,
                                  existing_udp_end_point_p);
            goto common_exit;
        }
    }

    layer4_end_point_p->layer4_port = udp_port;
    net_layer4_end_point_list_add(&layer4_udp_p->local_udp_end_point_list,
                                   layer4_end_point_p);
    error = 0;

common_exit:
    rtos_mutex_unlock(&layer4_udp_p->mutex);

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return error;
}


/**
 * Unbinds a UDP end point from a given UDP port number
 *
 * @param layer4_end_point_p    Pointer to UDP end point to bind
 */
void net_layer4_udp_end_point_unbind(struct net_layer4_end_point *layer4_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer4,
                                sizeof g_net_layer4,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(layer4_end_point_p, sizeof *layer4_end_point_p,
                               0);
#   endif

    D_ASSERT(g_net_layer4.initialized);
    D_ASSERT(layer4_end_point_p->protocol == NET_LAYER4_UDP);

    struct net_layer4_udp *layer4_udp_p = &g_net_layer4.udp;

    rtos_mutex_lock(&layer4_udp_p->mutex);

    net_layer4_end_point_list_remove(&layer4_udp_p->local_udp_end_point_list,
                                     layer4_end_point_p);

    layer4_end_point_p->layer4_port = 0; /* unbound */
    rtos_mutex_unlock(&layer4_udp_p->mutex);

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


error_t net_layer4_send_udp_datagram_over_ipv4(
    struct net_layer4_end_point *layer4_end_point_p,
    const struct ipv4_address *dest_ip_addr_p,
    uint16_t dest_port, /* big endian */
    struct network_packet *tx_packet_p,
    size_t data_payload_length)
{
    error_t error;

    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer4,
                                sizeof g_net_layer4,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(layer4_end_point_p, sizeof *layer4_end_point_p,
                               0);
#   endif

    D_ASSERT(g_net_layer4.initialized);
    D_ASSERT(layer4_end_point_p->protocol == NET_LAYER4_UDP);
    D_ASSERT(data_payload_length <= NET_MAX_IPV4_UDP_PACKET_PAYLOAD_SIZE);

    /*
     * Populate UDP header:
     */

    struct udp_header *udp_header_p =
        (struct udp_header *)GET_IPV4_DATA_PAYLOAD_AREA(tx_packet_p);

    udp_header_p->source_port = layer4_end_point_p->layer4_port;
    udp_header_p->dest_port = dest_port;
    udp_header_p->datagram_length = hton16(sizeof(struct udp_header) +
                                           data_payload_length);

    /*
     * NOTE: udp_header_p->datagram_checksum is filled by the Ethernet MAC
     * hardware. We just need to initialize it to 0.
     */
    udp_header_p->datagram_checksum = 0;

    if (g_net_layer4.tracing_on) {
        DEBUG_PRINTF("Net layer4: UDP datagram sent: "
                     "source port %u, destination port %u, length %u\n",
                     ntoh16(udp_header_p->source_port),
                     ntoh16(udp_header_p->dest_port),
                     ntoh16(udp_header_p->datagram_length));
    }

    /*
     * Send IP packet:
     */
    error = net_layer3_send_ipv4_packet(dest_ip_addr_p,
                                        tx_packet_p,
                                        sizeof(struct udp_header) +
                                            data_payload_length,
                                        IP_PACKET_TYPE_UDP);

    ATOMIC_POST_INCREMENT_UINT32(&g_net_layer4.udp.sent_packets_over_ipv4_count);

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return error;
}


error_t net_layer4_receive_udp_datagram_over_ipv4(
    struct net_layer4_end_point *layer4_end_point_p,
    uint32_t timeout_ms,
    struct ipv4_address *source_ip_addr_p,
    uint16_t *source_port_p,
    struct network_packet **rx_packet_pp)
{
    error_t error;

    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer4,
                                sizeof g_net_layer4,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(layer4_end_point_p, sizeof *layer4_end_point_p,
                               0);
#   endif

    D_ASSERT(g_net_layer4.initialized);
    D_ASSERT(layer4_end_point_p->protocol == NET_LAYER4_UDP);

    struct network_packet *rx_packet_p =
        net_packet_queue_remove(&layer4_end_point_p->rx_packet_queue,
                                timeout_ms);

    if (rx_packet_p == NULL) {
        *rx_packet_pp = NULL;
        error = CAPTURE_ERROR("No Rx packet available", timeout_ms, 0);
        goto common_exit;
    }

    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(rx_packet_p);

    D_ASSERT(ipv4_header_p->protocol_type == IP_PACKET_TYPE_UDP);

    struct udp_header *udp_header_p =
        (struct udp_header *)GET_IPV4_DATA_PAYLOAD_AREA(rx_packet_p);

    D_ASSERT(udp_header_p->dest_port == layer4_end_point_p->layer4_port);

    source_ip_addr_p->value = ipv4_header_p->source_ip_addr.value;
    *source_port_p = udp_header_p->source_port;
    *rx_packet_pp = rx_packet_p;
    error = 0;

common_exit:
#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return error;
}


/**
 * Lookup local UDP end point bound to a given UDP port number
 *
 * @param layer4_udp_p    Pointer to UDP layer
 * @param udp_port        UDP port number
 *
 * @return Pointer to wanted UDP end point, if found
 * @return NULL, otherwise
 */
static struct net_layer4_end_point *
lookup_local_udp_end_point(struct net_layer4_udp *layer4_udp_p, uint16_t udp_port)
{
    struct net_layer4_end_point *layer4_end_point_p;

    rtos_mutex_lock(&layer4_udp_p->mutex);

    layer4_end_point_p =
        net_layer4_end_point_list_lookup(&layer4_udp_p->local_udp_end_point_list,
                                         udp_port);

    rtos_mutex_unlock(&layer4_udp_p->mutex);
    return layer4_end_point_p;
}


void net_layer4_process_incoming_udp_datagram(struct network_packet *rx_packet_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
        struct mpu_region_range old_comp_region;

        rtos_thread_set_comp_region(&g_net_layer4,
                                    sizeof g_net_layer4,
                                    0,
                                    &old_comp_region);

        rtos_thread_set_tmp_region(rx_packet_p, sizeof *rx_packet_p, 0);
#   endif

    D_ASSERT(g_net_layer4.initialized);

    struct net_layer4_udp *layer4_udp_p = &g_net_layer4.udp;

    D_ASSERT(rx_packet_p->total_length >=
               sizeof(struct ethernet_header) + sizeof(struct ipv4_header) +
               sizeof(struct udp_header));

    struct udp_header *udp_header_p = GET_IPV4_DATA_PAYLOAD_AREA(rx_packet_p);

    if (g_net_layer4.tracing_on) {
        DEBUG_PRINTF("Net layer4: UDP datagram received: "
                     "source port %u, destination port %u, length %u\n",
                     ntoh16(udp_header_p->source_port),
                     ntoh16(udp_header_p->dest_port),
                     ntoh16(udp_header_p->datagram_length));
    }

    /*
     * Lookup local UDP end point by destination port:
     */
    struct net_layer4_end_point *layer4_end_point_p =
        lookup_local_udp_end_point(layer4_udp_p, udp_header_p->dest_port);

    if (layer4_end_point_p != NULL) {
        net_packet_queue_add(&layer4_end_point_p->rx_packet_queue, rx_packet_p);
        ATOMIC_POST_INCREMENT_UINT32(&g_net_layer4.udp.rx_packets_accepted_count);
    } else {
        ERROR_PRINTF("Received UDP datagram ignored: unknown port %u\n",
                     ntoh16(udp_header_p->dest_port));

        net_recycle_rx_packet(rx_packet_p);
        ATOMIC_POST_INCREMENT_UINT32(&g_net_layer4.udp.rx_packets_dropped_count);
    }

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}
