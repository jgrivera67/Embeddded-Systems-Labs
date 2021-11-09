/**
 * @file networking_layer3_ipv4.c
 *
 * Networking layer 3 implementation: IPv4
 *
 * @author German Rivera
 */
#include "networking_layer3.h"
#include "ethernet_mac.h"
#include "networking_layer4.h"
#include "runtime_log.h"
#include <string.h>
#include <stdlib.h>

/**
 * DHCP client port
 */
#define DHCP_UDP_CLIENT_PORT    68

/**
 * DHCP server port
 */
#define DHCP_UDP_SERVER_PORT    67

/**
 * Initializes Networking layer-3 for IPv4
 *
 * @param layer3_ipv4_p: Pointer to IPv4 networking layer-3
 */
void net_layer3_ipv4_init(struct net_layer3_ipv4 *layer3_ipv4_p)
{
    net_packet_queue_init("Incoming IPv4 ping reply packet queue",
                          true,
                          &layer3_ipv4_p->rx_ipv4_ping_reply_packet_queue);


    rtos_mutex_init(&layer3_ipv4_p->expecting_ping_reply_mutex,
                    "expecting_ping_reply mutex");

    rtos_semaphore_init(&layer3_ipv4_p->ping_reply_received_semaphore,
                        "ping_reply_received semaphore",
                        0);
}


/**
 * Chooses the local network end-point to be used for sending a packet,
 * based on the destination IPv4 address
 */
static struct net_layer3_end_point *
choose_ipv4_local_layer3_end_point(const struct ipv4_address *dest_ip_addr_p)
{
    /*
     * There is only one local layer-3 end-point
     */
    return &g_net_layer3.local_layer3_end_points[0];
}


static void net_send_ipv4_dhcp_discovery(
    struct net_layer3_end_point *layer3_end_point_p,
    struct net_layer4_end_point *client_end_point_p)
{
    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);

    struct dhcp_message *dhcp_discovery_msg_p =
        get_ipv4_udp_data_payload_area(tx_packet_p);

    struct ethernet_mac_address local_mac_address;

    net_layer2_get_mac_addr(layer3_end_point_p->layer2_end_point_p, &local_mac_address);

    dhcp_discovery_msg_p->op = 0x1;
    dhcp_discovery_msg_p->hardware_type = 0x1; /* Ethernet */
    dhcp_discovery_msg_p->hw_addr_len = 0x6;
    dhcp_discovery_msg_p->hops = 0x0;
    dhcp_discovery_msg_p->transaction_id = rtos_get_ticks_since_boot();
    dhcp_discovery_msg_p->seconds = 0x0;
    dhcp_discovery_msg_p->flags = 0x0;
    dhcp_discovery_msg_p->client_ip_addr.value = IPV4_NULL_ADDR;
    dhcp_discovery_msg_p->your_ip_addr.value = IPV4_NULL_ADDR;
    dhcp_discovery_msg_p->next_server_ip_addr.value = IPV4_NULL_ADDR;
    dhcp_discovery_msg_p->relay_agent_ip_addr.value = IPV4_NULL_ADDR;
    COPY_MAC_ADDRESS(&dhcp_discovery_msg_p->client_mac_addr,
             &local_mac_address);

    bzero(dhcp_discovery_msg_p->zero_filled,
      sizeof dhcp_discovery_msg_p->zero_filled);

    dhcp_discovery_msg_p->magic_cookie = 0x63825363; /* DHCP */
    dhcp_discovery_msg_p->options[0] = 53; /* option 1 type: DHCP message type */
    dhcp_discovery_msg_p->options[1] = 1; /* option length */
    dhcp_discovery_msg_p->options[2] = 0x01; /* option value: DHCPDISCOVER */

    dhcp_discovery_msg_p->options[3] = 55; /* option 2 type: parameter request list */
    dhcp_discovery_msg_p->options[4] = 11; /* option length */
    dhcp_discovery_msg_p->options[5] = 0x01; /* option value[0]: subnet mask */
    dhcp_discovery_msg_p->options[6] = 0x1c; /* option value[1]: broadcast address */
    dhcp_discovery_msg_p->options[7] = 0x02; /* option value[2]: time offset */
    dhcp_discovery_msg_p->options[8] = 0x03; /* option value[3]: router */
    dhcp_discovery_msg_p->options[9] = 0x0f; /* option value[4]: domain name */
    dhcp_discovery_msg_p->options[10] = 0x06; /* option value[5]: domain name server */
    dhcp_discovery_msg_p->options[11] = 0x77; /* option value[6]: domain search */
    dhcp_discovery_msg_p->options[12] = 0x0c; /* option value[7]: host name */
    dhcp_discovery_msg_p->options[13] = 0x1a; /* option value[8]: interface MTU */
    dhcp_discovery_msg_p->options[14] = 0x79; /* option value[9]: classless static route */
    dhcp_discovery_msg_p->options[15] = 0x2a; /* option value[10]: network time protocol servers */

    struct ipv4_address dest_ip_addr = { .value = IPV4_BROADCAST_ADDR };

    net_layer4_send_udp_datagram_over_ipv4(client_end_point_p,
                                           &dest_ip_addr,
                                           hton16(DHCP_UDP_SERVER_PORT),
                                           tx_packet_p,
                                           sizeof(struct dhcp_message) + 16);

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: DHCP client sent discovery message\n");
    }
}


/**
 * Maps multicast IPv4 address to multicast Ethernet MAC address
 */
static inline void map_ipv4_multicast_addr_to_ethernet_multicast_addr(
    const struct ipv4_address *ipv4_multicast_addr_p,
    struct ethernet_mac_address *ethernet_multicast_addr_p)
{
    D_ASSERT(IPV4_ADDR_IS_MULTICAST(ipv4_multicast_addr_p));

    ethernet_multicast_addr_p->bytes[0] = 0x01;
    ethernet_multicast_addr_p->bytes[1] = 0x00;
    ethernet_multicast_addr_p->bytes[2] = 0x5e;
    ethernet_multicast_addr_p->bytes[3] = (ipv4_multicast_addr_p->bytes[1] & 0x7f);
    ethernet_multicast_addr_p->hwords[2] = ipv4_multicast_addr_p->hwords[1];
}


static void
join_ipv4_multicast_group(struct net_layer3_end_point *layer3_end_point_p,
                          const struct ipv4_address *multicast_addr_p)
{
    struct ethernet_mac_address enet_multicast_addr;
    struct net_layer2_end_point *layer2_end_point_p =
        layer3_end_point_p->layer2_end_point_p;

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);

    map_ipv4_multicast_addr_to_ethernet_multicast_addr(multicast_addr_p,
                                                       &enet_multicast_addr);

    ethernet_mac_add_multicast_addr(layer2_end_point_p->ethernet_mac_p,
                                    &enet_multicast_addr);
}


void net_layer3_join_ipv4_multicast_group(const struct ipv4_address *multicast_addr_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);
#   endif

    join_ipv4_multicast_group(&g_net_layer3.local_layer3_end_points[0],
                              multicast_addr_p);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


static void arp_cache_init(struct arp_cache *arp_cache_p)
{
    rtos_mutex_init(&arp_cache_p->mutex, "ARP cache mutex");

    rtos_semaphore_init(&arp_cache_p->cache_updated_semaphore,
                        "ARP cache updated semaphore",
                        0);

    for (unsigned int i = 0; i < ARP_CACHE_NUM_ENTRIES; i++) {
        struct arp_cache_entry *entry_p = &arp_cache_p->entries[i];

        entry_p->state = ARP_ENTRY_INVALID;
    }
}


/**
 * Send an ARP request message
 */
static void
net_send_arp_request(const struct net_layer2_end_point *layer2_end_point_p,
                     const struct ipv4_address *source_ip_addr_p,
                     const struct ipv4_address *dest_ip_addr_p)
{
    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);

    D_ASSERT(tx_packet_p != NULL);

    struct ethernet_frame *const tx_frame_p =
        (struct ethernet_frame *)tx_packet_p->data_buffer;

    struct arp_packet *const arp_packet_p = &tx_frame_p->arp_packet;
    struct ethernet_mac_address local_mac_address;

    net_layer2_get_mac_addr(layer2_end_point_p, &local_mac_address);

    arp_packet_p->link_addr_type = hton16(0x1);
    arp_packet_p->network_addr_type = hton16(FRAME_TYPE_IPv4_PACKET);
    arp_packet_p->link_addr_size = sizeof(struct ethernet_mac_address);
    arp_packet_p->network_addr_size = sizeof(struct ipv4_address);
    arp_packet_p->operation = hton16(ARP_REQUEST);

    COPY_MAC_ADDRESS(&arp_packet_p->source_mac_addr, &local_mac_address);
    COPY_UNALIGNED_IPv4_ADDRESS(&arp_packet_p->source_ip_addr, source_ip_addr_p);
    COPY_MAC_ADDRESS(&arp_packet_p->dest_mac_addr, &g_ethernet_null_mac_addr);
    COPY_UNALIGNED_IPv4_ADDRESS(&arp_packet_p->dest_ip_addr, dest_ip_addr_p);

    if (g_net_layer3.tracing_on) {
    	bool gratuitous_arp_req = (dest_ip_addr_p->value == source_ip_addr_p->value);

        DEBUG_PRINTF("Net layer3: %s request sent:\n"
                     "\tdestination IPv4 address %u.%u.%u.%u\n",
					 gratuitous_arp_req ? "gratuitous ARP" : "ARP",
                     arp_packet_p->dest_ip_addr.bytes[0],
                     arp_packet_p->dest_ip_addr.bytes[1],
                     arp_packet_p->dest_ip_addr.bytes[2],
                     arp_packet_p->dest_ip_addr.bytes[3]);
    }

    (void)net_layer2_send_ethernet_frame(layer2_end_point_p,
                                         &g_ethernet_broadcast_mac_addr,
                                         tx_packet_p,
                                         FRAME_TYPE_ARP_PACKET,
                                         sizeof(struct arp_packet));
}


/**
 * Send an ARP reply message
 */
static void net_send_arp_reply(
    const struct net_layer2_end_point *layer2_end_point_p,
    const struct ipv4_address *source_ip_addr_p,
    const struct ethernet_mac_address *dest_mac_addr_p,
    const struct ipv4_address *dest_ip_addr_p)
{
    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);

    D_ASSERT(tx_packet_p != NULL);

    struct ethernet_frame *const tx_frame_p =
        (struct ethernet_frame *)tx_packet_p->data_buffer;

    struct arp_packet *const arp_packet_p = &tx_frame_p->arp_packet;
    struct ethernet_mac_address local_mac_address;

    net_layer2_get_mac_addr(layer2_end_point_p, &local_mac_address);

    /*
     * NOTE: The Ethernet MAC hardware populates the source MAC address
     * automatically in an outgoing frame
     */
#if 0
    COPY_MAC_ADDRESS(&tx_frame->enet_header.source_mac_addr,
                     &local_mac_address);
#endif

    arp_packet_p->link_addr_type = hton16(0x1);
    arp_packet_p->network_addr_type = hton16(FRAME_TYPE_IPv4_PACKET);
    arp_packet_p->link_addr_size = sizeof(struct ethernet_mac_address);
    arp_packet_p->network_addr_size = sizeof(struct ipv4_address);
    arp_packet_p->operation = hton16(ARP_REPLY);

    COPY_MAC_ADDRESS(&arp_packet_p->source_mac_addr, &local_mac_address);
    COPY_UNALIGNED_IPv4_ADDRESS(&arp_packet_p->source_ip_addr, source_ip_addr_p);
    COPY_MAC_ADDRESS(&arp_packet_p->dest_mac_addr, dest_mac_addr_p);
    COPY_UNALIGNED_IPv4_ADDRESS(&arp_packet_p->dest_ip_addr, dest_ip_addr_p);

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: ARP reply sent:\n"
                     "\tsource IPv4 address %u.%u.%u.%u\n"
                     "\tdestination IPv4 address %u.%u.%u.%u\n",
                     arp_packet_p->source_ip_addr.bytes[0],
                     arp_packet_p->source_ip_addr.bytes[1],
                     arp_packet_p->source_ip_addr.bytes[2],
                     arp_packet_p->source_ip_addr.bytes[3],
                     arp_packet_p->dest_ip_addr.bytes[0],
                     arp_packet_p->dest_ip_addr.bytes[1],
                     arp_packet_p->dest_ip_addr.bytes[2],
                     arp_packet_p->dest_ip_addr.bytes[3]);
    }

    (void)net_layer2_send_ethernet_frame(layer2_end_point_p,
                                         dest_mac_addr_p,
                                         tx_packet_p,
                                         FRAME_TYPE_ARP_PACKET,
                                         sizeof(struct arp_packet));
}


static struct arp_cache_entry *
arp_cache_lookup_or_allocate(struct arp_cache *arp_cache_p,
                             const struct ipv4_address *dest_ip_addr_p,
                             struct arp_cache_entry **free_entry_pp)
{
    struct arp_cache_entry *first_free_entry_p = NULL;
    struct arp_cache_entry *least_recently_used_entry_p = NULL;
    uint32_t least_recently_used_ticks_delta = 0;
    struct arp_cache_entry *matching_entry_p = NULL;

    *free_entry_pp = NULL;
    for (unsigned int i = 0; i < ARP_CACHE_NUM_ENTRIES; i++) {
        struct arp_cache_entry *entry_p = &arp_cache_p->entries[i];
        uint32_t current_ticks = rtos_get_ticks_since_boot();

        if (entry_p->state == ARP_ENTRY_INVALID) {
            if (first_free_entry_p == NULL) {
                first_free_entry_p = entry_p;
            }
        } else {
            D_ASSERT(entry_p->state == ARP_ENTRY_FILLED ||
                     entry_p->state == ARP_ENTRY_HALF_FILLED);

            if (entry_p->dest_ip_addr.value == dest_ip_addr_p->value) {
                matching_entry_p = entry_p;
                break;
            }

            if (least_recently_used_entry_p == NULL ||
                RTOS_TICKS_DELTA(entry_p->last_lookup_time_stamp,
                                 current_ticks) >
                   least_recently_used_ticks_delta) {
                least_recently_used_entry_p = entry_p;
                least_recently_used_ticks_delta =
                RTOS_TICKS_DELTA(entry_p->last_lookup_time_stamp,
                                 current_ticks);
            }
        }
    }

    if (matching_entry_p == NULL) {
        if (first_free_entry_p != NULL) {
            *free_entry_pp = first_free_entry_p;
        } else {
            /*
             * Overwrite the least recently used entry:
             */
            D_ASSERT(least_recently_used_entry_p != NULL);
            least_recently_used_entry_p->state = ARP_ENTRY_INVALID;
            *free_entry_pp = least_recently_used_entry_p;
        }
    }

    return matching_entry_p;
}


static error_t resolve_dest_ipv4_addr(
    struct net_layer3_end_point *layer3_end_point_p,
    const struct ipv4_address *dest_ip_addr_p,
    struct ethernet_mac_address *dest_mac_addr_p)
{
    unsigned int arp_request_retries = 0;
    struct arp_cache_entry *matching_entry_p = NULL;
    struct arp_cache_entry *free_entry_p = NULL;
    struct arp_cache *arp_cache_p = &layer3_end_point_p->ipv4.arp_cache;
    error_t error;

    rtos_mutex_lock(&arp_cache_p->mutex);
    for ( ; ; ) {
        bool send_arp_request = false;
        matching_entry_p = arp_cache_lookup_or_allocate(arp_cache_p,
                                                        dest_ip_addr_p,
                                                        &free_entry_p);
        uint32_t current_ticks = rtos_get_ticks_since_boot();

        if (matching_entry_p == NULL) {
            send_arp_request = true;
        } else if (matching_entry_p->state == ARP_ENTRY_FILLED) {
            if (RTOS_TICKS_DELTA(matching_entry_p->entry_filled_time_stamp,
                                 current_ticks) <
                    ARP_CACHE_ENTRY_LIFETIME_IN_TICKS) {
                /*
                 * ARP cache hit
                 */
                *dest_mac_addr_p = matching_entry_p->dest_mac_addr;
                break;
            } else {
                /*
                 * ARP entry expired, send a new ARP request:
                 */
                if (g_net_layer3.tracing_on) {
                    DEBUG_PRINTF("Net layer3: Expired ARP cache entry for IP address %u.%u.%u.%u\n",
                                 dest_ip_addr_p->bytes[0],
                                 dest_ip_addr_p->bytes[1],
                                 dest_ip_addr_p->bytes[2],
                                 dest_ip_addr_p->bytes[3]);
                }

                matching_entry_p->state = ARP_ENTRY_INVALID;
                send_arp_request = true;
            }
        } else {
            D_ASSERT(matching_entry_p->state == ARP_ENTRY_HALF_FILLED);

            if (RTOS_TICKS_DELTA(matching_entry_p->arp_request_time_stamp, current_ticks) >=
                MILLISECONDS_TO_TICKS(ARP_REPLY_WAIT_TIMEOUT_IN_MS)) {
                /*
                 * Re-send ARP request:
                 */
                ERROR_PRINTF(
                    "Outstanding ARP request re-sent for IP address: %u.%u.%u.%u\n",
                    dest_ip_addr_p->bytes[0],
                    dest_ip_addr_p->bytes[1],
                    dest_ip_addr_p->bytes[2],
                    dest_ip_addr_p->bytes[3]);

                send_arp_request = true;
            }
        }

        /*
         * Send ARP request if necessary:
         */
        if (send_arp_request) {
            if (arp_request_retries == ARP_REQUEST_MAX_RETRIES) {
                error = CAPTURE_ERROR("Unreachable IP address",
                                      dest_ip_addr_p->value, 0);

                ERROR_PRINTF("Unreachable IP address: %u.%u.%u.%u\n",
                             dest_ip_addr_p->bytes[0],
                             dest_ip_addr_p->bytes[1],
                             dest_ip_addr_p->bytes[2],
                             dest_ip_addr_p->bytes[3]);

                goto common_exit;
            }

            arp_request_retries ++;
            if (matching_entry_p != NULL) {
                matching_entry_p->arp_request_time_stamp = rtos_get_ticks_since_boot();
                matching_entry_p->state = ARP_ENTRY_HALF_FILLED;
            } else {
                D_ASSERT(free_entry_p != NULL);
                free_entry_p->dest_ip_addr.value = dest_ip_addr_p->value;
                free_entry_p->arp_request_time_stamp = rtos_get_ticks_since_boot();
                free_entry_p->state = ARP_ENTRY_HALF_FILLED;
            }

            net_send_arp_request(layer3_end_point_p->layer2_end_point_p,
                                 &layer3_end_point_p->ipv4.local_ip_addr,
                                 dest_ip_addr_p);
        }

        /*
         * Wait for ARP cache update:
         */
        rtos_mutex_unlock(&arp_cache_p->mutex);
        rtos_semaphore_wait_timeout(&arp_cache_p->cache_updated_semaphore,
                                    ARP_REPLY_WAIT_TIMEOUT_IN_MS);
        rtos_mutex_lock(&arp_cache_p->mutex);
    }

    error = 0;

common_exit:
    rtos_mutex_unlock(&arp_cache_p->mutex);
    return error;
}


static void arp_cache_update(struct arp_cache *arp_cache_p,
                             const struct ipv4_address *dest_ip_addr_p,
                             struct ethernet_mac_address *dest_mac_addr_p)
{
    struct arp_cache_entry *chosen_entry_p = NULL;
    struct arp_cache_entry *free_entry_p = NULL;

    rtos_mutex_lock(&arp_cache_p->mutex);
    chosen_entry_p = arp_cache_lookup_or_allocate(arp_cache_p, dest_ip_addr_p,
                                                  &free_entry_p);

    if (chosen_entry_p == NULL) {
        D_ASSERT(free_entry_p != NULL);
        chosen_entry_p = free_entry_p;
        chosen_entry_p->dest_ip_addr.value = dest_ip_addr_p->value;
    }

    COPY_MAC_ADDRESS(&chosen_entry_p->dest_mac_addr, dest_mac_addr_p);
    chosen_entry_p->state = ARP_ENTRY_FILLED;
    chosen_entry_p->entry_filled_time_stamp = rtos_get_ticks_since_boot();
    rtos_mutex_unlock(&arp_cache_p->mutex);
    rtos_semaphore_signal(&arp_cache_p->cache_updated_semaphore);
}


/**
 * Sends an IPv4 packet over Ethernet
 */
error_t net_layer3_send_ipv4_packet(const struct ipv4_address *dest_ip_addr_p,
                                    struct network_packet *tx_packet_p,
                                    size_t data_payload_length,
                                    uint_fast8_t ip_packet_type)
{
    struct ethernet_mac_address dest_mac_addr;
    error_t error;

    D_ASSERT(tx_packet_p->signature == NET_TX_PACKET_SIGNATURE);
    D_ASSERT(data_payload_length <= NET_MAX_IPV4_PACKET_PAYLOAD_SIZE);

    struct ethernet_frame *tx_frame_p =
       (struct ethernet_frame *)tx_packet_p->data_buffer;
    struct ipv4_header *const ipv4_header_p = &tx_frame_p->ipv4_header;

    struct net_layer3_end_point *layer3_end_point_p =
        choose_ipv4_local_layer3_end_point(dest_ip_addr_p);

    struct net_layer2_end_point *layer2_end_point_p =
        layer3_end_point_p->layer2_end_point_p;

    /*
     * Populate IP header
     */
    ipv4_header_p->version_and_header_length = 0;
    SET_BIT_FIELD(ipv4_header_p->version_and_header_length,
          IP_VERSION_MASK, IP_VERSION_SHIFT, 4);
    SET_BIT_FIELD(ipv4_header_p->version_and_header_length,
          IP_HEADER_LENGTH_MASK, IP_HEADER_LENGTH_SHIFT, 5);

    ipv4_header_p->type_of_service = 0; /* normal service */
    ipv4_header_p->total_length =
        hton16(sizeof(struct ipv4_header) + data_payload_length);

    ipv4_header_p->identification =
        hton16(ATOMIC_POST_INCREMENT_UINT16(
                    &layer3_end_point_p->ipv4.next_tx_ip_packet_seq_num));

    /*
     * No IP packet fragmentation is supported:
     */
    ipv4_header_p->flags_and_fragment_offset =
        hton16(IP_FLAG_DONT_FRAGMENT_MASK);

    ipv4_header_p->time_to_live = 64; /* max routing hops */
    ipv4_header_p->protocol_type = ip_packet_type;

    ipv4_header_p->source_ip_addr.value =
        layer3_end_point_p->ipv4.local_ip_addr.value;

    ipv4_header_p->dest_ip_addr.value = dest_ip_addr_p->value;

    /*
     * NOTE: enet_frame->ipv4_header.header_checksum is computed by hardware.
     * We just need to initialize the checksum field to 0
     */
    ipv4_header_p->header_checksum = 0;

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: IPv4 packet sent:\n"
                     "\tsource IPv4 address %u.%u.%u.%u\n"
                     "\tdestination IPv4 address %u.%u.%u.%u\n"
                     "\tPacket type %#x, Total length: %u\n",
                     ipv4_header_p->source_ip_addr.bytes[0],
                     ipv4_header_p->source_ip_addr.bytes[1],
                     ipv4_header_p->source_ip_addr.bytes[2],
                     ipv4_header_p->source_ip_addr.bytes[3],
                     ipv4_header_p->dest_ip_addr.bytes[0],
                     ipv4_header_p->dest_ip_addr.bytes[1],
                     ipv4_header_p->dest_ip_addr.bytes[2],
                     ipv4_header_p->dest_ip_addr.bytes[3],
                     ipv4_header_p->protocol_type,
                     ntoh16(ipv4_header_p->total_length));
    }

    ATOMIC_POST_INCREMENT_UINT32(&g_net_layer3.ipv4.sent_packets_count);

    /*
     * Get destination MAC address:
     */
    if (layer3_end_point_p->ipv4.local_ip_addr.value == dest_ip_addr_p->value) {
        error = CAPTURE_ERROR("IPv4 Loopback not supported", 0, 0);
    } else if (dest_ip_addr_p->value == IPV4_BROADCAST_ADDR) {
        dest_mac_addr = g_ethernet_broadcast_mac_addr;
        error = 0;
    } else if (IPV4_ADDR_IS_MULTICAST(dest_ip_addr_p)) {
        map_ipv4_multicast_addr_to_ethernet_multicast_addr(dest_ip_addr_p,
                                                           &dest_mac_addr);
        error = 0;
    } else if (SAME_IPv4_SUBNET(&layer3_end_point_p->ipv4.local_ip_addr,
                                dest_ip_addr_p,
                                layer3_end_point_p->ipv4.subnet_mask)) {
        error = resolve_dest_ipv4_addr(layer3_end_point_p,
                                       dest_ip_addr_p,
                                       &dest_mac_addr);
    } else if (layer3_end_point_p->ipv4.default_gateway_ip_addr.value == IPV4_NULL_ADDR) {
        error = CAPTURE_ERROR("No default IPv4 gateway defined", 0, 0);
    } else {
        error = resolve_dest_ipv4_addr(layer3_end_point_p,
                                       &layer3_end_point_p->ipv4.default_gateway_ip_addr,
                                       &dest_mac_addr);
    }

    if (error != 0) {
        return error;
    }

   return net_layer2_send_ethernet_frame(layer2_end_point_p,
                                         &dest_mac_addr,
                                         tx_packet_p,
                                         FRAME_TYPE_IPv4_PACKET,
                                         sizeof(struct ipv4_header) + data_payload_length);
}



/**
 * Send an ICMPv4 message
 *
 * @param dest_ip_addr_p
 * @param tx_packet_p
 * @param msg_type
 * @param msg_code
 * @param data_payload_length
 *
 * @return 0, on success
 * @return error code, on failure
 *
 */
error_t net_layer3_send_ipv4_icmp_message(const struct ipv4_address *dest_ip_addr_p,
                                          struct network_packet *tx_packet_p,
                                          uint8_t msg_type,
                                          uint8_t msg_code,
                                          size_t data_payload_length)
{
    /*
     * Populate ICMP header:
     */
    struct icmpv4_header *icmp_header_p =
    (struct icmpv4_header *)GET_IPV4_DATA_PAYLOAD_AREA(tx_packet_p);

    icmp_header_p->msg_type = msg_type;
    icmp_header_p->msg_code = msg_code;

    /*
     * NOTE: icmp_header_p->msg_checksum is computed by hardware.
     * We just need to initialize the checksum field to 0
     */
    icmp_header_p->msg_checksum = 0;

    /*
     * Send IP packet:
     */
    return net_layer3_send_ipv4_packet(dest_ip_addr_p,
                                       tx_packet_p,
                                       sizeof(struct icmpv4_header) + data_payload_length,
                                       IP_PACKET_TYPE_ICMP);
}


static void net_trace_received_arp_packet(struct network_packet *packet_p)
{
    struct ethernet_frame *frame_p =
        (struct ethernet_frame *)packet_p->data_buffer;

    uint16_t arp_operation = ntoh16(frame_p->arp_packet.operation);

    DEBUG_PRINTF(
        "Net layer3: Received ARP packet:\n"
        "\toperation: %s (%#x)\n"
        "\tsource mac addr: %x:%x:%x:%x:%x:%x\n"
        "\tsource IP address: %u.%u.%u.%u\n"
        "\tdest mac addr: %x:%x:%x:%x:%x:%x\n"
        "\tdest IP address: %u.%u.%u.%u\n",
        arp_operation == ARP_REQUEST ? "ARP request" : "ARP reply",
        arp_operation,
        frame_p->arp_packet.source_mac_addr.bytes[0],
        frame_p->arp_packet.source_mac_addr.bytes[1],
        frame_p->arp_packet.source_mac_addr.bytes[2],
        frame_p->arp_packet.source_mac_addr.bytes[3],
        frame_p->arp_packet.source_mac_addr.bytes[4],
        frame_p->arp_packet.source_mac_addr.bytes[5],
        frame_p->arp_packet.source_ip_addr.bytes[0],
        frame_p->arp_packet.source_ip_addr.bytes[1],
        frame_p->arp_packet.source_ip_addr.bytes[2],
        frame_p->arp_packet.source_ip_addr.bytes[3],
        frame_p->arp_packet.dest_mac_addr.bytes[0],
        frame_p->arp_packet.dest_mac_addr.bytes[1],
        frame_p->arp_packet.dest_mac_addr.bytes[2],
        frame_p->arp_packet.dest_mac_addr.bytes[3],
        frame_p->arp_packet.dest_mac_addr.bytes[4],
        frame_p->arp_packet.dest_mac_addr.bytes[5],
        frame_p->arp_packet.dest_ip_addr.bytes[0],
        frame_p->arp_packet.dest_ip_addr.bytes[1],
        frame_p->arp_packet.dest_ip_addr.bytes[2],
        frame_p->arp_packet.dest_ip_addr.bytes[3]);
}


void net_layer3_receive_arp_packet(struct network_packet *rx_packet_p)
{
    D_ASSERT(rx_packet_p->total_length >=
             sizeof(struct ethernet_header) + sizeof(struct arp_packet));

    if (g_net_layer3.tracing_on) {
        net_trace_received_arp_packet(rx_packet_p);
    }

    struct net_layer2_end_point *const layer2_end_point_p =
        rx_packet_p->layer2_end_point_p;
    struct net_layer3_end_point *const layer3_end_point_p =
        layer2_end_point_p->layer3_end_point_p;
    struct ethernet_frame *const rx_frame_p =
        (struct ethernet_frame *)rx_packet_p->data_buffer;

    uint16_t arp_operation = ntoh16(rx_frame_p->arp_packet.operation);
    struct ipv4_address source_ip_addr;
    struct ipv4_address dest_ip_addr;
    bool duplicate_ip_addr_detected = false;

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);
    D_ASSERT(layer3_end_point_p->layer2_end_point_p == layer2_end_point_p);

    if (arp_operation == ARP_REQUEST || arp_operation == ARP_REPLY) {
        D_ASSERT(rx_frame_p->arp_packet.link_addr_type == hton16(0x1));
        D_ASSERT(rx_frame_p->arp_packet.network_addr_type ==
                 hton16(FRAME_TYPE_IPv4_PACKET));
        D_ASSERT(rx_frame_p->arp_packet.link_addr_size ==
                 sizeof(struct ethernet_mac_address));
        D_ASSERT(rx_frame_p->arp_packet.network_addr_size ==
                 sizeof(struct ipv4_address));
    }

    COPY_UNALIGNED_IPv4_ADDRESS(&source_ip_addr,
                                &rx_frame_p->arp_packet.source_ip_addr);

    COPY_UNALIGNED_IPv4_ADDRESS(&dest_ip_addr,
                                &rx_frame_p->arp_packet.dest_ip_addr);

    switch(arp_operation) {
    case ARP_REQUEST:
        if (dest_ip_addr.value == layer3_end_point_p->ipv4.local_ip_addr.value) {
            if (source_ip_addr.value == dest_ip_addr.value) {
                /*
                 * Duplicate IPv4 address detected:
                 * Received gratuitous ARP request from  a remote layer-3
                 * end-point that wants to have the same IP address as us.
                 */
                duplicate_ip_addr_detected = true;
                ERROR_PRINTF(
                    "Duplicated IP address %u.%u.%u.%u detected. "
                    "Remote node with MAC address %x:%x:%x:%x:%x:%x wants to have the same IP address.\n",
                    source_ip_addr.bytes[0],
                    source_ip_addr.bytes[1],
                    source_ip_addr.bytes[2],
                    source_ip_addr.bytes[3],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[0],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[1],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[2],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[3],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[4],
                    rx_frame_p->arp_packet.source_mac_addr.bytes[5]);
            }

            /*
             * Send ARP reply
             */
            net_send_arp_reply(layer2_end_point_p,
                               &layer3_end_point_p->ipv4.local_ip_addr,
                               &rx_frame_p->arp_packet.source_mac_addr,
                               &source_ip_addr);
        }

        if (source_ip_addr.value != IPV4_NULL_ADDR &&
            !duplicate_ip_addr_detected) {
            /*
             * Update ARP cache with (source IP addr, source MAC addr)
             */
            arp_cache_update(&layer3_end_point_p->ipv4.arp_cache,
                             &source_ip_addr,
                             &rx_frame_p->arp_packet.source_mac_addr);
        }

        break;

    case ARP_REPLY:
        D_ASSERT(
            UNALIGNED_IPv4_ADDRESSES_EQUAL(&rx_frame_p->arp_packet.dest_ip_addr,
                                           &layer3_end_point_p->ipv4.local_ip_addr));

        struct ethernet_mac_address local_mac_address;

        net_layer2_get_mac_addr(layer2_end_point_p, &local_mac_address);

        D_ASSERT(MAC_ADDRESSES_EQUAL(&rx_frame_p->arp_packet.dest_mac_addr,
                                     &local_mac_address));

        if (source_ip_addr.value == layer3_end_point_p->ipv4.local_ip_addr.value) {
            /*
             * Duplicate IPv4 address detected:
             * Received ARP reply from a remote layer-3
             * end-point that already has the same IP address as us.
             */
            ERROR_PRINTF(
                "Duplicated IP address %u.%u.%u.%u detected. "
                "Remote node with MAC address %x:%x:%x:%x:%x:%x already has the same IP address.\n",
                source_ip_addr.bytes[0],
                source_ip_addr.bytes[1],
                source_ip_addr.bytes[2],
                source_ip_addr.bytes[3],
                rx_frame_p->arp_packet.source_mac_addr.bytes[0],
                rx_frame_p->arp_packet.source_mac_addr.bytes[1],
                rx_frame_p->arp_packet.source_mac_addr.bytes[2],
                rx_frame_p->arp_packet.source_mac_addr.bytes[3],
                rx_frame_p->arp_packet.source_mac_addr.bytes[4],
                rx_frame_p->arp_packet.source_mac_addr.bytes[5]);
        } else {
            /*
             * Update ARP cache with (source IP addr, source MAC addr)
             */
            arp_cache_update(&layer3_end_point_p->ipv4.arp_cache,
                             &source_ip_addr,
                             &rx_frame_p->arp_packet.source_mac_addr);
        }

        break;

    default:
        ERROR_PRINTF("Received ARP packet with unsupported operation (%#x)\n",
                     arp_operation);
    }

    net_recycle_rx_packet(rx_packet_p);
}


static void net_send_ipv4_ping_reply(
    const struct ipv4_address *dest_ip_addr_p,
    const struct icmpv4_echo_message *ping_request_msg_p)
{
    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);
    struct icmpv4_echo_message *echo_msg_p =
    (struct icmpv4_echo_message *)GET_IPV4_DATA_PAYLOAD_AREA(tx_packet_p);

    echo_msg_p->identifier = ping_request_msg_p->identifier;
    echo_msg_p->seq_num = ping_request_msg_p->seq_num;
    net_layer3_send_ipv4_icmp_message(dest_ip_addr_p,
                                      tx_packet_p,
                                      ICMP_TYPE_PING_REPLY,
                                      ICMP_CODE_PING_REPLY,
                                      sizeof(struct icmpv4_echo_message) -
                                         sizeof(struct icmpv4_header));
}


static void net_process_incoming_icmpv4_message(struct network_packet *rx_packet_p)
{
    bool signal_ping_reply_received = false;

    D_ASSERT(rx_packet_p->total_length >=
             sizeof(struct ethernet_header) + sizeof(struct ipv4_header) +
             sizeof(struct icmpv4_header));

    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(rx_packet_p);
    struct icmpv4_header *icmpv4_header_p = GET_IPV4_DATA_PAYLOAD_AREA(rx_packet_p);

    switch (icmpv4_header_p->msg_type) {
    case ICMP_TYPE_PING_REPLY:
        D_ASSERT(icmpv4_header_p->msg_code == ICMP_CODE_PING_REPLY);

        rtos_mutex_lock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
        if (g_net_layer3.ipv4.expecting_ping_reply) {
            net_packet_queue_add(&g_net_layer3.ipv4.rx_ipv4_ping_reply_packet_queue,
                                 rx_packet_p);

            g_net_layer3.ipv4.expecting_ping_reply = false;
            signal_ping_reply_received = true;
        } else {
            /*
             * Drop unmatched ping reply
             */
            net_recycle_rx_packet(rx_packet_p);
        }

        rtos_mutex_unlock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
        if (signal_ping_reply_received) {
            rtos_semaphore_signal(&g_net_layer3.ipv4.ping_reply_received_semaphore);
        }

        break;

    case ICMP_TYPE_PING_REQUEST:
        D_ASSERT(icmpv4_header_p->msg_code == ICMP_CODE_PING_REQUEST);

        struct ipv4_address dest_ip_addr;

        dest_ip_addr.value = ipv4_header_p->source_ip_addr.value;

        net_send_ipv4_ping_reply(
            &dest_ip_addr,
            (struct icmpv4_echo_message *)icmpv4_header_p);

        net_recycle_rx_packet(rx_packet_p);
        break;

    default:
        ERROR_PRINTF("Received ICMP message with unsupported type: %#x\n",
                     icmpv4_header_p->msg_type);

        net_recycle_rx_packet(rx_packet_p);
    }
}


/**
 * ICMPv4 packet receiver task for a given IPv4 end point
 */
static void icmpv4_packet_receiver_task(void *arg)
{
    struct ipv4_end_point *const ipv4_end_point_p =
        (struct ipv4_end_point *)arg;
    struct net_layer3_end_point *const layer3_end_point_p =
        ENCLOSING_STRUCT(ipv4_end_point_p, struct net_layer3_end_point, ipv4);

#   ifdef USE_MPU
    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                NULL);
#   endif

    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);

    for ( ; ; ) {
        struct network_packet *rx_packet_p = NULL;

        rx_packet_p =
            net_packet_queue_remove(&ipv4_end_point_p->rx_icmpv4_packet_queue, 0);

        D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
        rx_packet_p->state_flags &= ~NET_PACKET_IN_ICMP_QUEUE;
        net_process_incoming_icmpv4_message(rx_packet_p);
    }

    ERROR_PRINTF("task %s should not have terminated\n",
                 rtos_task_self()->tsk_name_p);
}


static void net_send_ipv4_dhcp_request(
    struct net_layer3_end_point *layer3_end_point_p,
    struct net_layer4_end_point *client_end_point_p,
    struct dhcp_message *dhcp_offer_msg_p)
{
    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);

    struct dhcp_message *dhcp_request_msg_p =
        get_ipv4_udp_data_payload_area(tx_packet_p);

    *dhcp_request_msg_p = *dhcp_offer_msg_p;
    dhcp_request_msg_p->op = 0x1;
    dhcp_request_msg_p->options[0] = 53; /* option 1 type: DHCP message type */
    dhcp_request_msg_p->options[1] = 1; /* option length */
    dhcp_request_msg_p->options[2] = 0x03; /* option value: DHCPREQUEST */

    dhcp_request_msg_p->options[3] = 54; /* option 2 type: DHCP server identifier */
    dhcp_request_msg_p->options[4] = 4; /* option length */
    dhcp_request_msg_p->options[5] = dhcp_offer_msg_p->next_server_ip_addr.bytes[0]; /* option value[0] */
    dhcp_request_msg_p->options[6] = dhcp_offer_msg_p->next_server_ip_addr.bytes[1]; /* option value[1] */
    dhcp_request_msg_p->options[7] = dhcp_offer_msg_p->next_server_ip_addr.bytes[2]; /* option value[2] */
    dhcp_request_msg_p->options[8] = dhcp_offer_msg_p->next_server_ip_addr.bytes[3]; /* option value[3] */

    dhcp_request_msg_p->options[9] = 50; /* option 3 type: Requested IP address */
    dhcp_request_msg_p->options[10] = 4; /* option length */
    dhcp_request_msg_p->options[11] = dhcp_offer_msg_p->your_ip_addr.bytes[0]; /* option value[0] */
    dhcp_request_msg_p->options[12] = dhcp_offer_msg_p->your_ip_addr.bytes[1]; /* option value[1] */
    dhcp_request_msg_p->options[13] = dhcp_offer_msg_p->your_ip_addr.bytes[2]; /* option value[2] */
    dhcp_request_msg_p->options[14] = dhcp_offer_msg_p->your_ip_addr.bytes[3]; /* option value[3] */

    dhcp_request_msg_p->options[15] = 55; /* option 2 type: parameter request list */
    dhcp_request_msg_p->options[16] = 11; /* option length */
    dhcp_request_msg_p->options[17] = 0x01; /* option value[0]: subnet mask */
    dhcp_request_msg_p->options[18] = 0x1c; /* option value[1]: broadcast address */
    dhcp_request_msg_p->options[19] = 0x02; /* option value[2]: time offset */
    dhcp_request_msg_p->options[20] = 0x03; /* option value[3]: router */
    dhcp_request_msg_p->options[21] = 0x0f; /* option value[4]: domain name */
    dhcp_request_msg_p->options[22] = 0x06; /* option value[5]: domain name server */
    dhcp_request_msg_p->options[23] = 0x77; /* option value[6]: domain search */
    dhcp_request_msg_p->options[24] = 0x0c; /* option value[7]: host name */
    dhcp_request_msg_p->options[25] = 0x1a; /* option value[8]: interface MTU */
    dhcp_request_msg_p->options[26] = 0x79; /* option value[9]: classless static route */
    dhcp_request_msg_p->options[27] = 0x2a; /* option value[10]: network time protocol servers */

    struct ipv4_address dest_ip_addr = { .value = IPV4_BROADCAST_ADDR };

    net_layer4_send_udp_datagram_over_ipv4(client_end_point_p, &dest_ip_addr,
                                           hton16(DHCP_UDP_SERVER_PORT),
                                           tx_packet_p,
                                           sizeof(struct dhcp_message) + 28);

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: DHCP client sent request message\n");
    }
}


static void net_set_local_ipv4_addr_from_dhcp(
    struct net_layer3_end_point *layer3_end_point_p,
    struct dhcp_message *dhcp_ack_msg_p,
    size_t dhcp_ack_msg_size)
{
    uint32_t subnet_mask = 0;
    uint32_t lease_time = 0;
    struct ipv4_address router_ip_addr = { .value = IPV4_NULL_ADDR };
    struct ipv4_address *local_ip_addr_p = &dhcp_ack_msg_p->your_ip_addr;
    size_t options_len = dhcp_ack_msg_size - sizeof(struct dhcp_message);
    unsigned int expected_options_found = 0;

    D_ASSERT(g_net_layer3.initialized);
    D_ASSERT(local_ip_addr_p->value != IPV4_NULL_ADDR);

    /*
     * Extract DHCP options:
     */
    for (unsigned int i = 0; i < options_len; ) {
        if (dhcp_ack_msg_p->options[i] == 1) {          /* subnet mask option */
            D_ASSERT(dhcp_ack_msg_p->options[i + 1] == 4);

            uint8_t *subnet_mask_bytes_p = (uint8_t *)&subnet_mask;

            subnet_mask_bytes_p[0] = dhcp_ack_msg_p->options[i + 2];
            subnet_mask_bytes_p[1] = dhcp_ack_msg_p->options[i + 3];
            subnet_mask_bytes_p[2] = dhcp_ack_msg_p->options[i + 4];
            subnet_mask_bytes_p[3] = dhcp_ack_msg_p->options[i + 5];

            expected_options_found ++;
        } else if (dhcp_ack_msg_p->options[i] == 3) {   /* router option */
            D_ASSERT(dhcp_ack_msg_p->options[i + 1] == 4);

            router_ip_addr.bytes[0] = dhcp_ack_msg_p->options[i + 2];
            router_ip_addr.bytes[1] = dhcp_ack_msg_p->options[i + 3];
            router_ip_addr.bytes[2] = dhcp_ack_msg_p->options[i + 4];
            router_ip_addr.bytes[3] = dhcp_ack_msg_p->options[i + 5];

            expected_options_found ++;
        } else if (dhcp_ack_msg_p->options[i] == 51) {  /* lease time option */
            D_ASSERT(dhcp_ack_msg_p->options[i + 1] == 4);

            uint8_t *lease_time_bytes_p = (uint8_t *)&lease_time;

            lease_time_bytes_p[0] = dhcp_ack_msg_p->options[i + 2];
            lease_time_bytes_p[1] = dhcp_ack_msg_p->options[i + 3];
            lease_time_bytes_p[2] = dhcp_ack_msg_p->options[i + 4];
            lease_time_bytes_p[3] = dhcp_ack_msg_p->options[i + 5];

            lease_time = ntoh32(lease_time);
            expected_options_found ++;
        }

        /*
         * Skip to next option:
         */
        i += dhcp_ack_msg_p->options[i + 1];
    }

    D_ASSERT(expected_options_found == 3);

    layer3_end_point_p->ipv4.local_ip_addr = *local_ip_addr_p;
    layer3_end_point_p->ipv4.subnet_mask = subnet_mask;
    layer3_end_point_p->ipv4.default_gateway_ip_addr = router_ip_addr;
    layer3_end_point_p->ipv4.dhcp_lease_time = lease_time;

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: Set local IP address from DHCP: %u.%u.%u.%u\n",
                     local_ip_addr_p->bytes[0],
                     local_ip_addr_p->bytes[1],
                     local_ip_addr_p->bytes[2],
                     local_ip_addr_p->bytes[3]);
    }

    /*
     * Send gratuitous ARP request (to catch if someone else is using the same
     * IP address):
     */
    net_send_arp_request(layer3_end_point_p->layer2_end_point_p,
                         &layer3_end_point_p->ipv4.local_ip_addr,
                         &layer3_end_point_p->ipv4.local_ip_addr);
}


/**
 * DHCPv4 client task
 */
static void dhcpv4_client_task(void *arg)
{
    enum dhcp_client_states {
        DHCP_OFFER_EXPECTED,
        DHCP_ACKNOWLEDGE_EXPECTED,
        DHCP_LEASE_GRANTED,
    } state;

    error_t error;
    struct network_packet *rx_packet_p = NULL;
    struct ipv4_address server_ip_addr;
    uint16_t server_port;
    struct dhcp_message *dhcp_msg_p;
    size_t dhcp_msg_size;
    uint32_t timeout_ms = 0;
    struct ipv4_end_point *const ipv4_end_point_p =
        (struct ipv4_end_point *)arg;
    struct net_layer3_end_point *const layer3_end_point_p =
        ENCLOSING_STRUCT(ipv4_end_point_p, struct net_layer3_end_point, ipv4);

#   ifdef USE_MPU
    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                NULL);
#   endif

    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);

    struct net_layer4_end_point *dhcp_client_end_point_p =
            &ipv4_end_point_p->dhcpv4_client_end_point;

    net_layer4_udp_end_point_init(dhcp_client_end_point_p);

    error = net_layer4_udp_end_point_bind(dhcp_client_end_point_p,
                                          DHCP_UDP_CLIENT_PORT);
    if (error != 0) {
        goto exit;
    }

    net_send_ipv4_dhcp_discovery(layer3_end_point_p, dhcp_client_end_point_p);
    state = DHCP_OFFER_EXPECTED;
    for ( ; ; ) {
        /*
         * Receive DHCP message:
         */
        net_layer4_receive_udp_datagram_over_ipv4(dhcp_client_end_point_p,
                                                  timeout_ms,
                                                  &server_ip_addr,
                                                  &server_port,
                                                  &rx_packet_p);

        D_ASSERT(rx_packet_p != NULL);
        D_ASSERT(server_port == DHCP_UDP_SERVER_PORT);
        if (g_net_layer3.tracing_on) {
            DEBUG_PRINTF("Net layer3: DHCP client received message %#x from %u.%u.%u.%u\n",
                         dhcp_msg_p->options[2],
                         server_ip_addr.bytes[0],
                         server_ip_addr.bytes[1],
                         server_ip_addr.bytes[2],
                         server_ip_addr.bytes[3]);
        }

        dhcp_msg_size = get_ipv4_udp_data_payload_length(rx_packet_p);
        D_ASSERT(dhcp_msg_size >= sizeof(struct dhcp_message));
        switch (state) {
        case DHCP_OFFER_EXPECTED:
            dhcp_msg_p = get_ipv4_udp_data_payload_area(rx_packet_p);
            D_ASSERT(dhcp_msg_p->op == 0x2);
            D_ASSERT(dhcp_msg_p->options[2] == 0x2);
            net_send_ipv4_dhcp_request(layer3_end_point_p,
                                       dhcp_client_end_point_p,
                                       dhcp_msg_p);
            state = DHCP_ACKNOWLEDGE_EXPECTED;
            break;

        case DHCP_ACKNOWLEDGE_EXPECTED:
            dhcp_msg_p = get_ipv4_udp_data_payload_area(rx_packet_p);
            D_ASSERT(dhcp_msg_p->op == 0x2);
            D_ASSERT(dhcp_msg_p->options[2] == 0x5);
            net_set_local_ipv4_addr_from_dhcp(layer3_end_point_p, dhcp_msg_p,
                                              dhcp_msg_size);
            state = DHCP_LEASE_GRANTED;
            break;

        default:
            /* Drop packet */
            if (g_net_layer3.tracing_on) {
                DEBUG_PRINTF("Net layer3: Dropped DHCP message (DHCP client state: %d)\n",
                             state);
            }
        }

        net_recycle_rx_packet(rx_packet_p);
    }

exit:
    ERROR_PRINTF("task %s should not have terminated\n",
                 rtos_task_self()->tsk_name_p);
}


/**
 * IPv4-specific initialization of a layer-3 end point
 *
 * @param ipv4_end_point_p Pointer to IPv4 layer-3 end point
 *
 * NOTE: This function is to be invoked from net_layer3_end_point_init()
 */
void net_layer3_ipv4_end_point_init(struct ipv4_end_point *ipv4_end_point_p)
{
    net_packet_queue_init("ICMPv4 incoming packet queue",
                          true,
                          &ipv4_end_point_p->rx_icmpv4_packet_queue);

    /*
     * Initialize IPv4 ARP cache for the layer-3 end point
     */
    arp_cache_init(&ipv4_end_point_p->arp_cache);
}


/**
 * Start RTOS tasks for an IPv4 layer-3 end point
 *
 * @param ipv4_end_point_p    Pointer to IPv4 end point
 *
 * NOTE: This function is to be invoked from net_layer3_end_point_start_tasks()
 */
void net_layer3_ipv4_end_point_start_tasks(struct ipv4_end_point *ipv4_end_point_p)
{
    /*
     * Create ICMPv4 packet receiver task:
     */
    rtos_task_create(&ipv4_end_point_p->icmpv4_packet_receiver_task,
                     "ICMPv4 packet receiver task",
                     icmpv4_packet_receiver_task,
                     ipv4_end_point_p,
                     HIGHEST_APP_TASK_PRIORITY + 2);

    /*
     * Create DHCPv4 client task:
     */
    rtos_task_create(&ipv4_end_point_p->dhcpv4_client_task,
                     "DHCPv4 client task",
                     dhcpv4_client_task,
                     ipv4_end_point_p,
                     HIGHEST_APP_TASK_PRIORITY + 2);
}


void net_layer3_receive_ipv4_packet(struct network_packet *rx_packet_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(rx_packet_p, sizeof *rx_packet_p, 0);
#   endif

    D_ASSERT(rx_packet_p->total_length >=
             sizeof(struct ethernet_header) + sizeof(struct ipv4_header));

    struct net_layer2_end_point *const layer2_end_point_p =
        rx_packet_p->layer2_end_point_p;
    struct net_layer3_end_point *const layer3_end_point_p =
        layer2_end_point_p->layer3_end_point_p;

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);
    D_ASSERT(layer3_end_point_p->layer2_end_point_p == layer2_end_point_p);

    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(rx_packet_p);

    if (g_net_layer3.tracing_on) {
        DEBUG_PRINTF("Net layer3: IPv4 packet received:\n"
                     "\tsource IPv4 address %u.%u.%u.%u\n"
                     "\tdestination IPv4 address %u.%u.%u.%u\n"
                     "\tPacket type %#x, Total length: %u\n",
                     ipv4_header_p->source_ip_addr.bytes[0],
                     ipv4_header_p->source_ip_addr.bytes[1],
                     ipv4_header_p->source_ip_addr.bytes[2],
                     ipv4_header_p->source_ip_addr.bytes[3],
                     ipv4_header_p->dest_ip_addr.bytes[0],
                     ipv4_header_p->dest_ip_addr.bytes[1],
                     ipv4_header_p->dest_ip_addr.bytes[2],
                     ipv4_header_p->dest_ip_addr.bytes[3],
                     ipv4_header_p->protocol_type,
                     ntoh16(ipv4_header_p->total_length));
    }

    bool packet_dropped = false;

    switch (ipv4_header_p->protocol_type) {
    case IP_PACKET_TYPE_ICMP:
        rx_packet_p->state_flags |= NET_PACKET_IN_ICMP_QUEUE;
        net_packet_queue_add(&layer3_end_point_p->ipv4.rx_icmpv4_packet_queue,
                             rx_packet_p);
        break;

    case IP_PACKET_TYPE_UDP:
        net_layer4_process_incoming_udp_datagram(rx_packet_p);
        break;

#if 0
    case IP_PACKET_TYPE_TCP:
        net_layer4_process_incoming_tcp_segment(rx_packet_p);
        break;
#endif

    default:
        ERROR_PRINTF("Received IPv4 packet with unsupported protocol type: %#x\n",
                     ipv4_header_p->protocol_type);

        net_recycle_rx_packet(rx_packet_p);
        packet_dropped = true;
    }

    if (packet_dropped) {
        ATOMIC_POST_INCREMENT_UINT32(&g_net_layer3.ipv4.rx_packets_dropped_count);
    } else {
		ATOMIC_POST_INCREMENT_UINT32(&g_net_layer3.ipv4.rx_packets_accepted_count);
    }

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Set IPv4 address for the local IPv4 end point

 * @param ip_addr_p         Pointer to IPv4 address
 * @param subnet_prefix  Subnet prefix
 */
void net_layer3_set_local_ipv4_address(const struct ipv4_address *ip_addr_p,
                                       uint8_t subnet_prefix)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(ip_addr_p, sizeof *ip_addr_p, 0);
#   endif

    D_ASSERT(g_net_layer3.initialized);
    D_ASSERT(ip_addr_p->value != IPV4_NULL_ADDR);
    D_ASSERT(subnet_prefix < 32);

    struct net_layer3_end_point *const layer3_end_point_p =
     &g_net_layer3.local_layer3_end_points[0];

    layer3_end_point_p->ipv4.local_ip_addr = *ip_addr_p;
    layer3_end_point_p->ipv4.subnet_mask = IPv4_SUBNET_MASK(subnet_prefix);

    struct net_layer2_end_point *const layer2_end_point_p =
        layer3_end_point_p->layer2_end_point_p;

#   ifdef USE_MPU
    rtos_thread_set_tmp_region(layer2_end_point_p, sizeof *layer2_end_point_p, 0);
#   endif

    INFO_PRINTF("Net layer3: Set local IPv4 address to %u.%u.%u.%u/%u "
                "for MAC address %02x:%02x:%02x:%02x:%02x:%02x\n",
                ip_addr_p->bytes[0],
                ip_addr_p->bytes[1],
                ip_addr_p->bytes[2],
                ip_addr_p->bytes[3],
                subnet_prefix,
                layer2_end_point_p->mac_address.bytes[0],
                layer2_end_point_p->mac_address.bytes[1],
                layer2_end_point_p->mac_address.bytes[2],
                layer2_end_point_p->mac_address.bytes[3],
                layer2_end_point_p->mac_address.bytes[4],
                layer2_end_point_p->mac_address.bytes[5]);

    /*
     * Send gratuitous ARP request (to catch if someone else is using the same
     * IP address):
     */
    net_send_arp_request(layer2_end_point_p,
                         &layer3_end_point_p->ipv4.local_ip_addr,
                         &layer3_end_point_p->ipv4.local_ip_addr);

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Retrieve IP address for the local IPv4 end point
 *
 * @param ip_addr_p    Pointer to area where IPv4 address is to be returned
 */
void net_layer3_get_local_ipv4_address(struct ipv4_address *ip_addr_p,
                                       struct ipv4_address *subnet_mask_p)
{
#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(ip_addr_p, sizeof *ip_addr_p, 0);
#   endif

    D_ASSERT(g_net_layer3.initialized);

    struct net_layer3_end_point *layer3_end_point_p =
     &g_net_layer3.local_layer3_end_points[0];

    *ip_addr_p = layer3_end_point_p->ipv4.local_ip_addr;
    subnet_mask_p->value = layer3_end_point_p->ipv4.subnet_mask;

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Parses an IPv4 address string of the form:
 * <ddd>.<ddd>.<ddd>.<ddd>[/<subnet prefix>]
 *
 * If subnet_prefix_p is not NULL, a subnet prefix is expected.
 *
 * @param ipv4_addr_string    IPv4 address string
 * @param ipv4_addr_p        Pointer to area where binary IPv4 address is to be
 *                             stores
 * @param subnet_prefix_p    Pointer to area where binary subnet prefix is to be
 *                             stored
 *
 * @return true, if parsing successful
 * @return false, if parsing unsuccessful
 */
bool net_layer3_parse_ipv4_addr(const char *ipv4_addr_string,
                                struct ipv4_address *ipv4_addr_p,
                                uint8_t *subnet_prefix_p)
{
    char parsing_buffer[19];
    char *token_s;
    char *token_end_s;

    strncpy(parsing_buffer, ipv4_addr_string, sizeof parsing_buffer);
    parsing_buffer[sizeof(parsing_buffer) - 1] = '\0';
    token_s = parsing_buffer;
    for (uint_fast8_t i = 0; i < 4; i ++) {
        if (i == 3) {
            if (subnet_prefix_p != NULL) {
                token_end_s = strchr(token_s, '/');
                if (token_end_s == NULL) {
                    return false;
                }

                *token_end_s = '\0';
            }
        } else {
            token_end_s = strchr(token_s, '.');
            if (token_end_s == NULL) {
                return false;
            }

            *token_end_s = '\0';
        }

        ipv4_addr_p->bytes[i] = atoi(token_s);
        token_s = token_end_s + 1;
    }

    if (subnet_prefix_p != NULL) {
        *subnet_prefix_p = atoi(token_s);
    }

    return true;
}


error_t net_layer3_send_ipv4_ping_request(
    const struct ipv4_address *dest_ip_addr_p,
    uint16_t identifier,
    uint16_t seq_num)
{
    error_t error;

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(dest_ip_addr_p, sizeof *dest_ip_addr_p, 0);
#   endif

    rtos_mutex_lock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
    while (g_net_layer3.ipv4.expecting_ping_reply) {
        rtos_mutex_unlock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
        rtos_semaphore_wait(&g_net_layer3.ipv4.ping_reply_received_semaphore);
        rtos_mutex_lock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
    }

    g_net_layer3.ipv4.expecting_ping_reply = true;
    rtos_mutex_unlock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);

    struct network_packet *tx_packet_p = net_layer2_allocate_tx_packet(true);

    D_ASSERT(tx_packet_p != NULL);
    struct icmpv4_echo_message *echo_msg_p =
        (struct icmpv4_echo_message *)GET_IPV4_DATA_PAYLOAD_AREA(tx_packet_p);

    echo_msg_p->identifier = identifier;
    echo_msg_p->seq_num = seq_num;
    error = net_layer3_send_ipv4_icmp_message(dest_ip_addr_p,
                                              tx_packet_p,
                                              ICMP_TYPE_PING_REQUEST,
                                              ICMP_CODE_PING_REQUEST,
                                              sizeof(struct icmpv4_echo_message) -
                                                  sizeof(struct icmpv4_header));
    if (error != 0) {
        rtos_mutex_lock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
        g_net_layer3.ipv4.expecting_ping_reply = false;
        rtos_mutex_unlock(&g_net_layer3.ipv4.expecting_ping_reply_mutex);
    }

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return error;
}


error_t net_layer3_receive_ipv4_ping_reply(
    uint32_t timeout_ms,
    struct ipv4_address *remote_ip_addr_p,
    uint16_t *identifier_p,
    uint16_t *seq_num_p)
{
    error_t error;

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);
#   endif

    struct network_packet *rx_packet_p = net_packet_queue_remove(
        &g_net_layer3.ipv4.rx_ipv4_ping_reply_packet_queue, timeout_ms);

    if (rx_packet_p == NULL) {
        error = CAPTURE_ERROR("No Rx packet available", timeout_ms, 0);
        goto exit;
    }

    D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);

    struct ipv4_header *ipv4_header_p = GET_IPV4_HEADER(rx_packet_p);
    struct icmpv4_header *icmpv4_header_p = GET_IPV4_DATA_PAYLOAD_AREA(rx_packet_p);
    struct icmpv4_echo_message *echo_msg_p =
        (struct icmpv4_echo_message *)(icmpv4_header_p);

    remote_ip_addr_p->value = ipv4_header_p->source_ip_addr.value;
    *identifier_p = echo_msg_p->identifier;
    *seq_num_p = echo_msg_p->seq_num;
    net_recycle_rx_packet(rx_packet_p);
    error = 0;

exit:
#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return error;
}
