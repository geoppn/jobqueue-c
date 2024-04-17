#ifndef JOB_QUEUE_H
#define JOB_QUEUE_H

typedef struct JobDetail {
    char command[1024];
    pid_t pid;
    JobStatus status;
    struct JobDetail *next;
} JobDetail;

typedef struct Job {
    char command[1024];
    struct Job *next;
} Job;

typedef enum { QUEUED, RUNNING, COMPLETED, STOPPED } JobStatus;

void addJob(char *command);
Job *getNextJob();

#endif