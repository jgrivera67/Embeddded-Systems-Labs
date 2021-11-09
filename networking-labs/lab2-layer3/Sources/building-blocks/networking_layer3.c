/**
 * @file networking_layer3.c
 *
 * Networking layer 3 implementation: common part
 *
 * @author German Rivera
 */
#include "networking_layer3.h"
#include "mem_utils.h"
#include "runtime_log.h"

/**
 * Networking layer-3 - global state variables
 */
struct net_layer3 g_net_layer3 = {
    .initialized = false,
    .tracing_on = false,
    .ipv4 = {
        .expecting_ping_reply = false,
        .rx_packets_accepted_count = 0,
        .rx_packets_dropped_count = 0,
        .sent_packets_count = 0,
    },
    .local_layer3_end_points = {
        [0] = {
            .signature = NET_LAYER3_END_POINT_SIGNATURE,
            .initialized = false,
            .layer2_end_point_p = &g_net_layer2.local_layer2_end_points[0],
            .ipv4 = {
                .local_ip_addr.value = IPV4_NULL_ADDR,
                .subnet_mask = 0x0,
                .default_gateway_ip_addr.value = IPV4_NULL_ADDR,
                .next_tx_ip_packet_seq_num = 0,
            },
            .ipv6 = {
                .flags = 0,
            }
        },
    },
};


/**
 * Initializes a layer-3 end point
 *
 * @param layer3_end_point_p Pointer to layer-3 end point
 */
static void net_layer3_end_point_init(struct net_layer3_end_point *layer3_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer3_end_point_p,
                                sizeof *layer3_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);
    D_ASSERT(!layer3_end_point_p->initialized);

    net_layer3_ipv4_end_point_init(&layer3_end_point_p->ipv4);
    net_layer3_ipv6_end_point_init(&layer3_end_point_p->ipv6);

    layer3_end_point_p->initialized = true;

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Start RTOS tasks for a layer-3 end point
 *
 * @param layer3_end_point_p    Pointer to layer-3 end point
 */
static void net_layer3_end_point_start_tasks(struct net_layer3_end_point *layer3_end_point_p)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(layer3_end_point_p,
                                sizeof *layer3_end_point_p,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(layer3_end_point_p->signature == NET_LAYER3_END_POINT_SIGNATURE);
    D_ASSERT(layer3_end_point_p->initialized);

    net_layer3_ipv4_end_point_start_tasks(&layer3_end_point_p->ipv4);
    net_layer3_ipv6_end_point_start_tasks(&layer3_end_point_p->ipv6);

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Global initialization of networking layer 3
 */
void net_layer3_init(void)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(!g_net_layer3.initialized);

    net_layer3_ipv4_init(&g_net_layer3.ipv4);
    net_layer3_ipv6_init(&g_net_layer3.ipv6);

    for (unsigned int i = 0;
         i < ARRAY_SIZE(g_net_layer3.local_layer3_end_points); i ++) {
        net_layer3_end_point_init(&g_net_layer3.local_layer3_end_points[i]);
    }

    g_net_layer3.initialized = true;

    DEBUG_PRINTF("Networking layer 3 initialized\n");

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Start RTOS tasks for networking layer 3
 */
void net_layer3_start_tasks(void)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer3,
                                sizeof g_net_layer3,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(g_net_layer3.initialized);

    /*
     * Start RTOS tasks for each layer-3 end point:
     */
    for (unsigned int i = 0;
         i < ARRAY_SIZE(g_net_layer3.local_layer3_end_points); i ++) {
        net_layer3_end_point_start_tasks(&g_net_layer3.local_layer3_end_points[i]);
    }

    g_net_layer3.initialized = true;

    DEBUG_PRINTF("Networking layer 3 tasks started\n");

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


void net_layer3_start_tracing(void)
{
    g_net_layer3.tracing_on = true;
}

void net_layer3_stop_tracing(void)
{
    g_net_layer3.tracing_on = false;
}
