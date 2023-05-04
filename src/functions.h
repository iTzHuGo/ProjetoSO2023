//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

// Defines
#define DEBUG 0
#define BUFFER_SIZE 512
#define LOG_FILE "log.txt"
#define SENSOR_PIPE "SENSOR_PIPE"
#define CONSOLE_PIPE "CONSOLE_PIPE"

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
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>


// Informacao sobre os sensores
typedef struct {
    char id[33];
    int interval;
    char key[33];
    int min;
    int max;
} sensor_data;

// Informacao sobre a shared memory
typedef struct {
    sensor_data* sensors;

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

pthread_t console_reader_thread;
pthread_t sensor_reader_thread;
pthread_t dispatcher_thread;

int fd_console_pipe;
int fd_sensor_pipe;

// Shared memory
int shmid;
shm* shared_memory;

// Funcoes
void init();
void init_shared_mem();
void terminate();
void write_log(char* msg);
void user_console();
void sensor(char* id, int interval, char* key, int min, int max);
void system_manager(char* config_file);
bool is_digit(char argument[]);