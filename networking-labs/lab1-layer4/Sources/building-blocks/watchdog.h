/**
 * @file watchdog.h
 *
 * Hardware watchdog timer interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_WATCHDOG_H_
#define SOURCES_BUILDING_BLOCKS_WATCHDOG_H_

#include <stdint.h>
#include "io_utils.h"

/*
 * Liveness events masks
 *
 * Define event masks here. For example:
 *
 * #define WDOG_LIVENESS_EVENT_XXX	BIT(0)
 */

void watchdog_init(void);

void watchdog_restart(void);

void watchdog_expect_liveness_event(uint32_t event_mask);

void watchdog_signal_liveness_event(uint32_t event_mask);

void watchdog_get_before_reset_info(uint32_t *old_expected_liveness_events_p,
                                    uint32_t *old_signaled_liveness_events_p);

#endif /* SOURCES_BUILDING_BLOCKS_WATCHDOG_H_ */
