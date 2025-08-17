# Job Queue & Execution Management System

Georgios Papaioannou - 1115202100222

## Overview

SysPro1 is a C-based mini system designed to manage and execute jobs using a queue, with support for job control, concurrency, and inter-process communication through named pipes and signals. The project is split into several components, each responsible for distinct aspects of job management and server-client interaction.

---

## Contents

- [`jobQueue.h` / `jobQueue.c`]:  
  Implements the core data structures and functions for job queue management.
- [`JobCommander.c`]:  
  Acts as the client/controller, sending commands to the server.
- [`JobExecutorServer.c`]:  
  The main server that handles job execution, concurrency, and communication.
- Bash scripts:  
  Automate multi-job submissions and job stopping.
- Makefile:  
  Facilitates compilation and clean-up tasks.

---

## Build Instructions

To compile and manage the project, use the following commands:

- `make all`  
  Builds all necessary executables for the project.

- `make clean`  
  Removes executables, any generated `.txt` files, and named pipes.

- `make jobExecutorServer/jobCommander`  
  Compiles the specified executable (`jobExecutorServer` or `jobCommander`).

---

## Components & Functionality

### 1. Job Queue

- **Enumerations**
  - `JobStatus`: Represents the state of a job (`QUEUED`, `RUNNING`).
- **Structs**
  - `Job`: Contains job `id`, `pid`, `command`, and `queuePosition`.
- **Functions**
  - `getQueueLength`: Returns the current length of the queue.
  - `addJob`: Adds a new job to the queue, assigning it a unique ID and setting its status.
  - `getNextJob`: Retrieves the next job to execute or returns NULL if the queue is empty.
  - `findJobById`: Searches for a job by its ID.
  - `removeJob`: Removes a job from the queue, either when stopped or completed.
  - `getJobDetailsWithStatus`: Returns details of jobs matching a specific status.

### 2. JobCommander

- Creates two unidirectional named pipes for communication with the server.
- Checks for the server's `.txt` file; launches the server if not found via `execl` and `fork`.
- Prepares commands from user arguments and transmits them to the server.
- Handles special commands (`exit`, `poll`, `stop`) and closes pipes upon completion.

### 3. JobExecutorServer

- Maintains global variables for `concurrency` and the number of running jobs.
- Implements a simple linked list for active job tracking.
- Sets up signal handlers for inter-process communication.
- Utilizes an infinite loop (`pause()`) to remain idle until receiving signals.
- Signal Handling:
  - **SIGUSR1**: Handles commands like `setConcurrency`, `stop`, `exit`, `issueJob`, and `poll`. Interprets commands using buffers and acts accordingly.
  - **SIGCHLD**: Manages child process termination; removes completed jobs and fills available slots from the queue.

#### Command Breakdown

- `setConcurrency <N>`: Sets the maximum number of concurrent jobs.
- `stop <job_id>`: Terminates or removes a specified job.
- `exit`: Shuts down server operations and cleans up.
- `issueJob`: Issues a new job from the queue for execution.
- `poll [queued/running]`: Displays jobs matching the requested state.

---

## Error Handling & Reliability

- Extensive error checks are implemented throughout all modules.
- Bash scripts have been tested against all provided cases.
- Memory leak checks were attempted, but one possible leak remains unresolved.
- The system passes all functional test cases, including multi-job and mass stop scenarios.

---

## Scripts

- **multijob.sh**: Submits multiple jobs and validates against test cases.
- **allJobsStop.sh**: Stops all jobs using both `jobCommander` and bash scripting.

---

## Usage

1. Compile the project with `make all`.
2. Start the server via `jobExecutorServer`.
3. Use `jobCommander` to issue commands, manage jobs, poll status, and adjust concurrency.
4. Refer to the bash scripts for automated scenarios.
