/**
 * @file printf_utils.h
 *
 * Printf utils interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_BUILDING_BLOCKS_PRINTF_UTILS_H_
#define SOURCES_BUILDING_BLOCKS_PRINTF_UTILS_H_

#include <stdint.h>
#include <stdarg.h>

typedef void putchar_func_t(void *putchar_arg_p, uint8_t c);

void embedded_vprintf(
        putchar_func_t *putchar_func_p, void *putchar_arg_p,
        const char *fmt, va_list va);

void embedded_printf(
        putchar_func_t *putchar_func_p, void *putchar_arg_p,
        const char *fmt, ...);

#endif /* SOURCES_BUILDING_BLOCKS_PRINTF_UTILS_H_ */
