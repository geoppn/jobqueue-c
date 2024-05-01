#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include "jobQueue.h"

Job *queue = NULL;

static int jobCounter = 0; // Counter to keep track of the number of jobs

int getQueueLength() {
    int length = 0;
    Job *current = queue;
    while (current != NULL) {
        length++;
        current = current->next;
    }
    return length;
}

Job* addJob(char *command) {
    Job *job = malloc(sizeof(Job));
    strcpy(job->command, command);
    sprintf(job->id, "job_%d", ++jobCounter); // Set the id to "job_XX"
    job->queuePosition = getQueueLength() + 1; // Set the queue position to the current length of the queue + 1
    job->status = QUEUED; // Set the status to QUEUED
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

Job *getNextJob() {
    if (queue == NULL) {
        return NULL;
    } else {
        Job *job = queue;
        queue = queue->next;
        return job;
    }
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

void removeJob(Job *job) {
    if (queue == NULL) {
        return;
    }

    if (strcmp(queue->id, job->id) == 0) {
        Job *temp = queue;
        queue = queue->next;
        free(temp);
        // Update the queuePosition of all remaining jobs
        Job *current = queue;
        int position = 1;
        while (current != NULL) {
            current->queuePosition = position;
            position++;
            current = current->next;
        }
        return;
    }

    Job *current = queue;
    while (current->next != NULL && strcmp(current->next->id, job->id) != 0) {
        current = current->next;
    }

    if (current->next != NULL) {
        Job *temp = current->next;
        current->next = current->next->next;
        free(temp);
        // Update the queuePosition of all remaining jobs
        current = current->next;
        int position = current->queuePosition;
        while (current != NULL) {
            current->queuePosition = position;
            position++;
            current = current->next;
        }
    }
}