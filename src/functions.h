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
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>


typedef struct {
    char* id;
    int interval;
    char* key;
    int min;
    int max;
} sensor_data;

// informação sobre as regras para geração de alertas
typedef struct {
    sensor_data* sensors; 
} shm;

FILE *log_file;
sem_t *sem_log;

// Shared memory
int shmid;
shm* shared_memory;

void init();
void terminate();
void write_log(char* msg);
void user_console();
void sensor(char* id, char* interval, char* key, char* min, char* max);
void system_manager(char* config_file);
bool is_digit(char argument[]);