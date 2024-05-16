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

int concurrency = 1; 
int running_jobs = 0; // AMOUNT OF CURRENTLY RUNNING JOBS

// LINKED LIST

typedef struct Node {
    char jobID[20];
    struct Node* next;
} Node;

Node* GjobID = NULL; // GLOBAL JOB ID LINKED LIST HEAD FOR PROPER TERMINATION OF JOBS AFTER RUNTIME


void addJobID(char* jobID) { // FUNCTION TO ADD JOB ID TO THE LIST
    Node* newNode = (Node*)malloc(sizeof(Node));
    strncpy(newNode->jobID, jobID, sizeof(newNode->jobID));
    newNode->next = GjobID;
    GjobID = newNode;
}

void removeJobID(char* jobID) { // FUNCTION TO REMOVE JOB ID FROM THE LIST
    Node* temp = GjobID, *prev;

    // REPLACE THE HEAD NODE IF OUR SEARCHED NODE IS THE HEAD
    if (temp != NULL && strcmp(temp->jobID, jobID) == 0) {
        GjobID = temp->next; // CHANGED
        free(temp); // FREE OLD 
        return;
    }

    while (temp != NULL && strcmp(temp->jobID, jobID) != 0) { // SEARCH FOR THE NODE
        prev = temp;
        temp = temp->next;
    }

    if (temp == NULL) // IF THE NODE IS NOT FOUND:
        return;


    prev->next = temp->next; // UNLINK THE NODE

    free(temp); 
}

void handle_sigchld(int sig) {
    printf("SIGCHLD received\n");
    printf("Running jobs: %d\n", running_jobs);

    // WAIT FOR THE CHILD PROCESS(ES) TO TERMINATE
    while (waitpid(-1, NULL, WNOHANG) > 0) {
        running_jobs--; // DECREMENT THE AMOUNT OF RUNNING JOBS (CURRENTLY)
    }


    if (GjobID != NULL) {
        Job* jobToRemove = findJobById(GjobID->jobID);
        if (jobToRemove != NULL) {
            removeJob(jobToRemove);
            removeJobID(GjobID->jobID);
        }
    }

    // REPLACE THE TERMINATED JOBS WITH CORRESPONDING AMOUNT OF NEW ONES
    while (running_jobs < concurrency) {
        Job *job = getNextJob();
        if (job != NULL) {
            job->status = RUNNING; 
            addJobID(job->id); // ADD THE JOBID TO THE ID LIST
            pid_t pid = fork();
            if (pid == -1) {
                perror("Failed to fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                // CHILD:
                char *args[] = {"/bin/sh", "-c", job->command, NULL};
                execv(args[0], args);
                perror("Failed to exec");
                exit(EXIT_FAILURE);
            } else {
                job->pid = pid;
                running_jobs++; // INCREMENT THE AMOUNT OF RUNNING JOBS TO KEEP TRACK
            }
        } else {
            break; // QUEUE IS EMPTY
        }
    }
}

void handle_sigusr1(int sig) {
    char command[1024];
    memset(command, 0, sizeof(command)); // RESET COMMAND BUFFER BETWEEN COMMANDS!
    int pipe_fd = open("pipe_cmd_exec", O_RDONLY); // OPEN THE READ PIPE (ONLY WHEN SIGNAL IS RECEIVED)
    if (pipe_fd == -1) {
        perror("Failed to open pipe pipe_cmd_exec [SERVER]");
        exit(EXIT_FAILURE);
    }
    if (read(pipe_fd, command, sizeof(command)) > 0) {
        char *cmd = strtok(command, " ");
        if (strcmp(cmd, "setConcurrency") == 0) { // !!!!!!!!!!! SETCONCURRRENCY !!!!!!!!!!!
            concurrency = atoi(strtok(NULL, " ")); // GET THE VALUE FROM THE COMMAND AND SET IT ON THE GLOBAL VAR.
            printf("Concurrency set to %d\n", concurrency);
            while (running_jobs < concurrency) {
                Job *job = getNextJob();
                if (job != NULL) {
                    job->status = RUNNING; 
                    addJobID(job->id); // ADD THE JOBID TO THE ID LIST
                    pid_t pid = fork();
                    if (pid == -1) {
                        perror("Failed to fork");
                        exit(EXIT_FAILURE);
                    } else if (pid == 0) {
                        // CHILD:
                        char *args[] = {"/bin/sh", "-c", job->command, NULL};
                        execv(args[0], args);
                        perror("Failed to exec");
                        exit(EXIT_FAILURE);
                    } else {
                        job->pid = pid;
                        running_jobs++; // INCREMENT THE AMOUNT OF RUNNING JOBS TO KEEP TRACK
                    }
                } else {
                    break; // QUEUE IS EMPTY
                }
            }
        } else if (strcmp(cmd, "stop") == 0) { //        !!!!!!!!!!! STOP !!!!!!!!!!!
            char* jobID = strtok(NULL, " "); // GET THE JOBID FROM THE COMMAND
            char message[256]; 
            Job* job = findJobById(jobID);

           if (job != NULL) {
                if (job->status == RUNNING) {
                    // IF THE JOB IS RUNNING, TERMINATE IT
                    kill(job->pid, SIGTERM);
                    sprintf(message, "%s terminated\n", job->id); // SEND MESSAGE TO THE PIPE
                } else if (job->status == QUEUED) {
                    // IF THE JOB IS QUEUED, REMOVE IT FROM THE QUEUE
                    removeJob(job);
                    removeJobID(job->id); 
                    sprintf(message, "%s removed\n", job->id);
                }
            } else {
                sprintf(message, "No job found with ID: %s\n", jobID); // INVALID ID MESSAGE
            }
            // PIPE HANDLING
            int pipe_fd_write = open("pipe_exec_cmd", O_WRONLY);
            if (pipe_fd_write == -1) {
                perror("Failed to open pipe_exec_cmd");
                exit(EXIT_FAILURE);
            }

            if (write(pipe_fd_write, message, sizeof(message)) == -1) {
                perror("Failed to write to pipe_exec_cmd");
            }

            close(pipe_fd_write);
        } else if (strcmp(cmd, "exit") == 0) { // !!!!!!!!!!! EXIT !!!!!!!!!!!
            // DELETE JOBEXECUTORSERVER.TXT
            if (remove("jobExecutorServer.txt") == -1) {
                perror("Failed to delete jobExecutorServer.txt");
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
            usleep(500000);
            close(pipe_fd_write);
         } else if (strcmp(cmd, "issueJob") == 0) { // !!!!!!!!!!! ISSUEJOB !!!!!!!!!!!
            // GET THE COMMAND FROM THE INPUT
            char *command = strtok(NULL, "\n");
            // ADD JOB TO THE QUEUE  
            Job *job = addJob(command);
            if (job != NULL) {
                // PRINT THE JOB DETAILS
                printf("<%s,%s,%d>\n", job->id, job->command, job->queuePosition);
            } 
            if (getQueueLength() <= concurrency) { // IF ITS THE FIRST JOB WITHIN THE QUEUE, EXECUTE IT LOCALLY
                pid_t pid = fork();
                if (pid == 0) {
                    // CHILD:
                    char *args[] = {"/bin/sh", "-c", job->command, NULL};
                    execvp(args[0], args);

                    // ERROR HANDLING
                    printf("An error occurred while executing the command.\n");
                    exit(1);
                } else {
                    // PARENT
                    job->pid = pid;  // STORE PID
                    job->status = RUNNING;
                    addJobID(job->id); // SAVE THE JOBID GLOBALLY
                    running_jobs++;
                    // HANDLE SIGCHLD IS TRIGGERED HERE
                }
            }
        }else if(strcmp(cmd, "poll") == 0){ // !!!!!!!!!!! POLL !!!!!!!!!!!
            // GET THE STATUS FROM THE INPUT
            char *status = strtok(NULL, "\n");
            // USE OTHER PIPE TO SEND MESSAGE TO JOBCOMMANDER
            int pipe_fd_write = open("pipe_exec_cmd", O_WRONLY);
            if (pipe_fd_write == -1) {
                perror("Failed to open pipe_exec_cmd");
                exit(EXIT_FAILURE);
            }

            char message[1024] = ""; // BUFFER FOR MESSAGE
            if (strcmp(status, "running ") == 0) { // SPACE NEEDED
                printf("Polling running jobs\n");
                char *jobDetails = getJobDetailsWithStatus(RUNNING);
                strncat(message, jobDetails, sizeof(message) - strlen(message) - 1);
                message[sizeof(message) - 1] = '\0'; 
                free(jobDetails); // MEMORY 
            } else if (strcmp(status, "queued ") == 0) {
                char *jobDetails = getJobDetailsWithStatus(QUEUED);
                strncat(message, jobDetails, sizeof(message) - strlen(message) - 1);
                message[sizeof(message) - 1] = '\0'; 
                free(jobDetails); 
            } else {
                snprintf(message, sizeof(message), "Invalid status for poll command. use 'running' or 'queued' as arguments.\n"); // INVALID CASE MESSAGE
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

    FILE *file = NULL;

    // CREATE JOBEXECUTORSERVER.TXT AND WRITE PID
    if (access("jobExecutorServer.txt", F_OK) == -1) {
        // File does not exist, try to create it
        file = fopen("jobExecutorServer.txt", "w");
        if (file == NULL) {
            perror("Failed to create jobExecutorServer.txt");
            exit(EXIT_FAILURE);
        }
    } else {
        perror("File jobExecutorServer.txt already exists");
        exit(EXIT_FAILURE);
    }

    fprintf(file, "%d", getpid());
    fclose(file);

    while (1) { // SIGNAL WAITING LOOP
        pause(); 
    }

    return 0;
}