//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332
#define DEBUG 1
#define BUFFER_SIZE 512
#define LOG_FILE "log.txt"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include 

typedef struct {
    pid_t system_manager_pid;
} shm;

FILE *log_file;
sem_t *sem_log;

// Shared memory
int shmid;
shm* shared_memory;

