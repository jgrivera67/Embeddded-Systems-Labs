/**
 * @file network_packet.c
 *
 * Network packet services implementation
 *
 * @author German Rivera
 */
#include "network_packet.h"
#include "atomic_utils.h"

static void check_net_packet_queue_invariants(struct net_packet_queue *queue_p)
{
    if (_INFREQUENTLY_TRUE_(queue_p->head_p == NULL)) {
        D_ASSERT(queue_p->tail_p == NULL);
    } else {
        D_ASSERT(queue_p->tail_p != NULL);
        struct network_packet *head_packet_p = queue_p->head_p;
        struct network_packet *tail_packet_p = queue_p->tail_p;

        D_ASSERT(head_packet_p->signature == NET_TX_PACKET_SIGNATURE ||
                 head_packet_p->signature == NET_RX_PACKET_SIGNATURE);
        if (tail_packet_p != head_packet_p) {
            D_ASSERT(head_packet_p->next_p != NULL);
            D_ASSERT(tail_packet_p->signature == head_packet_p->signature);
        }

        D_ASSERT(tail_packet_p->next_p == NULL);
    }
}


/**
 * Initializes a network packet queue
 *
 * @param name_p     Name for the queue
 * @param use_mutex  Flag indicating if a mutex is to be used to serialize
 *                   concurrent accesses to the queue. If false, serialization
 *                   will be done by disabling interrupts.
 * @param queue_p    Pointer to the queue to be initialized
 */
void net_packet_queue_init(const char *name_p,
                           bool use_mutex,
                           struct net_packet_queue *queue_p)
{
    queue_p->signature = NET_PACKET_QUEUE_SIGNATURE;
    queue_p->length = 0;
    queue_p->length_high_water_mark = 0;
    queue_p->head_p = NULL;
    queue_p->tail_p = NULL;
    queue_p->use_mutex = use_mutex;
    if (use_mutex) {
        rtos_mutex_init(&queue_p->mutex, name_p);
    }

    rtos_semaphore_init(&queue_p->semaphore, name_p, 0);
    check_net_packet_queue_invariants(queue_p);
}


/**
 * Adds an element at the end of a network packet queue
 *
 * @param queue_p    Pointer to the queue
 * @param packet_p    Pointer to the packet to be added
 */
void net_packet_queue_add(struct net_packet_queue *queue_p,
                          struct network_packet *packet_p)
{
    bool use_mutex = queue_p->use_mutex;
    uint32_t int_mask;

    D_ASSERT(queue_p->signature == NET_PACKET_QUEUE_SIGNATURE);
    if (use_mutex) {
        rtos_mutex_lock(&queue_p->mutex);
    } else {
        int_mask = disable_cpu_interrupts();
    }

    check_net_packet_queue_invariants(queue_p);
    D_ASSERT(packet_p->signature == NET_RX_PACKET_SIGNATURE ||
             packet_p->signature == NET_TX_PACKET_SIGNATURE);
    D_ASSERT(NET_PACKET_NOT_IN_QUEUE(packet_p));

    struct network_packet *old_tail_packet_p = queue_p->tail_p;

    queue_p->tail_p = packet_p;
    if (_INFREQUENTLY_TRUE_(old_tail_packet_p == NULL)) {
        queue_p->head_p = packet_p;
    } else {
        old_tail_packet_p->next_p = packet_p;
    }

    packet_p->queue_p = queue_p;
    queue_p->length ++;
    if (queue_p->length > queue_p->length_high_water_mark) {
    	queue_p->length_high_water_mark = queue_p->length;
    }

    if (use_mutex) {
        rtos_mutex_unlock(&queue_p->mutex);
    } else {
        restore_cpu_interrupts(int_mask);
    }

    rtos_semaphore_signal(&queue_p->semaphore);
}


/**
 * Removes the packet from the head of a network packet queue, if the queue
 * is not empty. Otherwise, it waits until the queue becomes non-empty.
 * If timeout_ms is not 0, The wait will timeout at the specified
 * milliseconds value.
 *
 * @param queue_p        Pointer to the queue
 * @param timeout_ms    0, or timeout (in milliseconds) for waiting for the
 *                      queue to become non-empty
 *
 * @return pointer to packet removed from the queue, or NULL if timeout
 */
struct network_packet *net_packet_queue_remove(struct net_packet_queue *queue_p,
                                               uint32_t timeout_ms)
{
    uint32_t int_mask;
    struct network_packet *head_packet_p = NULL;
    bool use_mutex = queue_p->use_mutex;

    for ( ; ; ) {
        D_ASSERT(queue_p->signature == NET_PACKET_QUEUE_SIGNATURE);
        if (timeout_ms != 0) {
            bool sem_signaled = rtos_semaphore_wait_timeout(&queue_p->semaphore,
                                                            timeout_ms);
            if (!sem_signaled) {
                return NULL;
            }
        } else {
            rtos_semaphore_wait(&queue_p->semaphore);
        }

        if (use_mutex) {
            rtos_mutex_lock(&queue_p->mutex);
        } else {
            int_mask = disable_cpu_interrupts();
        }

        check_net_packet_queue_invariants(queue_p);
        head_packet_p = queue_p->head_p;
        if (_INFREQUENTLY_FALSE_(head_packet_p != NULL)) {
            break;
        }

        /*
         * Another thread got ahead of us and got the packet,
         * so try again.
         */
        if (use_mutex) {
            rtos_mutex_unlock(&queue_p->mutex);
        } else {
            restore_cpu_interrupts(int_mask);
        }
    }

    D_ASSERT(head_packet_p != NULL);
    D_ASSERT(head_packet_p->queue_p == queue_p);
    D_ASSERT(head_packet_p->signature == NET_RX_PACKET_SIGNATURE ||
             head_packet_p->signature == NET_TX_PACKET_SIGNATURE);

    queue_p->head_p = head_packet_p->next_p;
    head_packet_p->next_p = NULL;
    head_packet_p->queue_p = NULL;

    if (_INFREQUENTLY_TRUE_(queue_p->head_p == NULL)) {
        D_ASSERT(queue_p->tail_p == head_packet_p);
        queue_p->tail_p = NULL;
    }

    queue_p->length --;
    if (use_mutex) {
        rtos_mutex_unlock(&queue_p->mutex);
    } else {
        restore_cpu_interrupts(int_mask);
    }

    D_ASSERT(NET_PACKET_NOT_IN_QUEUE(head_packet_p));
    return head_packet_p;
}
