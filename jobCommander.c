#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {

    // CHECK IF JOBEXECUTORSERVER IS RUNNING
    if (access("jobExecutorServer.txt", F_OK) == -1) {
        // IT DOESNT EXIST, CREATE IT
        pid_t pid = fork();

        if (pid == 0) {
            // EXEC INTO JOBEXECUTORSERVER
            if (execl("jobExecutorServer", "jobExecutorServer", (char *)NULL) == -1) {
                perror("Failed to start jobExecutorServer");
                exit(EXIT_FAILURE);
            }
        } 
    }

    // CREATE NAMED PIPE
    if (access("pipe", F_OK) == -1) {
        if (mkfifo("pipe", 0666) == -1) {
            perror("Failed to create pipe");
            exit(EXIT_FAILURE);
        }
    }

    // OPEN SAID PIPE
    int pipe_fd = open("pipe", O_WRONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }
    
    // CONSTRUCT INSTRUCTION FROM ARGUMENTS
    char instruction[1024] = "";
    for (int i = 1; i < argc; i++) {
        strcat(instruction, argv[i]);
        strcat(instruction, " ");
    }

   // WRITE INSTRUCTION TO PIPE
    if (write(pipe_fd, instruction, strlen(instruction)) == -1) {
        perror("Failed to write to pipe");
        exit(EXIT_FAILURE);
    }

    // CLOSE PIPE
    close(pipe_fd);
    
    return 0;
}