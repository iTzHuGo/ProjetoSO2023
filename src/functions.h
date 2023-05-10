//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

// Defines
#define DEBUG 0
#define BUFFER_SIZE 1024
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
#include <signal.h>
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

typedef struct {
    int queue_sz;
    int n_workers;
    int max_keys;
    int max_sensors;
    int max_alerts;
} config_data;

// Informacao sobre a shared memory
typedef struct {
    sensor_data* sensors;
    int* workers_list;
} shm;

// Informacao sobre as mensagens recebidas
typedef struct node {
    char* msg;
    int priority;
    struct node* next;
} node;

// Variaveis globais
FILE* log_file;
sem_t* sem_log;
sem_t* sem_shm;
sem_t* sems_worker;
config_data config;
int terminate_threads;
node* root;

pthread_t console_reader_thread;
pthread_t sensor_reader_thread;
pthread_t dispatcher_thread;

int fd_console_pipe;
int fd_sensor_pipe;
int(*unnamed_pipes)[2];

// Shared memory
int shmid;
shm* shared_memory;

// Pids
pid_t pid_console;
pid_t pid_sensor;
pid_t pid_system_manager;

// Mutexes
pthread_mutex_t mutex_internal_queue;

// Cond vars
pthread_cond_t cond_internal_queue;

// Funcoes
void init();
void init_shared_mem();
void terminate();
void write_log(char* msg);
void user_console();
void sensor(char* id, int interval, char* key, int min, int max);
void system_manager(char* config_file);
bool is_digit(char argument[]);