#include "../include/shell.h"
#include <stdio.h>

int main() {
    char input[MAX_INPUT_SIZE];
    char *args[MAX_ARGS];
    int running = 1;

    printf("====================================================\n");
    printf(" Welcome to OSSP Mini-Interactive Shell (Practical-2)\n");
    printf(" Type 'help' to see available built-in commands.\n");
    printf("====================================================\n");
    fflush(stdout);

    while (running) {
        shell_display_prompt();
        
        if (shell_read_input(input, sizeof(input)) < 0) {
            printf("\nExiting minishell. Goodbye!\n");
            break;
        }

        int num_args = shell_parse_input(input, args);
        if (num_args == 0) {
            continue;
        }

        if (!shell_execute_builtin(args, &running)) {
            shell_execute_external(args);
        }
    }

    return 0;
}
