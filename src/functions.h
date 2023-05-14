//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

// Defines
#define DEBUG 0
#define BUFFER_SIZE 1024
#define LOG_FILE "log.txt"
#define SENSOR_PIPE "SENSOR_PIPE"
#define CONSOLE_PIPE "CONSOLE_PIPE"
#define QUEUE_NAME "/QUEUE_NAME"

// Includes
#include <sys/types.h>
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
#include <sys/msg.h>
#include <fcntl.h>
#include <stdbool.h>
#include <ctype.h>
#include <unistd.h>
#include <errno.h>


// Informacao sobre os sensores
typedef struct {
    char id[33];
} sensor_data;

typedef struct {
    char name[33];
    int last;
    int min;
    int max;
    double mean;
    int changes;
    int alert;
} key_data;

typedef struct {
    char id[33];
    char key[33];
    int min;
    int max;
    int console_id;
} alert_data;

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
    key_data* keys;
    alert_data* alerts;
    int* workers_list;
} shm;

// Informacao sobre as mensagens recebidas
typedef struct node {
    char* msg;
    int priority;
    struct node* next;
} node;

// Mensague queue
typedef struct {
    long mtype;
    char mtext[100];
} msgq;

// Variaveis globais
FILE* log_file;
sem_t* sem_log;
sem_t* sem_shm;
sem_t* sem_workers;
sem_t* sem_alerts;
config_data config;
int terminate_threads;
node* root;
int counter_sensor;

pthread_t console_reader_thread;
pthread_t sensor_reader_thread;
pthread_t dispatcher_thread;
pthread_t console_reader_msgq_listener_thread;

int fd_console_pipe;
int fd_sensor_pipe;
int(*unnamed_pipes)[2];
int fd_named_pipe_console;

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

// Message queue
//int msgq_id;
msgq mq;

// Funcoes
void init();
void init_shared_mem();
void terminate();
void write_log(char* msg);
void user_console(int console_id);
void sensor(char* id, int interval, char* key, int min, int max);
void system_manager(char* config_file);
bool is_digit(char argument[]);
void sigstp_handler(int signum);