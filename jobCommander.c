#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <signal.h>

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
    
    // GET THE PID FROM THE .TXT
    FILE *fp = fopen("jobExecutorServer.txt", "r");
    if (fp == NULL) {
        perror("Failed to open file");
        exit(EXIT_FAILURE);
    }

    pid_t jobExecutorServer_pid;
    fscanf(fp, "%d", &jobExecutorServer_pid);
    fclose(fp);

    // CONSTRUCT INSTRUCTION FROM ARGUMENTS
    char instruction[1024] = "";
    for (int i = 1; i < argc; i++) {
        strcat(instruction, argv[i]);
        strcat(instruction, " ");
    }

    printf("Read PID: %d\n", jobExecutorServer_pid);
    // SEND A SIGNAL TO THE JOBEXECUTORSERVER [ABOUT TO WRITE]
    if (kill(jobExecutorServer_pid, SIGUSR1) == -1) {
        perror("Failed to send signal");
        exit(EXIT_FAILURE);
    }

    // OPEN THE WRITE PIPE
    int pipe_fd = open("pipe_cmd_exec", O_WRONLY);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }
    printf("Pipe opened CMD\n");

   // WRITE INSTRUCTION TO PIPE
    if (write(pipe_fd, instruction, strlen(instruction)) == -1) {
        perror("Failed to write to pipe");
        exit(EXIT_FAILURE);
    }
    printf("Instruction sent to jobExecutorServer\n");

    // CLOSE PIPES
    close(pipe_fd);
    // close(pipe_fd2);
    
    return 0;
}