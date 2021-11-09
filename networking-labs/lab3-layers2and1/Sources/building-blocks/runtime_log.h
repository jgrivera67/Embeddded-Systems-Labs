/**
 * @file runtime_log.h
 *
 * Runtime log interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_
#define SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_

/**
 * Prints a debug message to the runtime debug log
 */
#define DEBUG_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_DEBUG_LOG, _fmt, ##__VA_ARGS__)

/**
 * Prints an error message to the runtime error log
 */
#define ERROR_PRINTF(_fmt, ...) \
        runtime_log_printf(RUNTIME_ERROR_LOG, _fmt, ##__VA_ARGS__)

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

void init_runtime_logs(void);

void runtime_log_printf(enum runtime_logs log, const char *fmt, ...);

void runtime_log_dump(enum runtime_logs log);

#endif /* SOURCES_BUILDING_BLOCKS_RUNTIME_LOG_H_ */
