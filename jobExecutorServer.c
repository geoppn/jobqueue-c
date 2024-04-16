#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>

void handle_sigchld(int sig) {
    // Wait for all dead child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // TODO: Take the next job from the queue and execute it
    }
}

int main() {
    // CREATE JOBEXECUTORSERVER.TXT AND WRITE PID
    FILE *file = fopen("jobExecutorServer.txt", "w");
    if (file == NULL) {
        perror("Failed to create jobExecutorServer.txt");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d", getpid());
    fclose(file);

    // OPEN THE PIPE
    int pipe_fd = open("/path/to/pipe", O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    signal(SIGCHLD, handle_sigchld);
    

    close(pipe_fd);
    // DELETE JOBEXECUTORSERVER.TXT
    if (remove("jobExecutorServer.txt") == -1) {
        perror("Failed to delete jobExecutorServer.txt");
        exit(EXIT_FAILURE);
    }

    return 0;
}