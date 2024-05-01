#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

typedef enum { QUEUED, RUNNING, COMPLETED, STOPPED } JobStatus;

typedef struct Job {
    char id[10];  // jobID
    pid_t pid;  // processID
    char command[1024];  // job command
    int queuePosition;  // queuePosition
    JobStatus status;  // job status
    struct Job *next;
} Job;

int getQueueLength();
Job* addJob(char *);
Job *getNextJob();
Job *findJobById(char *);
void removeJob(Job *);

#endif