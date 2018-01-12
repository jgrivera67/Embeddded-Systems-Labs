/**
 * @file printf_utils.c
 *
 * Printf utils implementation
 *
 * @author German Rivera
 */
#include "printf_utils.h"
#include <stddef.h>
#include <stdbool.h>
#include "io_utils.h"

/**
 * Get the most significant 4 bits of a given value
 */
#define GET_MSB_HEX_DIGIT(_value) \
        ((uint8_t)GET_BIT_FIELD(remaining_value, MSB_HEX_DIGIT_MASK, \
                                MSB_HEX_DIGIT_SHIFT))

static void
print_uint32_hexadecimal(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, uint32_t value,
    uint8_t padding_count)
{
#   define MAX_NUM_HEX_DIGITS   ((sizeof(uint32_t) * 8) / 4)
#   define MSB_BIT_INDEX        ((sizeof(uint32_t) * 8) - 1)
#   define MSB_HEX_DIGIT_MASK   MULTI_BIT_MASK(MSB_BIT_INDEX, MSB_BIT_INDEX - 3)
#   define MSB_HEX_DIGIT_SHIFT  (MSB_BIT_INDEX - 3)

    uint8_t    char_printed_count = 0;
    uint32_t   remaining_value = value;

    for (uint8_t i = 0; i < MAX_NUM_HEX_DIGITS; i ++) {
        uint8_t hex_digit = GET_MSB_HEX_DIGIT(remaining_value);

        remaining_value <<= 4;

        if (hex_digit == 0 && char_printed_count == 0) {
            continue;
        }

        D_ASSERT(hex_digit <= 0xf);

        if (hex_digit < 0xa) {
            putchar_func_p(putchar_arg_p, '0' + hex_digit);
        } else {
            putchar_func_p(putchar_arg_p, 'A' + (hex_digit - 0xa));
        }

        char_printed_count ++;
    }

    if (char_printed_count == 0) {
        putchar_func_p(putchar_arg_p, '0');
        char_printed_count ++;
    }

    while (char_printed_count < padding_count)
    {
        putchar_func_p(putchar_arg_p, ' ');
        char_printed_count ++;
    }

#   undef MAX_NUM_HEX_DIGITS
#   undef MSB_BIT_INDEX
#   undef MSB_HEX_DIGIT_MASK
#   undef MSB_HEX_DIGIT_SHIFT
}


static void
print_uint64_hexadecimal(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, uint64_t value,
    uint8_t padding_count)
{
#   define MAX_NUM_HEX_DIGITS   ((sizeof(uint64_t) * 8) / 4)
#   define MSB_BIT_INDEX        ((sizeof(uint64_t) * 8) - 1)
#   define MSB_HEX_DIGIT_MASK   MULTI_BIT_MASK64(MSB_BIT_INDEX, MSB_BIT_INDEX - 3)
#   define MSB_HEX_DIGIT_SHIFT  (MSB_BIT_INDEX - 3)

    uint8_t    char_printed_count = 0;
    uint64_t   remaining_value = value;

    for (uint8_t i = 0; i < MAX_NUM_HEX_DIGITS; i ++) {
        uint8_t hex_digit = GET_MSB_HEX_DIGIT(remaining_value);

        remaining_value <<= 4;

        if (hex_digit == 0 && char_printed_count == 0) {
            continue;
        }

        D_ASSERT(hex_digit <= 0xf);

        if (hex_digit < 0xa) {
            putchar_func_p(putchar_arg_p, '0' + hex_digit);
        } else {
            putchar_func_p(putchar_arg_p, 'A' + (hex_digit - 0xa));
        }

        char_printed_count ++;
    }

    if (char_printed_count == 0) {
        putchar_func_p(putchar_arg_p, '0');
        char_printed_count ++;
    }

    while (char_printed_count < padding_count)
    {
        putchar_func_p(putchar_arg_p, ' ');
        char_printed_count ++;
    }

#   undef MAX_NUM_HEX_DIGITS
#   undef MSB_BIT_INDEX
#   undef MSB_HEX_DIGIT_MASK
#   undef MSB_HEX_DIGIT_SHIFT
}


static void
print_string(putchar_func_t *putchar_func_p, void *putchar_arg_p, const char *str,
    uint8_t padding_count)
{
    D_ASSERT(str != NULL);

    const char *cursor_p = str;
    uint8_t char_printed_count = 0;

    while (*cursor_p != '\0')
    {
        uint8_t c = *cursor_p ++;

        putchar_func_p(putchar_arg_p, c);
        char_printed_count ++;
        if (char_printed_count == padding_count)
        {
            break;
        }
    }

    while (char_printed_count < padding_count)
    {
        putchar_func_p(putchar_arg_p, ' ');
        char_printed_count ++;
    }
}


static void
print_uint32_decimal(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, uint32_t value,
    uint8_t padding_count, bool prefix_with_zeros, bool left_justified)
{
    char buffer[16];
    char *p = &buffer[sizeof(buffer) - 1];

    *p = '\0';
    do {
        p--;
        if (p < buffer) {
            p++;
            *p = 'T'; /* for truncated */
            break;
        }

        /* p >= buffer */
        *p = (value % 10) + '0';
        value /= 10;
    } while (value > 0);

    /* p >= buffer */
    if (left_justified || padding_count == 0) {
        print_string(putchar_func_p, putchar_arg_p, p, padding_count);
    } else {
        uint_fast8_t digit_count = &buffer[sizeof(buffer) - 1] - p;

        /* digit_count >= 1 */
		if ( digit_count < padding_count) {
			int padding_char;
			padding_count -= digit_count;

			if (prefix_with_zeros) {
				padding_char = '0';
			} else {
				padding_char = ' ';
			}

			p--;
			while (p >= buffer && padding_count != 0) {
				*p = padding_char;
				p--;
				padding_count --;
			}

			p ++;
		}

		print_string(putchar_func_p, putchar_arg_p, p, 0);
    }
}


static void
print_int32_decimal(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, int32_t value,
    uint8_t padding_count)
{
    char      buffer[16];
    char      *p = &buffer[sizeof(buffer) - 1];
    uint32_t  abs_value;

    if (value < 0) {
        abs_value = (uint32_t)(-value);
    } else {
        abs_value = (uint32_t)value;
    }

    *p = '\0';
    do {
        p--;
        if (p < buffer) {
            p++;
            *p = 'T'; /* for truncated */
            goto Exit;
        }

        // FDC_ASSERT(p >= buffer, p, buffer);

        *p = (abs_value % 10) + '0';
        abs_value /= 10;
    } while (abs_value > 0);

    if (value < 0) {
        p --;
        *p = '-';
    }

Exit:
    print_string(putchar_func_p, putchar_arg_p, p, padding_count);
}


/**
 * Simplified vprintf function that calls the function pointed to by putchar_func_p
 * one character at a time, with the string resulting of processing the given
 * printf format string. It only supports the following format specifiers:
 * %x, %p, %u, %s.
 *
 * @param putchar_func_p    character output function
 *
 * @param putchar_arg_p     Argument to be passed to putchar_func_p() on every
 *                          invocation
 *
 * @param fmt               format string
 *
 * @param va                variable argument list
 *
 * @return None
 */
void
embedded_vprintf(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, const char *fmt, va_list va)
{
    const char *cursor_p = fmt;
    bool parsing_format_specifier = false;
    bool print_numeric_base_prefix = false;
    bool first_digit_in_format_specifier = false;
    bool prefix_with_zeros = false;
    bool left_justified = false;
    uint8_t padding_count = 0;

    while (*cursor_p != '\0')
    {
        uint8_t c = *cursor_p ++;

        if (parsing_format_specifier)
        {
            switch (c)
            {
            case 'x':
            case 'X':
            case 'p':
                if (print_numeric_base_prefix)
                {
                    print_string(putchar_func_p, putchar_arg_p, "0x", 0);
                    print_numeric_base_prefix = false;
                    if (padding_count >= 2)
                    {
                        padding_count -= 2;
                    }
                }

                if (c == 'p' && sizeof(void *) == sizeof(uint64_t)) {
                    print_uint64_hexadecimal(
                        putchar_func_p, putchar_arg_p, va_arg(va, uint64_t),
                        padding_count);

                } else {
                    print_uint32_hexadecimal(
                        putchar_func_p, putchar_arg_p, va_arg(va, uint32_t),
                        padding_count);
                }
                break;

            case 'u':
                print_uint32_decimal(putchar_func_p, putchar_arg_p, va_arg(va, uint32_t),
                    padding_count, prefix_with_zeros, left_justified);
                break;

            case 'd':
                print_int32_decimal(putchar_func_p, putchar_arg_p, va_arg(va, int32_t),
                    padding_count);
                break;

            case 'c':
                putchar_func_p(putchar_arg_p, va_arg(va, uint32_t));
                break;

            case 's':
                print_string(putchar_func_p, putchar_arg_p, va_arg(va, char *),
                    padding_count);
                break;

            case '#':
                print_numeric_base_prefix = true;
                continue;

            case '-':
                left_justified = true;
                continue;

            default:
                if (c >= '0' && c <= '9')
                {
                    if (first_digit_in_format_specifier) {
                        first_digit_in_format_specifier = false;
                        if (c == '0') {
                            prefix_with_zeros = true;
                            continue;
                        }
                    }

                    padding_count *= 10;
                    padding_count += (c - '0');
                    continue;
                }
                else
                {
                    putchar_func_p(putchar_arg_p, c);
                }
            }

            parsing_format_specifier = false;
        }
        else if (c == '%')
        {
            parsing_format_specifier = true;
            padding_count = 0;
            prefix_with_zeros = false;
            left_justified = false;
            first_digit_in_format_specifier = true;
        }
        else  if (c == '\n')
        {
            putchar_func_p(putchar_arg_p, '\r');
            putchar_func_p(putchar_arg_p, '\n');
        }
        else
        {
            putchar_func_p(putchar_arg_p, c);
        }
    }
}


/**
 * Simplified printf function that calls the function pointed to by putchar_func_p
 * one character at a time, with the string resulting of processing the given
 * printf format string. It only supports the following format specifiers:
 * %x, %p, %u, %s.
 *
 * @param putchar_func_p    character output function
 *
 * @param putchar_arg_p     Argument to be passed to putchar_func_p() on every
 *                          invocation
 *
 * @param fmt               format string
 *
 * @param ...               variable arguments
 *
 * @return None
 */
void
embedded_printf(
    putchar_func_t *putchar_func_p, void *putchar_arg_p, const char *fmt, ...)

{
    va_list va;

    va_start(va, fmt);
    embedded_vprintf(putchar_func_p, putchar_arg_p, fmt, va);
    va_end(va);
}

