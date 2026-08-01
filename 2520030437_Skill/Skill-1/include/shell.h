#ifndef SHELL_H
#define SHELL_H

#include <stddef.h>

#define MAX_INPUT_SIZE 512
#define MAX_ARGS 64

void shell_display_prompt();
int shell_read_input(char *buffer, size_t size);
int shell_parse_input(char *input, char *args[]);
int shell_execute_builtin(char *args[], int *running);
void shell_execute_external(char *args[]);

#endif
