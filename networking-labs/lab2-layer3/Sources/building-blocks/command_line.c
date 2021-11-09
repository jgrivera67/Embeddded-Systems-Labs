/**
 * @file command_line.c
 *
 * Command line processor implementation
 *
 * @author: German Rivera
 */
#include "command_line.h"
#include "serial_console.h"
#include "runtime_checks.h"
#include <ctype.h>

/**
 * Max command line size including null terminator
 */
#define COMMAND_LINE_BUFFER_SIZE 81

/**
 * Max number of entries in the command-line argv[]
 */
#define COMMAND_LINE_MAX_ARGV 8

/**
 * Command line state variables
 */
struct command_line {
    char buffer[COMMAND_LINE_BUFFER_SIZE];
    char *buffer_cursor;
    const char *prompt;
    command_parser_t *command_parser;
    int argc;
    const char *argv[COMMAND_LINE_MAX_ARGV];
};

static struct command_line g_command_line;


/**
 * Initializes global command-line object
 *
 * @param prompt            prompt string to be used
 * @param command_parser    callback function to parse commands
 */
void command_line_init(const char *prompt, command_parser_t *command_parser)
{
    g_command_line.buffer_cursor = g_command_line.buffer;
    g_command_line.prompt = prompt;
    g_command_line.command_parser = command_parser;
    console_printf("%s ", prompt);
    console_turn_on_cursor();
}


static void command_line_build_argv(struct command_line *command_line_p)
{
    int argc = 0;
    char *next_arg_p = command_line_p->buffer;

    for (char *s = next_arg_p; *s != '\0'; s ++) {
        if (*s == ' ' || *s == '\t') {
            *s = '\0';
            command_line_p->argv[argc] = next_arg_p;
            argc ++;
            for (s ++; *s == ' ' || *s == '\t'; s ++) {
                ;
            }

            next_arg_p = s;
        }
    }

    if (*next_arg_p != '\0') {
        command_line_p->argv[argc] = next_arg_p;
        argc ++;
    }

    command_line_p->argc = argc;
}


static void command_line_process_char(int c)
{
    D_ASSERT(g_command_line.buffer_cursor >= g_command_line.buffer &&
             g_command_line.buffer_cursor <
                 g_command_line.buffer + COMMAND_LINE_BUFFER_SIZE);

    if (c == '\r') {
        /*
         * Process completed command-line:
         */
        console_puts("\n");
        *g_command_line.buffer_cursor = '\0';
        command_line_build_argv(&g_command_line);
        g_command_line.command_parser(g_command_line.argc, g_command_line.argv);
        g_command_line.buffer_cursor = g_command_line.buffer;
        console_printf("%s ", g_command_line.prompt);
    } else if (c == '\b') {
        if (g_command_line.buffer_cursor > g_command_line.buffer) {
            console_puts("\b \b");
            g_command_line.buffer_cursor--;
        }
    } else if (isprint(c)) {
        if (g_command_line.buffer_cursor <
            g_command_line.buffer + (COMMAND_LINE_BUFFER_SIZE - 1)) {
            console_printf("%c", c);
            *g_command_line.buffer_cursor++ = c;
        }
    }
}

void command_line_process_input(bool wait)
{
    int c;

    if (wait) {
        c = console_getchar();
    } else {
        c = console_getchar_non_blocking();
        if (c == -1) {
            return;
        }
    }

    console_lock();
    command_line_process_char(c);
    console_unlock();
}


