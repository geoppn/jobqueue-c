#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "jobQueue.h"

JobDetail *jobs = NULL; // Initialize the head of the job list to NULL
int concurrency = 1;

void printRunningJobs() {
    JobDetail *current = jobs;
    while (current != NULL) {
        if (current->status == RUNNING) {
            printf("Job PID: %d, Command: %s, Status: RUNNING\n", current->pid, current->command);
        }
        current = current->next;
    }
}

void printQueuedJobs() {
    JobDetail *current = jobs;
    while (current != NULL) {
        if (current->status == QUEUED) {
            printf("Job PID: %d, Command: %s, Status: QUEUED\n", current->pid, current->command);
        }
        current = current->next;
    }
}

void handle_sigchld(int sig) {
    // Wait for all dead child processes
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        // Take the next job from the queue and execute it
        Job *job = getNextJob();
        if (job != NULL) {
            pid_t pid = fork();
            if (pid == -1) {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // Child process
                char *args[] = {"/bin/sh", "-c", job->command, NULL};
                execv(args[0], args);
                perror("Failed to exec");
                exit(EXIT_FAILURE);
            }
            // Parent process continues here
            free(job);
        }
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
    int pipe_fd = open("pipe", O_RDONLY | O_NONBLOCK);
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }

    char command[1024];
    
    while (read(pipe_fd, command, sizeof(command)) > 0) {
        char *cmd = strtok(command, " ");
        if (strcmp(cmd, "setConcurrency") == 0) {
            concurrency = atoi(strtok(NULL, " "));
        } else if (strcmp(cmd, "stop") == 0) {
            char id[10];
            strtok(NULL, " ");  // Skip "job"
            strcpy(id, strtok(NULL, " "));  // Get job ID

            // Find job and remove it
            JobDetail *job = findJobById(id);
            if (job != NULL) {
                if (job->status == RUNNING) {
                    kill(job->pid, SIGTERM);
                    printf("job_%s terminated\n", id);
                } else {
                    printf("job_%s removed\n", id);
                }
                removeJob(job);
            }
        } else if (strcmp(cmd, "poll") == 0) {
            char *option = strtok(NULL, " ");
            if (strcmp(option, "running") == 0) {
                printRunningJobs();
            } else if (strcmp(option, "queued") == 0) {
                printQueuedJobs();
            }
        } else if (strcmp(cmd, "exit") == 0) {
            printf("jobExecutorServer terminated.\n");
            break;
        } else {
            addJob(command);
        }
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