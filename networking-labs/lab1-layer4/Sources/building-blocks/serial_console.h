/**
 * @file serial_console.h
 *
 * Serial console services interface
 *
 * @author German Rivera
 */
#ifndef SOURCES_SERIAL_CONSOLE_H_
#define SOURCES_SERIAL_CONSOLE_H_

#include <stdint.h>
#include <stdbool.h>

/*
 * Values that can be or-ed to form the 'attributes'
 * argument of console_pos_printf
 */
#define CONSOLE_ATTR_NORMAL        0x0
#define CONSOLE_ATTR_BOLD          0x1
#define CONSOLE_ATTR_UNDERLINED    0x2
#define CONSOLE_ATTR_BLINK         0x4
#define CONSOLE_ATTR_REVERSE       0x8

/*
 * Incomplete struct declarations to avoid includes
 */
struct rtos_task;

void console_init(struct rtos_task *console_output_task_p);

void console_lock(void);

void console_unlock(void);

bool console_is_locked(void);

void console_putchar(int c);

void console_puts(const char *s);

void console_printf(const char *fmt_s, ...);

void console_pos_printf(uint8_t line, uint8_t column, uint32_t attributes,
                        const char *fmt_s, ...);

void console_pos_puts(uint8_t line, uint8_t column, uint32_t attributes,
                      const char *s);

void console_turn_off_cursor(void);

void console_turn_on_cursor(void);

void console_set_cursor_and_attributes(uint8_t line, uint8_t column,
                                       uint32_t attributes, bool save_old);

void console_restore_cursor_and_attributes(void);

void console_clear(void);

void console_set_scroll_region(uint8_t top_line, uint8_t bottom_line);

void console_draw_box(uint8_t line, uint8_t column, uint8_t height, uint8_t width,
                      uint32_t attributes);

int console_getchar(void);

int console_getchar_non_blocking(void);

#endif /* SOURCES_SERIAL_CONSOLE_H_ */
