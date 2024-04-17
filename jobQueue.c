#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include "jobQueue.h"

Job *queue = NULL;

void addJob(char *command) {
    Job *job = malloc(sizeof(Job));
    strcpy(job->command, command);
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

