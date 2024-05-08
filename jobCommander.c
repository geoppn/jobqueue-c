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
        } else {
            // Parent process, wait for a while to let the child process create the file
            usleep(500000); // SLEEP FOR HALF A SECOND SO THE FILE GETS CREATED
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
    char instruction[1024] = ""; // CANT BE STATIC. FIX!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
    for (int i = 1; i < argc; i++) {
        strcat(instruction, argv[i]);
        strcat(instruction, " ");
    }

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

   // WRITE INSTRUCTION TO PIPE
    if (write(pipe_fd, instruction, strlen(instruction)) == -1) {
        perror("Failed to write to pipe");
        exit(EXIT_FAILURE);
    }

    // CLOSE PIPE
    close(pipe_fd);

    if (strcmp(instruction, "exit ") == 0 || strcmp(instruction, "poll ") == 0 || strcmp(instruction, "stop ") == 0) { // SPACE NEEDED AFTER EXIT AND POLL
        // OPEN THE READ PIPE
        int pipe_fd2 = open("pipe_exec_cmd", O_RDONLY);
        if (pipe_fd2 == -1) {
            perror("Failed to open pipe_exec_cmd");
            exit(EXIT_FAILURE);
        }

        // READ THE MESSAGE
        char message[1024];
        if (read(pipe_fd2, message, sizeof(message)) > 0) {
            printf("%s", message);
        }

        // CLOSE AND DELETE READ PIPE
        close(pipe_fd2);

        if (unlink("pipe_exec_cmd") == -1) {
            perror("Failed to unlink pipe_exec_cmd");
        }
    }

    
    return 0;
}