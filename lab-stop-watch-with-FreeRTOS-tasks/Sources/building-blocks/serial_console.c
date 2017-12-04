/**
 * @file serial_console.c
 *
 * Serial console services implementation
 *
 * @author German Rivera
 */
#include "serial_console.h"
#include "runtime_checks.h"
#include "rtos_wrapper.h"
#include "byte_ring_buffer.h"
#include "uart_driver.h"
#include "microcontroller.h"
#include <stdarg.h>
#include "printf_utils.h"
#include "memory_protection_unit.h"

#include "fsl_lpuart.h" //??? TODO
#define uart_putchar(_uart_device_p, _c) \
	do { \
		uint8_t _x = (_c); \
		LPUART_WriteBlocking(LPUART0, &_x, 1); \
	} while (0)
//???

/**
 * Baud rate for the console UART
 */
#define CONSOLE_UART_BAUD       115200

/**
 * Size (in bytes) of the console output ring buffer
 */
#define CONSOLE_OUTPUT_BUFFER_SIZE    256

/**
 * Invalid screen attributes value
 */
#define CONSOLE_ATTR_INVALID    UINT32_MAX

/**
 * State variables of a serial console
 */
struct serial_console {
    bool initialized;
    bool do_async_output;
    uint32_t current_attributes;
    uint32_t saved_attributes;
    const struct uart_device *const uart_device_p;
    struct byte_ring_buffer output_buffer;
    uint8_t output_buffer_data[CONSOLE_OUTPUT_BUFFER_SIZE];

    /**
     * Mutex to serialize access to the serial console from multiple tasks
     */
    struct rtos_mutex mutex;
} __attribute__ ((aligned(2 * MPU_REGION_ALIGNMENT)));

static struct serial_console g_console = {
    .initialized = false,
    .do_async_output = false,
    .current_attributes = CONSOLE_ATTR_NORMAL,
    .saved_attributes = CONSOLE_ATTR_INVALID,
    //.uart_device_p = &g_uart_devices[0], //??? TODO
};

/*
 * Control character strings
 */
#define ESC    "\x1b"    /* escape */
#define SO    "\x0e"  /* shift out */
#define SI    "\x0f"  /* shift in */

#define ENTER_LINE_DRAWING_MODE    SO
#define EXIT_LINE_DRAWING_MODE    SI

/*
 * Line drawing characters:
 */
#define UPPER_LEFT_CORNER    '\x6c'
#define LOWER_LEFT_CORNER    '\x6d'
#define UPPER_RIGHT_CORNER    '\x6b'
#define LOWER_RIGHT_CORNER    '\x6a'
#define VERTICAL_LINE        '\x78'
#define HORIZONTAL_LINE        '\x71'


static void console_uart_putchar(uint8_t c)
{
    if (c == '\n') {
        uart_putchar(g_console.uart_device_p, '\r');
        uart_putchar(g_console.uart_device_p, '\n');
    } else {
        uart_putchar(g_console.uart_device_p, c);
    }
}


/**
 * Console output task. It reads characters from the
 * console output buffer in FIFO order and send them to
 * the UART.
 */
static void console_output_task_func(void *arg)
{
    uint8_t c;

    D_ASSERT(arg == NULL);
    D_ASSERT(g_console.do_async_output);
    for ( ; ; ) {
        c = byte_ring_buffer_read(&g_console.output_buffer);
        console_uart_putchar(c);
    }
}


/**
 * Initializes serial console
 *
 *@param console_output_task_p pointer to an RTOS task object to be
 *       used to create a task to handle console output. Use NULL
 *       if asynchronous console output is not wanted.
 *
 * NOTE: For now we leverage KSDK code for the UART initialization.
 * In the future, we will write our own code for this.
 */
void console_init(struct rtos_task *console_output_task_p)
{
	struct mpu_region_descriptor old_region;

    D_ASSERT(!g_console.initialized);

#if 0 //??? TODO
    uart_init(g_console.uart_device_p, CONSOLE_UART_BAUD, UART_DEFAULT_MODE);
#endif

    rtos_mutex_init(&g_console.mutex, "serial console mutex");
    set_private_data_region(&g_console, sizeof(g_console), READ_WRITE, &old_region);
    g_console.initialized = true;

    /*
     * Initialize async console output:
     */
    if (console_output_task_p != NULL) {
        D_ASSERT(!console_output_task_p->tsk_created);

        /*
         * Initialize console output buffer:
         */
        byte_ring_buffer_init(&g_console.output_buffer,
                              g_console.output_buffer_data,
                              sizeof g_console.output_buffer_data);

        g_console.do_async_output = true;

        /*
         * Create console output task:
         */
        rtos_task_create(console_output_task_p,
                         "Console output task",
                         console_output_task_func,
                         NULL,
                         LOWEST_APP_TASK_PRIORITY);
    }

    restore_private_data_region(&old_region);
}


/**
 * Acquire console mutex
 */
void console_lock(void)
{
    rtos_mutex_lock(&g_console.mutex);
}


/**
 * Release console mutex
 */
void console_unlock(void)
{
    rtos_mutex_unlock(&g_console.mutex);
}

bool console_is_locked(void)
{
    return rtos_mutex_is_mine(&g_console.mutex);
}

/**
 * Send a character to the UART
 */
void console_putchar(int c)
{
    D_ASSERT(g_console.initialized);

    if (g_console.do_async_output && CPU_INTERRUPTS_ARE_ENABLED()) {
        byte_ring_buffer_write(&g_console.output_buffer, c);
    } else {
        console_uart_putchar(c);
    }
}


/**
 * Send a string to the UART
 */
void console_puts(const char *s)
{
    while (*s != '\0') {
        console_putchar(*s);
        s ++;
    }
}


/**
 * Callback function invoked from embedded_vprintf()
 */
static void putchar_callback(void *putchar_arg_p, uint8_t c)
{
    D_ASSERT(putchar_arg_p == NULL);
    if (g_console.do_async_output && CPU_INTERRUPTS_ARE_ENABLED()) {
        byte_ring_buffer_write(&g_console.output_buffer, c);
    } else {
    	uart_putchar(g_console.uart_device_p, c);
    }
}


/**
 * printf service that sends output to the serial console, at
 * the current cursor position
 */
void console_printf(const char  *fmt_s, ...)
{
    va_list  va;

    D_ASSERT(g_console.initialized);
    va_start(va, fmt_s);
    embedded_vprintf(putchar_callback, NULL, fmt_s, va);
    va_end(va);
}


/**
 * Save current cursor and attributes
 */
void console_save_cursor_and_attributes(void)
{
	struct mpu_region_descriptor old_region;

    /*
     * Send VT100 control sequence to save current cursor and attributes:
     */
    console_puts(ESC "7");

    set_private_data_region(&g_console, sizeof(g_console), READ_WRITE, &old_region);
    g_console.saved_attributes = g_console.current_attributes;
    restore_private_data_region(&old_region);
}


/**
 * Restore previously saved cursor and attributes
 */
void console_restore_cursor_and_attributes(void)
{
	struct mpu_region_descriptor old_region;

    D_ASSERT(g_console.saved_attributes != CONSOLE_ATTR_INVALID);

    /*
     * Send VT100 control sequence to restore saved cursor and attributes:
     */
    console_puts(ESC "8");

    set_private_data_region(&g_console, sizeof(g_console), READ_WRITE, &old_region);
    g_console.current_attributes = g_console.saved_attributes;
    g_console.saved_attributes = CONSOLE_ATTR_INVALID;
    restore_private_data_region(&old_region);
}


/**
 * Set cursor and attributes and save the old cursor and attributes
 */
void console_set_cursor_and_attributes(uint8_t line, uint8_t column,
                                       uint32_t attributes, bool save_old)
{
    if (save_old) {
        console_save_cursor_and_attributes();
    }

    /*
     * Send VT100 control sequence to position cursor:
     */
    console_printf(ESC "[%u;%uH", line, column);

    if (attributes != g_console.current_attributes) {
		struct mpu_region_descriptor old_region;

		set_private_data_region(&g_console, sizeof(g_console), READ_WRITE, &old_region);
        g_console.current_attributes = attributes;
		restore_private_data_region(&old_region);

        /*
         * Send VT100 control sequences to set text attributes:
         */
        if (attributes == CONSOLE_ATTR_NORMAL) {
            console_puts(ESC "[0m");
        } else {
            if (attributes & CONSOLE_ATTR_BOLD) {
                console_puts(ESC "[1m");
            }

            if (attributes & CONSOLE_ATTR_UNDERLINED) {
                console_puts(ESC "[4m");
            }

            if (attributes & CONSOLE_ATTR_BLINK) {
                console_puts(ESC "[5m");
            }

            if (attributes & CONSOLE_ATTR_REVERSE) {
                console_puts(ESC "[7m");
            }
        }
    }
}


/**
 * Erase all characters in the current line.
 */
void console_erase_current_line(void)
{
    /*
     * Send VT100 control sequence
     */
    console_puts(ESC "[2K");
}


/**
 * Erase a range of lines
 *
 * @param top_line: First line of the range
 * @param bottom_line: last line of the range
 * @param preserve_cursor: flag to indicate if cursor need to be saved/restored
 */
void console_erase_lines(uint8_t top_line, uint8_t bottom_line, bool preserve_cursor)
{
    D_ASSERT(top_line >= 1);
    D_ASSERT(top_line <= bottom_line);
    if (preserve_cursor) {
        console_save_cursor_and_attributes();
    }

    console_set_cursor_and_attributes(top_line, 1, 0, false);
    for (uint_fast8_t line = top_line; line <= bottom_line; line ++) {
        console_erase_current_line();
        console_putchar('\n');
    }

    if (preserve_cursor) {
        console_restore_cursor_and_attributes();
    }
}


/**
 * printf service that sends output to the serial console, at
 * the specified (line, column) position.
 */
void console_pos_printf(uint8_t line, uint8_t column, uint32_t attributes,
                        const char  *fmt_s, ...)
{
    va_list  va;

    console_set_cursor_and_attributes(line, column, attributes, true);

    /*
     * Print formatted text:
     */
    va_start(va, fmt_s);
    embedded_vprintf(putchar_callback, NULL, fmt_s, va);
    va_end(va);

    console_restore_cursor_and_attributes();
}


void console_turn_off_cursor(void)
{
    console_puts(ESC "[?25l");
}

void console_turn_on_cursor(void)
{
    console_puts(ESC "[?25h");
}

/**
 * Clears the console screen and moves cursor to home
 */
void console_clear(void)
{
    /*
     * Send VT100 control sequences to:
     * - clear screen
     */
    console_puts(ESC "[2J" ESC "[H");

    /*
     * Send VT100 control sequences to:
     * - set the G0 character set as ASCII
     * - set the G1 character set as the Special Character and Line Drawing Set
     * - Select G0 as the current character set
     */
    console_puts(ESC "(B" ESC ")0" SI);

    console_turn_off_cursor();
}


/**
 * Set scroll region
 *
 * @param top_line: first line of the scroll region.
 *
 * @param bottom_line: Last line of the scroll region or 0, if
 * scroll region grows dynamically as the screen size grows.
 */
void console_set_scroll_region(uint8_t top_line, uint8_t bottom_line)
{
    /*
     * Send VT100 control sequence to set scroll region:
     */
    console_printf(ESC "[%u;%ur", top_line, bottom_line);
}


void console_draw_box(uint8_t line, uint8_t column, uint8_t height, uint8_t width,
                      uint32_t attributes)
{
    console_set_cursor_and_attributes(line, column, attributes, true);
    console_puts(ENTER_LINE_DRAWING_MODE);

    console_putchar(UPPER_LEFT_CORNER);
    for (uint8_t j = 0; j < width - 2; j++) {
        console_putchar(HORIZONTAL_LINE);
    }

    console_putchar(UPPER_RIGHT_CORNER);

    for (uint8_t i = 1; i < height - 1; i++) {
        console_set_cursor_and_attributes(line + i, column, 0, false);
        console_putchar(VERTICAL_LINE);
        console_set_cursor_and_attributes(line + i, column + width - 1, 0, false);
        console_putchar(VERTICAL_LINE);
    }

    console_set_cursor_and_attributes(line + height - 1, column, 0, false);
    console_putchar(LOWER_LEFT_CORNER);
    for (uint8_t j = 0; j < width - 2; j++) {
        console_putchar(HORIZONTAL_LINE);
    }

    console_putchar(LOWER_RIGHT_CORNER);

    console_puts(EXIT_LINE_DRAWING_MODE);
    console_restore_cursor_and_attributes();
}


void console_draw_horizontal_line(uint8_t line, uint8_t column, uint8_t width,
                                  uint32_t attributes)
{
    console_set_cursor_and_attributes(line, column, attributes, true);
    console_puts(ENTER_LINE_DRAWING_MODE);

    for (uint_fast8_t j = 0; j < width; j++) {
        console_putchar(HORIZONTAL_LINE);
    }

    console_puts(EXIT_LINE_DRAWING_MODE);
    console_restore_cursor_and_attributes();
}


/**
 * Reads the next character received on the console UART, waiting if necessary.
 *
 * If no character has been received, it waits until a character arrives
 * at the UART.
 *
 * @return: ASCII code of the character received
 */
int console_getchar(void)
{
    D_ASSERT(g_console.initialized);
    return uart_getchar(g_console.uart_device_p);
}

/**
 * Reads the next character received on the console UART, if any.
 *
 * If no character has been received, it returns right away with -1.
 * at the UART.
 *
 * NOTE: This function is to be used only if interrupts are disabled.
 *
 * @return: ASCII code of the character received, on success
 * @return: -1, if no character was available to read from the UART.
 */
int console_getchar_non_blocking(void)
{
    D_ASSERT(g_console.initialized);
#if 0 // TODO???
    return uart_getchar_non_blocking(g_console.uart_device_p);
#else
    struct mpu_region_descriptor old_region;
    int c;

    set_private_data_region(LPUART0, sizeof(*LPUART0), READ_ONLY, &old_region);
    if (LPUART0->STAT & LPUART_STAT_RDRF_MASK) {
    	c = (LPUART0->DATA & 0xff);
    } else {
    	c =-1;
    }

    restore_private_data_region(&old_region);
    return c;
#endif
}
