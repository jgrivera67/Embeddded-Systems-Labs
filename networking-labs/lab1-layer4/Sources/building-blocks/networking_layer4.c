/**
 * @file networking_layer4.c
 *
 * Networking layer 4 implementation: common part
 *
 * @author German Rivera
 */
#include "networking_layer4.h"
#include "net_layer4_end_point.h"
#include "runtime_checks.h"
#include "runtime_log.h"


/**
 * Networking layer-4 - global state variables
 */
struct net_layer4 g_net_layer4 = {
    .initialized = false,
    .tracing_on = false,
	.udp = {
		.rx_packets_accepted_count = 0,
		.rx_packets_dropped_count = 0,
		.sent_packets_over_ipv4_count = 0,
	},
};

/**
 * Global initialization of networking layer 3
 */
void net_layer4_init(void)
{
    D_ASSERT(CALLER_IS_THREAD());

#   ifdef USE_MPU
    struct mpu_region_range old_comp_region;

    rtos_thread_set_comp_region(&g_net_layer4,
                                sizeof g_net_layer4,
                                0,
                                &old_comp_region);
#   endif

    D_ASSERT(!g_net_layer4.initialized);

    net_layer4_udp_init(&g_net_layer4.udp);

    g_net_layer4.initialized = true;

    DEBUG_PRINTF("Networking layer 4 initialized\n");

#   ifdef USE_MPU
    rtos_thread_restore_comp_region(&old_comp_region);
#   endif
}


/**
 * Initializes a layer-4 end point
 *
 * @param layer4_end_point_p	Pointer to layer-4 end point to initialize
 * @param protocol				Transport protocol for the layer-4 end-point
 */
void net_layer4_end_point_init(struct net_layer4_end_point *layer4_end_point_p,
		                       enum net_layer4_protocols protocol)
{
	layer4_end_point_p->next_p = NULL;
	layer4_end_point_p->list_p = NULL;
    layer4_end_point_p->protocol = protocol;
    layer4_end_point_p->layer4_port = 0; /* unbound */
    net_packet_queue_init("Layer4 end point Rx packet queue", true,
                          &layer4_end_point_p->rx_packet_queue);
}


/**
 * Initializes a list of layer-4 end points
 *
 * @param list_p    Pointer to the list to be initialized
 */
void net_layer4_end_point_list_init(struct net_layer4_end_point_list *list_p,
                                    enum net_layer4_protocols protocol)
{
    list_p->protocol = protocol;
    list_p->length = 0;
    list_p->head_p = NULL;
    list_p->tail_p = NULL;
}


/**
 * Adds an element at the end of a layer-4 end-point list
 *
 * @param list_p    Pointer to the list
 * @param elem_p    Pointer to the element to be added
 *
 * NOTE: The caller is responsible for serializing access to the list.
 */
void net_layer4_end_point_list_add(struct net_layer4_end_point_list *list_p,
                                   struct net_layer4_end_point *elem_p)
{
    D_ASSERT(elem_p->protocol == list_p->protocol);
    D_ASSERT(elem_p->next_p == NULL);
    D_ASSERT(elem_p->list_p == NULL);
    D_ASSERT(elem_p->layer4_port != 0);

    struct net_layer4_end_point *old_tail_p = list_p->tail_p;
    list_p->tail_p = elem_p;
    if (_INFREQUENTLY_TRUE_(old_tail_p == NULL)) {
        list_p->head_p = elem_p;
    } else {
        old_tail_p->next_p = elem_p;
    }

    elem_p->list_p = list_p;
    list_p->length ++;
}


/**
 * Looks up an element with a given layer-4 port number in a list of layer-4
 * end points.
 *
 * @param list_p        Pointer to the list
 * @param layer4_port    Layer-4 port number to search for
 *
 * @return Pointer to layer-4 end point, if element found
 * @return NULL, if element not found
 *
 * NOTE: The caller is responsible for serializing access to the list.
 */
struct net_layer4_end_point *
net_layer4_end_point_list_lookup(struct net_layer4_end_point_list *list_p,
                                 uint_fast16_t layer4_port)
{
	uint_fast16_t elem_count = 0;

    for (struct net_layer4_end_point *next_p = list_p->head_p;
         next_p != NULL;
         next_p = next_p->next_p) {
        D_ASSERT(next_p->protocol == list_p->protocol);
        D_ASSERT(next_p->list_p == list_p);
        elem_count ++;
        D_ASSERT(elem_count <= list_p->length);

        if (next_p->layer4_port == layer4_port) {
            return next_p;
        }
    }

    return NULL;
}


/**
 * Removes an element from a layer-4 end-point list
 *
 * @param list_p    Pointer to the list
 * @param elem_p    Pointer to the element to be removed
 *
 * NOTE: The caller is responsible for serializing access to the list.
 */
void net_layer4_end_point_list_remove(struct net_layer4_end_point_list *list_p,
                                      struct net_layer4_end_point *elem_p)
{
    D_ASSERT(elem_p->protocol == list_p->protocol);
    D_ASSERT(elem_p->list_p == list_p);
    D_ASSERT(elem_p->layer4_port != 0);

    struct net_layer4_end_point *next_p;
    struct net_layer4_end_point *prev_p = NULL;

    for (next_p = list_p->head_p; next_p != NULL; next_p = next_p->next_p) {
        D_ASSERT(next_p->protocol == list_p->protocol);
        D_ASSERT(next_p->list_p == list_p);
    	if (next_p == elem_p) {
    		break;
    	}

    	prev_p = next_p;
    }

    if (next_p == NULL) {
    	ERROR_PRINTF("Element %#x not in list %#x\n", elem_p, list_p);
    	return;
    }

    D_ASSERT(next_p == elem_p);
	if (elem_p == list_p->head_p) {
	    D_ASSERT(prev_p == NULL);
		list_p->head_p = elem_p->next_p;
	} else {
	    D_ASSERT(prev_p != NULL);
		prev_p->next_p = elem_p->next_p;
    }

	elem_p->next_p = NULL;
	elem_p->list_p = NULL;
	if (elem_p == list_p->tail_p) {
		list_p->tail_p = prev_p;
	}

    list_p->length --;
}


void net_layer4_start_tracing(void)
{
	g_net_layer4.tracing_on = true;
}

void net_layer4_stop_tracing(void)
{
	g_net_layer4.tracing_on = false;
}
