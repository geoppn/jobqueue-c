#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

int main(int argc, char *argv[]) {
    // CREATE NAMED PIPE
    if (access("pipe_cmd_exec", F_OK) == -1) {
        if (mkfifo("pipe_cmd_exec", 0666) == -1) {
            perror("Failed to create pipe");
            exit(EXIT_FAILURE);
        }
    }

    // SECOND PIPE
    if (access("pipe_exec_cmd", F_OK) == -1) {
        if (mkfifo("pipe_exec_cmd", 0666) == -1) {
            perror("Failed to create pipe");
            exit(EXIT_FAILURE);
        }
    }



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

    // OPEN SAID PIPES
    int pipe_fd = open("pipe_cmd_exec", O_WRONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }
    
    int pipe_fd2 = open("pipe_exec_cmd", O_RDONLY | O_NONBLOCK);
    if (pipe_fd2 == -1) {
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

    // CLOSE PIPES
    close(pipe_fd);
    close(pipe_fd2);
    
    return 0;
}