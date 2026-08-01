#define _GNU_SOURCE
#include "../include/shell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

void shell_display_prompt() {
    char *user = getenv("USER");
    if (!user) user = getenv("LOGNAME");
    if (!user) user = "user";

    char hostname[128];
    if (gethostname(hostname, sizeof(hostname)) != 0) {
        strncpy(hostname, "minishell-host", sizeof(hostname));
    }

    char cwd[512];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        strncpy(cwd, "unknown", sizeof(cwd));
    }

    printf("\033[1;32m%s@%s\033[0m:\033[1;34m%s\033[0m$ ", user, hostname, cwd);
    fflush(stdout);
}

int shell_read_input(char *buffer, size_t size) {
    if (fgets(buffer, (int)size, stdin) == NULL) {
        return -1;
    }
    buffer[strcspn(buffer, "\r\n")] = '\0';
    return 0;
}

int shell_parse_input(char *input, char *args[]) {
    int i = 0;
    char *token = strtok(input, " \t");
    while (token != NULL && i < (MAX_ARGS - 1)) {
        args[i++] = token;
        token = strtok(NULL, " \t");
    }
    args[i] = NULL;
    return i;
}

int shell_execute_builtin(char *args[], int *running) {
    if (strcmp(args[0], "exit") == 0) {
        *running = 0;
        return 1;
    }

    if (strcmp(args[0], "clear") == 0) {
        printf("\033[H\033[J");
        fflush(stdout);
        return 1;
    }

    if (strcmp(args[0], "help") == 0) {
        printf("====================================================\n");
        printf("           OSSP Mini-Interactive Shell               \n");
        printf("====================================================\n");
        printf("Built-in Shell Commands:\n");
        printf("  cd [dir]  : Change current directory (Default: HOME)\n");
        printf("  clear     : Clear terminal screen\n");
        printf("  help      : Show shell manual and user commands\n");
        printf("  exit      : Terminate shell session cleanly\n");
        printf("\n");
        printf("System Commands:\n");
        printf("  Runs standard binaries (e.g. ls, pwd, top, whoami)\n");
        printf("====================================================\n");
        return 1;
    }

    if (strcmp(args[0], "cd") == 0) {
        char *target_dir = args[1];
        if (!target_dir) {
            target_dir = getenv("HOME");
            if (!target_dir) {
                target_dir = "/";
            }
        }
        if (chdir(target_dir) != 0) {
            perror("minishell: cd failed");
        }
        return 1;
    }

    return 0;
}

void shell_execute_external(char *args[]) {
    pid_t pid = fork();

    if (pid < 0) {
        perror("minishell: fork failed");
    } else if (pid == 0) {
        printf("[CHILD] PID: %d executing command '%s'\n", getpid(), args[0]);
        fflush(stdout);

        execvp(args[0], args);

        fprintf(stderr, "minishell: command not found or exec failed: %s\n", args[0]);
        exit(127);
    } else {
        printf("[PARENT] PID: %d waiting for child process PID: %d\n", getpid(), pid);
        fflush(stdout);

        int status;
        if (waitpid(pid, &status, 0) < 0) {
            perror("minishell: waitpid failed");
        } else {
            if (WIFEXITED(status)) {
                printf("[PARENT] Child finished with exit status: %d\n", WEXITSTATUS(status));
            } else if (WIFSIGNALED(status)) {
                printf("[PARENT] Child terminated by signal: %d\n", WTERMSIG(status));
            }
        }
        fflush(stdout);
    }
}
