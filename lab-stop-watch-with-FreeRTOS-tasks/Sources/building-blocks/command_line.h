/**
 * @file command_line.c
 *
 * Command line processor interface
 *
 * @author: German Rivera
 */
#ifndef SOURCES_COMMAND_LINE_H_
#define SOURCES_COMMAND_LINE_H_

#include <stdbool.h>

typedef void command_parser_t(int argc, const char *argv[]);

void command_line_init(const char *prompt, command_parser_t *command_parser);

void command_line_process_input(bool wait);

void command_line_print_prompt(void);

#endif /* SOURCES_COMMAND_LINE_H_ */
