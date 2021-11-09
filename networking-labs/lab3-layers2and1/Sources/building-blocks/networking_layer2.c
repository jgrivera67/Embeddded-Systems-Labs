/**
 * @file networking_layer2.c
 *
 * Networking layer 2 implementation
 *
 * @author German Rivera
 */
#include "networking_layer2.h"
#include "microcontroller.h"
#include "io_utils.h"
#include "mem_utils.h"
#include "ethernet_mac.h"
#include "ethernet_phy.h"
#include "networking_layer3.h"
#include "runtime_log.h"

const struct ethernet_mac_address g_ethernet_broadcast_mac_addr = {
    .bytes = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff }
};

const struct ethernet_mac_address g_ethernet_null_mac_addr = {
    .bytes = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }
};

/**
 * Networking layer-2 global state variables
 */
struct net_layer2 g_net_layer2 = {
    .initialized = false,
    .tracing_on = false,
    .rx_packets_accepted_count = 0,
	.rx_packets_dropped_count = 0,
	.sent_packets_count = 0,
    .local_layer2_end_points = {
        [0] = {
            .signature = NET_LAYER2_END_POINT_SIGNATURE,
            .type = NET_LAYER2_ETHERNET,
            .initialized = false,
            .layer3_end_point_p = &g_net_layer3.local_layer3_end_points[0],
            .ethernet_mac_p = &g_ethernet_mac0,
        },
    },
};

/**
 * Builds an Ethernet MAC address from the Micrcontroller's unique
 * hardware identifier
 *
 * @param mac_addr_p Pointer to are where the built MAC address is to be
 *                   stored
 */
static void build_local_mac_address(struct ethernet_mac_address *mac_addr_p)
{
    uint32_t reg_value;

    /*
     * Build MAC address from SoC's unique hardware identifier:
     */
    reg_value = READ_MMIO_REGISTER(&SIM_UIDML);
    mac_addr_p->bytes[0] = (uint8_t)((uint16_t)reg_value >> 8);
    mac_addr_p->bytes[1] = (uint8_t)reg_value;
    reg_value = READ_MMIO_REGISTER(&SIM_UIDL);
    mac_addr_p->bytes[2] = (uint8_t)(reg_value >> 24);
    mac_addr_p->bytes[3] = (uint8_t)(reg_value >> 16);
    mac_addr_p->bytes[4] = (uint8_t)((uint16_t)reg_value >> 8);
    mac_addr_p->bytes[5] = (uint8_t)reg_value;

    /*
     * Ensure special bits of first byte of the MAC address are properly
     * set:
     */
    mac_addr_p->bytes[0] &= ~MAC_MULTICAST_ADDRESS_MASK;
    mac_addr_p->bytes[0] |= MAC_PRIVATE_ADDRESS_MASK;
}


/**
 * Packet receive processing thread for a given Layer-2 end point
 */
static void net_layer2_packet_receiver_task(void *arg)
{
    struct net_layer2_end_point *const layer2_end_point_p =
        (struct net_layer2_end_point *)arg;

#   ifdef USE_MPU
    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                NULL);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);

    for ( ; ; ) {
        struct network_packet *rx_packet_p = NULL;
        bool frame_dropped = false;

        net_layer2_dequeue_rx_packet(layer2_end_point_p, &rx_packet_p);

        D_ASSERT(rx_packet_p != NULL);
        D_ASSERT(rx_packet_p->layer2_end_point_p == layer2_end_point_p);

        if (rx_packet_p->state_flags == NET_PACKET_RX_FAILED) {
            net_recycle_rx_packet(rx_packet_p);
            continue;
        }

        struct ethernet_frame *rx_frame_p =
            (struct ethernet_frame *)rx_packet_p->data_buffer;

        if (g_net_layer2.tracing_on) {
            DEBUG_PRINTF("Net layer2: Ethernet frame received:\n"
                         "\tsource MAC address %02x:%02x:%02x:%02x:%02x:%02x\n"
                         "\tdestination MAC address %02x:%02x:%02x:%02x:%02x:%02x\n"
                         "\tFrame type %#x, Total length: %u\n",
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[0],
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[1],
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[2],
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[3],
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[4],
                         rx_frame_p->ethernet_header.source_mac_addr.bytes[5],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[0],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[1],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[2],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[3],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[4],
                         rx_frame_p->ethernet_header.dest_mac_addr.bytes[5],
                         rx_frame_p->ethernet_header.frame_type,
                         rx_packet_p->total_length);
        }

        switch (ntoh16(rx_frame_p->ethernet_header.frame_type)) {
        case FRAME_TYPE_ARP_PACKET:
            net_layer3_receive_arp_packet(rx_packet_p);
            break;
        case FRAME_TYPE_IPv4_PACKET:
            net_layer3_receive_ipv4_packet(rx_packet_p);
            break;
        case FRAME_TYPE_IPv6_PACKET:
            net_layer3_receive_ipv6_packet(rx_packet_p);
            break;
        default:
            ERROR_PRINTF("Received frame of unknown type: %#x\n",
                          ntoh16(rx_frame_p->ethernet_header.frame_type));

            net_recycle_rx_packet(rx_packet_p);
            frame_dropped = true;
        }

        if (frame_dropped) {
			ATOMIC_POST_INCREMENT_UINT32(&g_net_layer2.rx_packets_dropped_count);
        } else {
			ATOMIC_POST_INCREMENT_UINT32(&g_net_layer2.rx_packets_accepted_count);
        }
    }

    ERROR_PRINTF("task %s should not have terminated\n",
                 rtos_task_self()->tsk_name_p);
}


/**
 * Initialize a given layer-2 end point
 *
 * @param layer2_end_point_p    Pointer to layer-2 end point
 */
static void net_layer2_end_point_init(struct net_layer2_end_point *layer2_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(!layer2_end_point_p->initialized);

    build_local_mac_address(&layer2_end_point_p->mac_address);

    INFO_PRINTF("Net layer2: Generated MAC address %02x:%02x:%02x:%02x:%02x:%02x for MAC %s\n",
                layer2_end_point_p->mac_address.bytes[0],
                layer2_end_point_p->mac_address.bytes[1],
                layer2_end_point_p->mac_address.bytes[2],
                layer2_end_point_p->mac_address.bytes[3],
                layer2_end_point_p->mac_address.bytes[4],
                layer2_end_point_p->mac_address.bytes[5],
                layer2_end_point_p->ethernet_mac_p->name_p);

    net_packet_queue_init("Layer-2 Rx network packet queue", false,
                          &layer2_end_point_p->rx_packet_queue);

    /*
     * Initialize Rx packets:
     */
    for (unsigned int i = 0;
         i < ARRAY_SIZE(layer2_end_point_p->rx_packets);
         i ++) {
        struct network_packet *rx_packet_p = &layer2_end_point_p->rx_packets[i];

        rx_packet_p->signature = NET_RX_PACKET_SIGNATURE;
        rx_packet_p->state_flags = 0;
        rx_packet_p->rx_buf_desc_p = NULL;
        rx_packet_p->layer2_end_point_p = layer2_end_point_p;
        rx_packet_p->queue_p = NULL;
        rx_packet_p->next_p = NULL;
    }

    ethernet_mac_init(layer2_end_point_p->ethernet_mac_p,
                      layer2_end_point_p);

    layer2_end_point_p->initialized = true;

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Start packet reception for a given layer-2 end point
 *
 * @param layer2_end_point_p    Pointer to layer-2 end point
 */
static void net_layer2_end_point_start(struct net_layer2_end_point *layer2_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer2_end_point_p->initialized);

    /*
     * Start packet reception at the Ethernet MAC:
     *
     * NOTE: Packets can start arriving right after this call. If so,
     * they will sit in the layer-2 end point's incoming queue.
     */
    ethernet_mac_start(layer2_end_point_p->ethernet_mac_p);

    /*
     * Create layer-2 packet receiver task:
     */
    rtos_task_create(&layer2_end_point_p->packet_receiver_task,
                     "Networking layer-2 packet receiver task",
                     net_layer2_packet_receiver_task,
                     layer2_end_point_p,
                     HIGHEST_APP_TASK_PRIORITY + 2);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Tell if the Ethernet link is up for a given Ethernet port
 *
 * @param layer2_end_point_p	Pointer to layer-2 end point
 *
 * @return true, link is up
 * @return false, link is down
 */
bool net_layer2_end_point_link_is_up(
            const struct net_layer2_end_point *layer2_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer2_end_point_p->initialized);

    const struct ethernet_mac_device *ethernet_mac_p =
        layer2_end_point_p->ethernet_mac_p;

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    bool link_up = ethernet_phy_link_is_up(ethernet_mac_p->ethernet_phy_p);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif

    return link_up;
}


/**
 * Set loopback mode on/off for a given given Ethernet port
 *
 * @param layer2_end_point_p	Pointer to layer-2 end point
 * @param on					boolean flag: true (on), false (off)
 */
void net_layer2_end_point_set_loopback(
	const struct net_layer2_end_point *layer2_end_point_p,
	bool on)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer2_end_point_p->initialized);

    const struct ethernet_mac_device *ethernet_mac_p =
        layer2_end_point_p->ethernet_mac_p;

    D_ASSERT(ethernet_mac_p->signature == ETHERNET_MAC_DEVICE_SIGNATURE);

    ethernet_phy_set_loopback(ethernet_mac_p->ethernet_phy_p, on);

    INFO_PRINTF("Layer2: Set loopback mode %s for MAC %s\n",
    		    on ? "on" : "off",
    		    ethernet_mac_p->name_p);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Obtain the MAC address associated with a layer-2 end point
 *
 * @param layer2_end_point_p    Pointer to Layer-2 end point
 * @param mac_addr_p            Area where MAC address is to be returned
 */
void net_layer2_get_mac_addr(const struct net_layer2_end_point *layer2_end_point_p,
                             struct ethernet_mac_address *mac_addr_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);

    rtos_thread_set_tmp_region(mac_addr_p, sizeof *mac_addr_p, 0);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(layer2_end_point_p->initialized);

    COPY_MAC_ADDRESS(mac_addr_p, &layer2_end_point_p->mac_address);

#   ifdef USE_MPU
    rtos_thread_unset_tmp_region();
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Initializes global pool of free Tx packets
 */
static void net_layer2_init_tx_packet_pool(struct net_tx_packet_pool *tx_packet_pool_p)
{
    net_packet_queue_init("Tx packet pool",
                          false,
                          &tx_packet_pool_p->free_list);

    for (unsigned int i = 0;
         i < ARRAY_SIZE(tx_packet_pool_p->tx_packets);
         i ++) {
        struct network_packet *const tx_packet_p =
            &tx_packet_pool_p->tx_packets[i];

        tx_packet_p->signature = NET_TX_PACKET_SIGNATURE;
        tx_packet_p->state_flags = NET_PACKET_IN_TX_POOL;
        tx_packet_p->tx_buf_desc_p = NULL;
        tx_packet_p->layer2_end_point_p = NULL;
        tx_packet_p->queue_p = NULL;
        tx_packet_p->next_p = NULL;

        net_packet_queue_add(&tx_packet_pool_p->free_list, tx_packet_p);
    }
}


/**
 * Global initialization of networking layer 2
 */
void net_layer2_init(void)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer2,
                                sizeof g_net_layer2,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(!g_net_layer2.initialized);
    net_layer2_init_tx_packet_pool(&g_net_layer2.free_tx_packet_pool);

#   ifdef USE_MPU
    /*
     * Enable access to Rx/Tx buffers memory for the ENET DMA engine:
     */
    ethernet_mac_register_dma_region(&g_net_layer2, sizeof g_net_layer2);
#   endif

    for (unsigned int i = 0;
         i < ARRAY_SIZE(g_net_layer2.local_layer2_end_points);
         i ++) {
        net_layer2_end_point_init(&g_net_layer2.local_layer2_end_points[i]);
    }

    g_net_layer2.initialized = true;

    DEBUG_PRINTF("Networking layer 2 initialized\n");

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}



/**
 * Start packet reception for networking layer 2
 */
void net_layer2_start(void)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer2,
                                sizeof g_net_layer2,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(g_net_layer2.initialized);

    for (unsigned int i = 0;
         i < ARRAY_SIZE(g_net_layer2.local_layer2_end_points);
         i ++) {
        net_layer2_end_point_start(&g_net_layer2.local_layer2_end_points[i]);
    }

    g_net_layer2.initialized = true;

    DEBUG_PRINTF("Networking layer 2 started\n");

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Allocates a Tx packet from layer-2's global Tx packet pool.
 * If there are no free Tx packets, it waits until one becomes available
 */
struct network_packet *net_layer2_allocate_tx_packet(bool free_after_tx_complete)
{
    struct network_packet *tx_packet_p = NULL;
    struct net_tx_packet_pool *const free_tx_packet_pool_p =
         &g_net_layer2.free_tx_packet_pool;

    D_ASSERT(CALLER_IS_THREAD());

#    ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer2,
                                sizeof g_net_layer2,
                                0,
                                &old_comp_region);
#    endif

    D_ASSERT(g_net_layer2.initialized);

    tx_packet_p = net_packet_queue_remove(&free_tx_packet_pool_p->free_list, 0);

    D_ASSERT(tx_packet_p->signature == NET_TX_PACKET_SIGNATURE);
    D_ASSERT(tx_packet_p->state_flags == NET_PACKET_IN_TX_POOL);
    D_ASSERT(tx_packet_p->tx_buf_desc_p == NULL);

    tx_packet_p->state_flags = NET_PACKET_IN_TX_USE_BY_APP;
    if (free_after_tx_complete) {
        tx_packet_p->state_flags |= NET_PACKET_FREE_AFTER_TX_COMPLETE;
    }

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
    return tx_packet_p;
}


/**
 * Frees a Tx packet back to the global Tx packet pool
 */
void net_layer2_free_tx_packet(struct network_packet *tx_packet_p)
{
    struct net_tx_packet_pool *const free_tx_packet_pool_p =
         &g_net_layer2.free_tx_packet_pool;

#    ifdef USE_MPU
    bool comp_region_changed = false;
    struct mpu_region_range old_comp_region;

    if (CALLER_IS_THREAD()) {
        rtos_thread_set_comp_region(free_tx_packet_pool_p,
                                    sizeof *free_tx_packet_pool_p,
                                    0,
                                    &old_comp_region);
        comp_region_changed = true;
    }
#    endif

    D_ASSERT(g_net_layer2.initialized);
    D_ASSERT(tx_packet_p->signature == NET_TX_PACKET_SIGNATURE);
    D_ASSERT(tx_packet_p->state_flags == NET_PACKET_IN_TX_USE_BY_APP);
    D_ASSERT(tx_packet_p->tx_buf_desc_p == NULL);

    tx_packet_p->state_flags = NET_PACKET_IN_TX_POOL;
    net_packet_queue_add(&free_tx_packet_pool_p->free_list, tx_packet_p);

#    ifdef USE_MPU
    if (comp_region_changed) {
        rtos_thread_restore_comp_region(&old_comp_region);
    }
#    endif
}


/**
 * Dequeues a Rx packet from a given layer-2 end point's Rx packet queue
 */
void net_layer2_dequeue_rx_packet(
        struct net_layer2_end_point *layer2_end_point_p,
        struct network_packet **rx_packet_pp)
{
    struct network_packet *rx_packet_p = NULL;

    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);

    rx_packet_p = net_packet_queue_remove(&layer2_end_point_p->rx_packet_queue, 0);

    D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
    D_ASSERT(rx_packet_p->state_flags & NET_PACKET_IN_RX_QUEUE);
    D_ASSERT(rx_packet_p->rx_buf_desc_p == NULL);

    rx_packet_p->state_flags &= ~NET_PACKET_IN_RX_QUEUE;
    rx_packet_p->state_flags |= NET_PACKET_IN_RX_USE_BY_APP;
    *rx_packet_pp = rx_packet_p;

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Enqueue a Rx packet to a given layer-2 end point's Rx packet queue
 */
void net_layer2_enqueue_rx_packet(
        struct net_layer2_end_point *layer2_end_point_p,
        struct network_packet *rx_packet_p)
{
#   ifdef USE_MPU
    bool comp_region_changed = false;
    struct mpu_region_range old_comp_region;

    if (CALLER_IS_THREAD()) {
        rtos_thread_set_comp_region(layer2_end_point_p,
                                    sizeof *layer2_end_point_p,
                                    0,
                                    &old_comp_region);
        comp_region_changed = true;
    }
#   endif

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
    D_ASSERT(rx_packet_p->state_flags == 0);
    D_ASSERT(rx_packet_p->rx_buf_desc_p == NULL);

    rx_packet_p->state_flags |= NET_PACKET_IN_RX_QUEUE;
    net_packet_queue_add(&layer2_end_point_p->rx_packet_queue, rx_packet_p);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Recycle a Rx packet for receiving another packet from the
 * corresponding layer-2 end point
 */
void net_recycle_rx_packet(struct network_packet *rx_packet_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer2_end_point_p,
                                sizeof *layer2_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    struct net_layer2_end_point *layer2_end_point_p = rx_packet_p->layer2_end_point_p;

    D_ASSERT(layer2_end_point_p->signature == NET_LAYER2_END_POINT_SIGNATURE);
    D_ASSERT(rx_packet_p->signature == NET_RX_PACKET_SIGNATURE);
    D_ASSERT(rx_packet_p->state_flags == NET_PACKET_IN_RX_USE_BY_APP);
    D_ASSERT(rx_packet_p->rx_buf_desc_p == NULL);

    ethernet_mac_repost_rx_packet(layer2_end_point_p->ethernet_mac_p, rx_packet_p);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


error_t net_layer2_send_ethernet_frame(
    const struct net_layer2_end_point *layer2_end_point_p,
    const struct ethernet_mac_address *dest_mac_addr_p,
    struct network_packet *tx_packet_p,
    uint16_t frame_type,
    size_t data_payload_length)
{
    struct ethernet_frame *tx_frame_p =
       (struct ethernet_frame *)tx_packet_p->data_buffer;

    /*
     * Populate Ethernet header:
     */

    /*
     * NOTE: The Ethernet MAC hardware populates the source MAC address
     * automatically in an outgoing frame
     */
#if 0
    ENET_COPY_MAC_ADDRESS(&tx_frame_p->enet_header.source_mac_addr,
                          &layer2_end_point_p->mac_address);
#endif

    COPY_MAC_ADDRESS(&tx_frame_p->ethernet_header.dest_mac_addr, dest_mac_addr_p);
    tx_frame_p->ethernet_header.frame_type = hton16(frame_type);

    tx_packet_p->total_length = sizeof(struct ethernet_header) +
                                data_payload_length;

    if (g_net_layer2.tracing_on) {
        DEBUG_PRINTF("Net layer2: Ethernet frame sent:\n"
                     "\tsource MAC address %02x:%02x:%02x:%02x:%02x:%02x\n"
                     "\tdestination MAC address %02x:%02x:%02x:%02x:%02x:%02x\n"
                     "\tFrame type %#x, Total length: %u\n",
                     layer2_end_point_p->mac_address.bytes[0],
                     layer2_end_point_p->mac_address.bytes[1],
                     layer2_end_point_p->mac_address.bytes[2],
                     layer2_end_point_p->mac_address.bytes[3],
                     layer2_end_point_p->mac_address.bytes[4],
                     layer2_end_point_p->mac_address.bytes[5],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[0],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[1],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[2],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[3],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[4],
                     tx_frame_p->ethernet_header.dest_mac_addr.bytes[5],
                     ntoh16(tx_frame_p->ethernet_header.frame_type),
                     tx_packet_p->total_length);
    }

    /*
     * Transmit packet:
     */
    ethernet_mac_start_xmit(layer2_end_point_p->ethernet_mac_p, tx_packet_p);
    ATOMIC_POST_INCREMENT_UINT32(&g_net_layer2.sent_packets_count);
    return 0;
}


void net_layer2_start_tracing(void)
{
    g_net_layer2.tracing_on = true;
}

void net_layer2_stop_tracing(void)
{
    g_net_layer2.tracing_on = false;
}
