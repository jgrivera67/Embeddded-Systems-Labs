/**
 * @file event_set.h
 *
 * Primitives to atomically add and remove events to an event set
 *
 * @author: German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_EVENT_SET_H_
#define SOURCES_BUILDING_BLOCKS_EVENT_SET_H_

#include <stdint.h>
#include <stdbool.h>

#define MAX_NUM_EVENTS (sizeof(uint32_t) * 8)

/**
 * Event set representation
 */
struct event_set {
    /**
     * Bit vector representing the events currently in the set of events.
     * Bit i is on in the bit vector, if event i is in the event set.
     */
    volatile uint32_t elements;
};

void init_event_set(struct event_set *event_set_p);

bool is_event_set_empty(const struct event_set *event_set_p);

bool test_and_set_event(struct event_set *event_set_p,
                        uint8_t event_index);

bool test_and_clear_event(struct event_set *event_set_p,
                          uint8_t event_index);

#endif /* SOURCES_BUILDING_BLOCKS_EVENT_SET_H_ */
