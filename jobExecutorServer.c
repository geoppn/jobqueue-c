#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <signal.h>
#include "jobQueue.h"

Job *jobs = NULL; // Initialize the head of the job list to NULL
int concurrency = 1; // WRONG

void printRunningJobs() {
    Job *current = jobs;
    while (current != NULL) {
        if (current->status == RUNNING) {
            printf("Job ID: %s, Command: %s, Status: RUNNING\n", current->id, current->command);
        }
        current = current->next;
    }
}

void printQueuedJobs() {
    Job *current = jobs;
    while (current != NULL) {
        if (current->status == QUEUED) {
            printf("Job ID: %s, Command: %s, Status: QUEUED\n", current->id, current->command);
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

void handle_sigusr1(int sig) {
    char command[1024];
    int pipe_fd = open("pipe_cmd_exec", O_RDONLY); // OPEN THE READ PIPE (ONLY WHEN SIGNAL IS RECEIVED)
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }
    // if (read(pipe_fd, command, sizeof(command)) > 0) {
    //     // COMMAND HANDLING [ALL WRONG BTW]
    //     while (read(pipe_fd, command, sizeof(command)) > 0) {
    //         char *cmd = strtok(command, " ");
    //         if (strcmp(cmd, "setConcurrency") == 0) {
    //             concurrency = atoi(strtok(NULL, " "));
    //             printf("SLAY");
    //         } else if (strcmp(cmd, "stop") == 0) {
    //             char id[10];
    //             strtok(NULL, " ");  // Skip "job"
    //             strcpy(id, strtok(NULL, " "));  // Get job ID

    //             // Find job and remove it
    //             Job *job = findJobById(id);
    //             if (job != NULL) {
    //                 if (job->status == RUNNING) {
    //                     kill(atoi(job->id + 4), SIGTERM); // Skip the "job_" part and convert the remaining part to an int
    //                     printf("job_%s terminated\n", id);
    //                 } else {
    //                     printf("job_%s removed\n", id);
    //                 }
    //                 removeJob(job);
    //             }
    //         } else if (strcmp(cmd, "poll") == 0) {
    //             char *option = strtok(NULL, " ");
    //             if (strcmp(option, "running") == 0) {
    //                 printRunningJobs();
    //             } else if (strcmp(option, "queued") == 0) {
    //                 printQueuedJobs();
    //             }
    //         } else if (strcmp(cmd, "exit") == 0) {
    //             printf("jobExecutorServer terminated.\n");
                
    //             // Assuming pipe1 and pipe2 are the names of your pipes
    //             if (unlink("pipe_exec_cmd") == -1) {
    //                 perror("Failed to delete pipe");
    //             }
    //             if (unlink("pipe_cmd_exec") == -1) {
    //                 perror("Failed to delete pipe");
    //             }

    //             break;
    //         }else { // MAKE THIS A SPECIFIC CASE, FIX IT. COMMAND IS NOT WHOLE. CHANGE COMMAND HANDLING TO TAKE THE WHOLE THING THEN CUT IT FOR THE FIRST IF
    //             addJob(command);
    //         }
    //     }
    // }
    close(pipe_fd);
}
int main() {
    // SIGNAL HANDLING
    signal(SIGUSR1, handle_sigusr1);
    //signal(SIGCHLD, handle_sigchld);


    // CREATE JOBEXECUTORSERVER.TXT AND WRITE PID
    FILE *file = fopen("jobExecutorServer.txt", "w");
    if (file == NULL) {
        perror("Failed to create jobExecutorServer.txt");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d", getpid());
    fclose(file);

    // OPEN THE WRITING PIPE
    // int pipe_fd2 = open("pipe_exec_cmd", O_WRONLY);
    // if (pipe_fd2 == -1) {
    //     perror("Failed to open pipe");
    //     exit(EXIT_FAILURE);
    // }
    // printf("Pipe opened EXEC\n");

    // close(pipe_fd2);
    while (1) { // SIGNAL WAITING LOOP
        pause(); 
    }

    printf("JobExecutorServer is FINISHED\n");

    return 0;
}