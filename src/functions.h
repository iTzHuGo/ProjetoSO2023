//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

// Defines
#define DEBUG 0
#define BUFFER_SIZE 512
#define LOG_FILE "log.txt"

// Includes
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>

// Informacao sobre os sensores
typedef struct {
    char id[32];
    int interval;
    char key[32];
    int min;
    int max;
} sensor_data;

// Informacao sobre a shared memory
typedef struct {
    sensor_data* sensors;

    pthread_t console_reader;
    pthread_t sensor_reader;
    pthread_t dispatcher;

    int queue_sz;
    int n_workers;
    int max_keys;
    int max_sensors;
    int max_alerts;
} shm;

// Variaveis globais
FILE* log_file;
sem_t* sem_log;
sem_t* sem_shm;

// Shared memory
int shmid;
shm* shared_memory;

// Funcoes
void init();
void init_shared_mem();
void terminate();
void write_log(char* msg);
void user_console();
void sensor(char* id, char* interval, char* key, char* min, char* max);
void system_manager(char* config_file);
bool is_digit(char argument[]);
void sigint(int signum);