/**
 * @file event_set.c
 *
 * Primitives to atomically add and remove events to an event set
 *
 * @author: German Rivera
 */
#include "event_set.h"
#include "atomic_utils.h"
#include "io_utils.h"
#include "runtime_checks.h"

/**
 * Initializes the given event set as empty
 */
void init_event_set(struct event_set *event_set_p)
{
    event_set_p->elements = 0x0;
}


/**
 * Tell if the given event set is empty
 *
 * @param event_set_p pointer to the event set
 */
bool is_event_set_empty(const struct event_set *event_set_p) {

	/*
	 * NOTE: We need to do a type cast to remove the const, since
	 * atomic_fecth_or_uint32() changes the first arg. However, since or-ing
	 * with 0 is a "no-op" the value of the set stays the same.
	 */
    uint32_t old_event_set =
        atomic_fetch_or_uint32((uint32_t *)&event_set_p->elements, 0x0);

    return (old_event_set == 0x0);
}


/**
 * Add an event to the event set, if not already in it.
 * Otherwise, it does not add it again (as sets in mathematics cannot
 * have duplicates).
 *
 * @param event_set_p pointer to the event set
 * @param event_index Index of the event to add to the event set
 *
 * @return true, if event added
 * @return false, otherwise
 */
bool test_and_set_event(struct event_set *event_set_p,
                        uint8_t event_index)
{
    uint32_t old_event_set;

    D_ASSERT(event_index < MAX_NUM_EVENTS);
    old_event_set = atomic_fetch_or_uint32(&event_set_p->elements,
    		                               BIT(event_index));

    return (old_event_set & BIT(event_index)) == 0;
}


/**
 * Remove an event from the event set, if the event is in the set.
 * Otherwise, it does not do anything.
 *
 * @param event_set_p pointer to the event set
 * @param event_index Index of the event to add to the event set
 *
 * @return true, if event removed
 * @return false, otherwise
 */
bool test_and_clear_event(struct event_set *event_set_p,
                          uint8_t event_index)
{
    uint32_t old_event_set;

    D_ASSERT(event_index < MAX_NUM_EVENTS);
    old_event_set = atomic_fetch_and_uint32(
                            &event_set_p->elements,
                            ~BIT(event_index));

    return (old_event_set & BIT(event_index)) != 0;
}

