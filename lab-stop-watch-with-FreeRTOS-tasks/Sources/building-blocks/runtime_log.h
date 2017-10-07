/**
 * @file runtime_log.h
 *
 * Runtime log interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_
#define SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_

#include <stdint.h>
#include "io_utils.h"
#include "compile_time_checks.h"

/*
 * Flags to control printing a message to a runtime log
 */

/**
 * Append a stack trace to the log message
 */
#define LOG_PRINT_STACK_TRACE    BIT(8)

/**
 * Include code address where the message originated
 */
#define LOG_PRINT_CODE_ADDR        BIT(9)

/**
 * Prints a debug message to the runtime debug log
 */
#if 0
#define DEBUG_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_DEBUG_LOG | LOG_PRINT_STACK_TRACE, \
                           _fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_DEBUG_LOG | LOG_PRINT_CODE_ADDR, \
                           _fmt, ##__VA_ARGS__)
#endif

/**
 * Prints a debug message with stack trace to the runtime debug log
 */
#define DEBUG_PRINTF_STACK_TRACE(_fmt, ...) \
        runtime_log_printf(RUNTIME_DEBUG_LOG | LOG_PRINT_STACK_TRACE, \
                           _fmt, ##__VA_ARGS__)

/**
 * Prints an error message to the runtime error log
 */
#define ERROR_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_ERROR_LOG | LOG_PRINT_STACK_TRACE,    \
                           _fmt, ##__VA_ARGS__)

/**
 * Prints an informational message to the runtime error log
 */
#define INFO_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_INFO_LOG, _fmt, ##__VA_ARGS__)

/**
 * Runtime logs
 */
enum runtime_logs {
    RUNTIME_DEBUG_LOG = 0,
    RUNTIME_ERROR_LOG,
    RUNTIME_INFO_LOG,

    /*
     * Last entry reserved for number of entries in the enum
     */
    NUM_RUNTIME_LOGS
};

C_ASSERT(NUM_RUNTIME_LOGS <= UINT8_MAX + 1);

void init_runtime_logs(void);

void runtime_log_printf(uint32_t log_and_flags, const char *fmt, ...);

void runtime_log_dump(enum runtime_logs log, uint_fast8_t max_screen_lines);

void runtime_log_dump_tail(enum runtime_logs log, uint_fast16_t num_tail_lines,
                           uint_fast8_t max_screen_lines);

#endif /* SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_ */
