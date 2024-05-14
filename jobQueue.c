#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "jobQueue.h"

Job *queue = NULL;

static int jobCounter = 0; // QUEUE JOB COUNTER GLOBAL VAR

int getQueueLength() { // GET THE LENGTH OF THE QUEUE
    int length = 0;
    Job *current = queue;
    while (current != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

Job* addJob(char *command) { // ADD A JOB TO THE QUEUE
    Job *job = malloc(sizeof(Job));
    strcpy(job->command, command);
    sprintf(job->id, "job_%d", ++jobCounter); // SET THE ID TO THE REQUESTED FORMAT 
    job->queuePosition = getQueueLength() + 1; // SET THE QUEUE POSITION
    job->status = QUEUED; // SET THE STATUS TO QUEUED
    job->next = NULL;

    if (queue == NULL) {
        queue = job;
    } else {
        Job *current = queue;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = job;
    }
    return job;
}

Job *getNextJob() { // GET THE NEXT QUEUED JOB IN THE QUEUE WITHOUT REMOVING IT
    Job *current = queue;
    while (current != NULL) {
        if (current->status == QUEUED) {
            return current; // Return the first job that's queued
        }
        current = current->next;
    }
    return NULL; // Return NULL if no queued jobs are found
}

Job *findJobById(char *id) {
    Job *current = queue;
    while (current != NULL) {
        if (strcmp(current->id, id) == 0) {
            return current;
        }
        current = current->next;
    }
    return NULL;
}

void removeJob(Job *job) { // REMOVE A JOB FROM THE QUEUE
    if (queue == NULL) {
        return;
    }

    if (strcmp(queue->id, job->id) == 0) {
        Job *temp = queue;
        queue = queue->next;
        free(temp);
        // UPDATE THE QUEUE POSITION OF ALL REMAINING JOBS
        Job *current = queue;
        int position = 1;
        while (current != NULL) {
            current->queuePosition = position;
            position++;
            current = current->next;
        }
        return;
    }

    Job *current = queue; // FIND THE JOB IN THE QUEUE
    while (current->next != NULL && strcmp(current->next->id, job->id) != 0) {
        current = current->next;
    }

    if (current->next != NULL) { // REMOVE THE JOB
        Job *temp = current->next;
        current->next = current->next->next;
        free(temp);
        // UPDATE THE QUEUE POSITION OF ALL REMAINING JOBS
        current = current->next;
        if (current != NULL) {
            int position = current->queuePosition;
            while (current != NULL) {
                current->queuePosition = position;
                position++;
                current = current->next;
            }
        }
    }
}

char* getJobDetailsWithStatus(JobStatus status) { // GET THE JOB DETAILS WITH A SPECIFIC STATUS (RUNNING/QUEUED)
    char *message = malloc(1024 * sizeof(char)); // ALLOCATE MEMORY FOR THE MESSAGE
    message[0] = '\0'; // INIT THE MESSAGE FOR STARTERS

    Job *current = queue;
    while (current != NULL) { // ITERATE THROUGH THE QUEUE
        if (current->status == status) { // IF THE STATUS MATCHES:
            char jobDetails[256]; 
            // FORMAT THE JOB DETAILS AND APPEND THEM TO THE MESSAGE
            snprintf(jobDetails, sizeof(jobDetails), "Job ID: %.50s, Command: %.150s, Queue Position: %d\n", current->id, current->command, current->queuePosition); // ADDED PRECISION TO AVOID UNECESSARY WARNING
            strncat(message, jobDetails, 1024 - strlen(message) - 1); // Append job details to the message
        }
        current = current->next;
    }

    return message;
}