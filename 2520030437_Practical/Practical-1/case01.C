#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

int main() {
    char input[256];
    printf("Enter command: ");
    fflush(stdout);
    if (fgets(input, sizeof(input), stdin) == NULL) {
        return 0;
    }
    input[strcspn(input, "\r\n")] = 0;

    char *args[64];
    int i = 0;
    char *token = strtok(input, " ");
    while (token != NULL && i < 63) {
        args[i++] = token;
        token = strtok(NULL, " ");
    }
    args[i] = NULL;

    if (args[0] == NULL) {
        return 0;
    }

    printf("Parent PID: %d\n", getpid());
    fflush(stdout);

    pid_t pid = fork();

    if (pid < 0) {
        perror("fork failed");
        exit(1);
    } else if (pid == 0) {
        printf("Child PID: %d\n", getpid());
        fflush(stdout);
        execvp(args[0], args);
        perror("exec failed");
        exit(1);
    } else {
        wait(NULL);
        printf("Child finished. Parent PID: %d\n", getpid());
    }

    return 0;
}