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
int concurrency = 1; 
int newConcurrency = -1; // TEMP VALUE FOR SETCONCURRENCY COMMAND [PIAZZA: CONCURRENCY CHANGES WHE NALL ACTIVE JOBS ARE FINISHED]

// Function to check if all jobs are finished
int allJobsFinished() {
    Job *current = jobs;
    while (current != NULL) {
        if (current->status == RUNNING) {
            return 0;
        }
        current = current->next;
    }
    return 1;
}

// Function to update the concurrency when all active jobs are finished
void updateConcurrency() {
    if (newConcurrency != -1 && allJobsFinished()) {
        concurrency = newConcurrency;
        newConcurrency = -1;
    }
}

void handle_sigchld(int sig) { // DOESNT ACCOUNT FOR CONCURRENCY?
    printf("SIGCHLD received\n");
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
            } else { // PARENT
                job->status = RUNNING;    
            }
            

        }
    }

    // CHECK IF NEW CONCURRENCY VALUE IS PENDING
    updateConcurrency();
}

void handle_sigusr1(int sig) {
    char command[1024];
    memset(command, 0, sizeof(command)); // RESET COMMAND BUFFER BETWEEN COMMANDS!
    int pipe_fd = open("pipe_cmd_exec", O_RDONLY); // OPEN THE READ PIPE (ONLY WHEN SIGNAL IS RECEIVED)
    if (pipe_fd == -1) {
        perror("Failed to open pipe");
        exit(EXIT_FAILURE);
    }
    if (read(pipe_fd, command, sizeof(command)) > 0) {
        char *cmd = strtok(command, " ");
        if (strcmp(cmd, "setConcurrency") == 0) { // !!!!!!!!!!! SETCONCURRRENCY !!!!!!!!!!!
            newConcurrency = atoi(strtok(NULL, " "));
            printf("New concurrency value set: %d\n", newConcurrency); // Print the new concurrency value
        } else if (strcmp(cmd, "stop") == 0) { //        !!!!!!!!!!! STOP !!!!!!!!!!!
            char* jobID = strtok(NULL, " "); // Get the jobID from the command
            printf("Stopping job with ID: %s\n", jobID);  // Print the jobID

            // Find the job with the given jobID
            Job* job = NULL;
            Job* prev = NULL;
            for (Job* current = jobs; current != NULL; current = current->next) {
                if (strcmp(current->id, jobID) == 0) {
                    job = current;
                    break;
                }
                prev = current;
            }

            if (job != NULL) {
                if (job->status == RUNNING) {
                    // If the job is running, terminate it
                    kill(job->pid, SIGTERM);
                    printf("job_%s terminated\n", jobID);
                } else if (job->status == QUEUED) {
                    // If the job is queued, remove it from the queue
                    if (prev == NULL) {
                        jobs = job->next;
                    } else {
                        prev->next = job->next;
                    }
                    free(job);
                    printf("%s removed\n", jobID);
                }
            } else {
                printf("No job found with ID: %s\n", jobID);
            }
        } else if (strcmp(cmd, "exit") == 0) { // !!!!!!!!!!! EXIT !!!!!!!!!!!
            // DELETE JOBEXECUTORSERVER.TXT
            if (remove("jobExecutorServer.txt") == -1) {
                perror("Failed to delete jobExecutorServer.txt");
            }

            // UNLINK THE PIPE
            if (unlink("pipe_cmd_exec") == -1) {
                perror("Failed to unlink pipe_cmd_exec");
            }

            // USE OTHER PIPE TO SEND MESSAGE TO JOBCOMMANDER
            int pipe_fd_write = open("pipe_exec_cmd", O_WRONLY);
            if (pipe_fd_write == -1) {
                perror("Failed to open pipe_exec_cmd");
                exit(EXIT_FAILURE);
            }

            char message[] = "jobExecutorServer terminated.\n";
            if (write(pipe_fd_write, message, sizeof(message)) == -1) {
                perror("Failed to write to pipe_exec_cmd");
            }

            close(pipe_fd_write);
         } else if (strcmp(cmd, "issueJob") == 0) { // !!!!!!!!!!! ISSUEJOB !!!!!!!!!!!
            // Get the command from the input
            char *command = strtok(NULL, "\n");
            // Add the job to the queue    
            Job *job = addJob(command);

            // Print the triplet <jobID, job, queuePosition>
            if (job != NULL) {
                // Print the triplet <jobID, job, queuePosition>
                printf("<%s,%s,%d>\n", job->id, job->command, job->queuePosition);
            } // This closing brace was missing
            if (getQueueLength() == 1) { // IF ITS THE FIRST JOB WITHIN THE QUEUE, EXECUTE IT LOCALLY
                pid_t pid = fork();
                if (pid == 0) {
                    // Child process
                    char *args[] = {"/bin/sh", "-c", job->command, NULL};
                    execvp(args[0], args);

                    // ERROR HANDLING
                    printf("An error occurred while executing the command.\n");
                    exit(1);
                } else {
                    // PARENT
                    job->pid = pid;  // STORE PID
                    job->status = RUNNING;

                    //waitpid(pid, NULL, WNOHANG); // ONCE THE JOB IS DONE, WE REMOVE IT FROM THE QUEUE
                    //removeJob(job); // INSANEEEEEEEEEEE ISSUE
                    // HANDLE SIGCHLD IS TRIGGERED HERE
                }
            }
        }else if(strcmp(cmd, "poll") == 0){ // !!!!!!!!!!! POLL !!!!!!!!!!!
            // Get the status from the input
            char *status = strtok(NULL, "\n");

            // USE OTHER PIPE TO SEND MESSAGE TO JOBCOMMANDER
            int pipe_fd_write = open("pipe_exec_cmd", O_WRONLY);
            if (pipe_fd_write == -1) {
                perror("Failed to open pipe_exec_cmd");
                exit(EXIT_FAILURE);
            }

            char *message = malloc(1);
            if (strcmp(status, "running") == 0) {
                // Get running jobs
                Job *current = jobs;
                while (current != NULL) {
                    if (current->status == RUNNING) {
                        int jobDetailsSize = snprintf(NULL, 0, "Job ID: %s, Command: %s, Queue Position: %d\n", current->id, current->command, current->queuePosition) + 1;
                        char* jobDetails = malloc(jobDetailsSize);
                        if (jobDetails == NULL) {
                            perror("Failed to allocate memory for jobDetails");
                            exit(EXIT_FAILURE);
                        }
                        snprintf(jobDetails, jobDetailsSize, "Job ID: %s, Command: %s, Queue Position: %d\n", current->id, current->command, current->queuePosition);
                        message = realloc(message, strlen(message) + strlen(jobDetails) + 1);
                        if (message == NULL) {
                            perror("Failed to allocate memory for message");
                            exit(EXIT_FAILURE);
                        }
                        strcat(message, jobDetails);
                        free(jobDetails);
                    }
                    current = current->next;
                }
            } else if (strcmp(status, "queued") == 0) {
                // Get queued jobs
                Job *current = jobs;
                while (current != NULL) {
                    if (current->status == QUEUED) {
                        int jobDetailsSize = snprintf(NULL, 0, "Job ID: %s, Command: %s, Queue Position: %d\n", current->id, current->command, current->queuePosition) + 1;
                        char* jobDetails = malloc(jobDetailsSize);
                        if (jobDetails == NULL) {
                            perror("Failed to allocate memory for jobDetails");
                            exit(EXIT_FAILURE);
                        }
                        snprintf(jobDetails, jobDetailsSize, "Job ID: %s, Command: %s, Queue Position: %d\n", current->id, current->command, current->queuePosition);
                        message = realloc(message, strlen(message) + strlen(jobDetails) + 1);
                        if (message == NULL) {
                            perror("Failed to allocate memory for message");
                            exit(EXIT_FAILURE);
                        }
                        strcat(message, jobDetails);
                        free(jobDetails);
                    }
                    current = current->next;
                }
            } else {
                sprintf(message, "Invalid status for poll command.\n");
            }

            if (write(pipe_fd_write, message, sizeof(message)) == -1) {
                perror("Failed to write to pipe_exec_cmd");
            }

            close(pipe_fd_write);
        }
    }
    
    close(pipe_fd);
}
int main() {
    // SIGNAL HANDLING
    signal(SIGUSR1, handle_sigusr1);
    signal(SIGCHLD, handle_sigchld);


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

    return 0;
}