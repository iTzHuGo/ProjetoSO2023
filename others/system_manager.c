/*
    Miguel Filipe de Andrade Sérgio
    2020225643

    João Miguel Carmo Pino

*/

//#include "project.h"

//Includes
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <math.h>
#include <time.h>
#include <stdarg.h>
#include <pthread.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <signal.h>

//Tamanho maximo de qualquer buffer de texxto no programa
#define MAX_BUFFER 256

//Semafro para acesso a shared memory
#define SEM_SHM "/SHM_Semaphore"
sem_t *sem_SHHM;
int shmid;

//Valores do ficheiro de configuracoes
int MAX_KEYS;
int MAX_SENSORS;
int MAX_ALERTS;
int QUEUE_SZ;
int N_WORKERS;

//Mutex para controlar acessos de escrita no ficheiro/consola
pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;

//Valores indicadores da fila de menssagens
int message_queue_id;
key_t key = 1238;

//Unamed pipes
int (*pipes)[2];
sem_t *semaphore_worker;

pid_t alerts_watcher_pid;
pid_t *worker_pids;

char *SENSOR_PIPE_name = "SENSOR_PIPE";
char *CONSOLE_PIPE_name = "CONSOLE_PIPE";
int SENSOR_PIPE;
int CONSOLE_PIPE;

pthread_t thread_dispatcher;
pthread_t thread_console_reader;
pthread_t thread_sensor_reader;

int dispatcher_flag;
int console_reader_flag;
int sensor_reader_flag;

//Estrutura na shared memory com informao sobre os sensores
typedef struct{
    char sensor_id[MAX_BUFFER];
} Data_sensor;

//Estrutura na shared memory com informacao sobre os sensores, sobre as chaves e estatisticas
typedef struct{
    char sensor_id[MAX_BUFFER];
    char sensor_key[MAX_BUFFER];
    int recive_value;
    int min_recive;
    int max_recive;
    int total_recive;
    double mean_recive;
} Data_keys;

//Estrutura na shared memory com informacao sobre os alertas
typedef struct{
    char alert_id[MAX_BUFFER];
    char sensor_key[MAX_BUFFER];
    int min_value;
    int max_value;
} Data_alerts;

//Estrutura geral da shared memory
typedef struct{
    Data_sensor *data_sensor;
    Data_keys *data_keys;
    Data_alerts *data_alerts;
} Data;

//Estrutura acociada a fila de menssagens
typedef struct {
    char text[MAX_BUFFER];
} Message;

typedef struct node{
    char *message;
    struct node *anterior;
    struct node *proximo;
} Node;

typedef struct {
    Node *frente;
    Node *tras;
    int size;
} Fila;

Fila *fila;

//Funcao para escrever no ficheiro log e na consola
void write_log(char *message) {
    //Obtem informacoes da hora
    time_t rawtime;
    struct tm *timeinfo;
    char buffer[MAX_BUFFER];

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer, MAX_BUFFER, "%H:%M:%S", timeinfo);

    //Fecha o mutex acociado para nao haver conflitos na escrita
    pthread_mutex_lock(&log_mutex);
    //Abre o ficheiro log e faz a escrita tanto na conssola como no ficheiro
    FILE *fp = fopen("log.txt", "a");

    printf("%s  %s\n", buffer, message);
    fprintf(fp, "%s %s\n", buffer, message);

    //Fecha o ficheiro e abre o mutex
    fclose(fp);
    pthread_mutex_unlock(&log_mutex);
}

//Funcao para criar a shared memory
Data *create_SHM(){
    Data *shm;
    //Faco unlink primeiro por causa de conflitos com o "caminho" do semafro
    sem_unlink(SEM_SHM);
    //Criacao do semafro para aceco a shared memory
    sem_SHHM = sem_open(SEM_SHM,  O_CREAT | O_EXCL, 0666, 1);
    if (sem_SHHM == SEM_FAILED) {
        write_log("Error em sem_open");
    }

    write_log("Semafro da memoria partilhada criado.");

    //Shared memory a ser acecada semafro fechado
    sem_wait(sem_SHHM);
    
    //Cria a shared memory
    if ((shmid = shmget(1235, sizeof(Data), IPC_CREAT | 0666)) < 0){
        write_log("Error in shmget.");
    }
    //Acede a shared memory
    if ((shm = (Data *) shmat(shmid, NULL, 0)) == (Data *)-1){
        write_log("Error in shmat.");
    }

    //Alloca memoria de modo a tornar o data_key, o data_sensor e o data_alerts vetores com o devido tamanho
    shm->data_alerts = (Data_alerts *) malloc(MAX_ALERTS * sizeof(Data_alerts));
    shm->data_keys = (Data_keys *) malloc(MAX_KEYS * sizeof(Data_keys));
    shm->data_sensor = (Data_sensor *) malloc(MAX_SENSORS * sizeof(Data_sensor));

    //Inicia os valores da shared memory
    for(int i = 0; i < MAX_ALERTS; i++){
        strcpy(shm->data_alerts[i].alert_id, "");
        strcpy(shm->data_alerts[i].sensor_key, "");
        shm->data_alerts[i].min_value = 0;
        shm->data_alerts[i].max_value = 0;
    }

    for(int i = 0; i < MAX_KEYS; i++){
        strcpy(shm->data_keys[i].sensor_id, "");
        strcpy(shm->data_keys[i].sensor_key, "");
        shm->data_keys[i].recive_value = 0;
        shm->data_keys[i].min_recive = 0;
        shm->data_keys[i].max_recive = 0;
        shm->data_keys[i].total_recive = 0;
        shm->data_keys[i].mean_recive = 0.0;
    }

    for(int i = 0; i < MAX_ALERTS; i++){
        strcpy(shm->data_sensor[i].sensor_id, "");
    }

    //Liberta o aceco a shared memory
    sem_post(sem_SHHM);

    write_log("Memoria partilhada criada.");
    return shm;
}

//Abre uma shared memory ja criada
Data *attach_SHM() {
    Data *shm;
    if ((shm = shmat(shmid, NULL, 0)) == (Data*)-1) {
        write_log("Erro ao associar a shared memory.");
    }
    return shm;
}

//Corta o aceco a uma shared memory ja criada
void detach_SHM(Data *shm){
    if (shmdt(shm) == -1){
        write_log("Erro in shmdt.");
    }
    write_log("SHM desacociada do processo.");
}

//Elemina a shared memory
void delete_SHM(){
    if (shmctl(shmid, IPC_RMID, NULL) == -1){
        write_log("Erro in shmctl.");
    }
    write_log("SHM eliminada.");
}

//Le o ficheiro de configuracoes e guarda os respetivos valores
int read_config(char *file){
    FILE *fp;
    char buffer[MAX_BUFFER];

    fp = fopen(file, "r");

    if (fp == NULL) {
        write_log("Erro ao abrir o arquivo.");
        return 1;
    }

    fgets(buffer, MAX_BUFFER, fp);
    QUEUE_SZ = atoi(buffer);
    if(QUEUE_SZ < 1){
        write_log("Erro tamanho da INTERNAL_QUEUE <1.");
        return 1;
    }

    fgets(buffer, MAX_BUFFER, fp);
    N_WORKERS = atoi(buffer);
    if(N_WORKERS < 1){
        write_log("Erro N_WORKERS < 1.");
        return 1;
    }

    fgets(buffer, MAX_BUFFER, fp);
    MAX_KEYS = atoi(buffer);
    if(MAX_KEYS < 1){
        write_log("Erro MAX_KEYS < 1.");
        return 1;
    }

    fgets(buffer, MAX_BUFFER, fp);
    MAX_SENSORS = atoi(buffer);
    if(MAX_SENSORS < 1){
        write_log("Erro MAX_SENSORS < 1.");
        return 1;
    }

    fgets(buffer, MAX_BUFFER, fp);
    MAX_ALERTS = atoi(buffer);
    if(MAX_ALERTS < 1){
        write_log("Erro MAX_ALERTS < 1.");
        return 1;
    }

    fclose(fp);

    write_log("Ficheiro lido com sucesso.");

    return 0;
}

//Cria fila de menssagens
void create_MQ(){
    message_queue_id = msgget(key, IPC_CREAT | 0660);
    if (message_queue_id == -1) {
        write_log("Erro ao criar a fila de mensagens");
    }
    write_log("Fila de menssagens criada.");
}

//Elimina fila de menssagens
void eleminate_MQ(){
    msgctl(message_queue_id, IPC_RMID, NULL);
    write_log("Elemina Message Queu.");
}

//Envia por a fila de menssagens ja criada
void send_MQ(char *text) {
    Message message;
    strcpy(message.text, text);
    if(msgsnd(message_queue_id, &message, sizeof(message.text), 0) == -1){
        write_log("Erro a enviar menssagem.");
    }
    write_log("Menssagem enviada.");
}

//Recebe pela fila de menssagens ja criada
char *receive_MQ() {
    Message message;
    if(msgrcv(message_queue_id, &message, sizeof(message.text), 0, 0) == -1){
        write_log("Erro a receber menssagem");
    }
    char *received_text = malloc(strlen(message.text)+1);
    strcpy(received_text, message.text);
    return received_text;
}

//Processo
void alerts_watcher(){

    char output[MAX_BUFFER];
    char *sender;
    
    write_log("Processo alerts_watcher a iniciar");
    //Acede a shared memory
    Data *shm = attach_SHM();

    while (1){
        //Precorre todos os aletrtas, verifica os valores se estes nao estiverem na norma envia pela fila de menssagens um alerta
        for (int i = 0; i < MAX_ALERTS; i++){
            char *sensor_key = shm->data_alerts[i].sensor_key;
            for (int j = 0; j < MAX_KEYS; j++){
                if (strcmp(sensor_key, shm->data_keys[j].sensor_key) == 0){
                    if (shm->data_keys[j].recive_value > shm->data_alerts[i].max_value){
                        strcpy(output, "");
                        sprintf(output, "Alerta: %s, Sensor: %s, Chave: %s, Valor recebido(%d) excede o valor maximo do alerta(%d)", shm->data_alerts[i].alert_id, shm->data_keys[j].sensor_id, shm->data_keys[j].sensor_key, shm->data_keys[j].recive_value, shm->data_alerts[i].max_value);
                        sender = output;
                        send_MQ(output);
                        strcpy(output, "");
                    }
                    if (shm->data_keys[j].recive_value < shm->data_alerts[i].min_value){
                        strcpy(output, "");
                        sprintf(output, "Alerta: %s, Sensor: %s, Chave: %s, Valor recebido(%d) e infrior ao valor minimo do alerta(%d)", shm->data_alerts[i].alert_id, shm->data_keys[j].sensor_id, shm->data_keys[j].sensor_key, shm->data_keys[j].recive_value, shm->data_alerts[i].min_value);
                        sender = output;
                        send_MQ(output);
                        strcpy(output, "");
                    }
                    if (shm->data_keys[j].mean_recive > shm->data_alerts[i].max_value){
                        strcpy(output, "");
                        sprintf(output, "Alerta: %s, Sensor: %s, Chave: %s, Valor medio(%f) excede o valor maximo do alerta(%d)", shm->data_alerts[i].alert_id, shm->data_keys[j].sensor_id, shm->data_keys[j].sensor_key, shm->data_keys[j].mean_recive, shm->data_alerts[i].max_value);
                        sender = output;
                        send_MQ(output);
                        strcpy(output, "");
                    }
                    if (shm->data_keys[j].mean_recive < shm->data_alerts[i].max_value){
                        strcpy(output, "");
                        sprintf(output, "Alerta: %s, Sensor: %s, Chave: %s, Valor recebido(%f) e infrior ao valor minimo do alerta(%d)", shm->data_alerts[i].alert_id, shm->data_keys[j].sensor_id, shm->data_keys[j].sensor_key, shm->data_keys[j].mean_recive, shm->data_alerts[i].min_value);
                        sender = output;
                        send_MQ(output);
                        strcpy(output, "");
                    }
                }
            }
        }
    }

    write_log("Processo alerts_watcher a terminar.");

    detach_SHM(shm);
}

//Processo
void worker(int id){
    char output[MAX_BUFFER];
    char output_aux[MAX_BUFFER];
    char buffer[MAX_BUFFER];
    char *delim = "#";
    char *sender;
    
    strcpy(output, "");
    sprintf(output, "Processo worker com id %d a iniciar", id);
    sender = output;
    write_log(sender);
    strcpy(output, "");

    //Inicia semafro para dar informacao se o worker esta ou nao ocupado
    if (sem_init(&semaphore_worker[id], 0, 1) == -1) {
        write_log("Erro ao criar o semáforo");
    }
    //Acede a shared memory
    Data *shm = attach_SHM();

    while(1){
        //Le do unamed pipe a ele acociado
        if(read(pipes[id][0], buffer, sizeof(buffer)) == -1){
            write_log("Erro ao ler do unamed pipe");
        }
        //Poe o worker em trabalho
        sem_wait(&semaphore_worker[id]);
        strcpy(output, "");
        sprintf(output, "Processo worker com id %d ocupado", id);
        sender = output;
        write_log(sender);
        strcpy(output, "");
        //Verifica qual o comando a realizar e realizao
        sender = buffer;
        if(strcmp(buffer, "stats") == 0){
            for(int i = 0; i < MAX_SENSORS; i++){
                if(shm->data_sensor[i].sensor_id != ""){
                    strcpy(output, "");
                    sprintf(output, "Sensor %s\n", shm->data_sensor[i].sensor_id);
                    char *sensor_id = shm->data_sensor[i].sensor_id;
                    for (int j = 0; j < MAX_KEYS; j++){
                        if (strcmp(sensor_id, shm->data_keys[j].sensor_id) == 0){
                            strcpy(output_aux, "");
                            sprintf(output_aux, "Chave %s: valores medios recebidos = %f, total de valores recebidos = %d, valor maximo recebido = %d, valor minimo recebido = %d\n", shm->data_keys[j].sensor_key, shm->data_keys->mean_recive, shm->data_keys->total_recive, shm->data_keys->max_recive, shm->data_keys->min_recive);
                            strcat(output, output_aux);
                            strcpy(output_aux, "");
                        }
                    }
                }
                sender = output;
                send_MQ(sender);
                write_log(sender);
                strcpy(output, "");
            }
        }
        else if (strcmp(buffer, "reset") == 0){
            for (int i = 0; i < MAX_KEYS; i++){
                shm->data_keys[i].mean_recive = 0;
                shm->data_keys[i].total_recive = 0;
                shm->data_keys[i].max_recive = 0;
                shm->data_keys[i].min_recive = 0;
            }
            send_MQ("OK");
            write_log("Estatisticas reiniciadas.");
        }
        else if (strcmp(buffer, "sensores") == 0){
            for(int i = 0; i < MAX_SENSORS; i++){
                if(shm->data_sensor[i].sensor_id != ""){
                    sender = output;
                    send_MQ(sender);
                    sender = shm->data_sensor[i].sensor_id;
                    write_log(sender);
                }
            }
        }
        else if (strcmp(strtok(buffer, " "), "add_alert") == 0){
            char *palavras[4];
            int i = 0;
            char *token = strtok(buffer, " ");
            while (buffer != NULL) {
                palavras[i] = token;
                i++;
                token = strtok(NULL, " ");
            }
            for(int i = 0; i < MAX_ALERTS; i++){
                if(strcmp(shm->data_alerts[i].alert_id, "") == 0){
                    strcpy(shm->data_alerts[i].alert_id, palavras[1]);
                    strcpy(shm->data_alerts[i].sensor_key, palavras[2]);
                    shm->data_alerts[i].min_value = atoi(palavras[3]);
                    shm->data_alerts[i].min_value = atoi(palavras[4]);
                }
            }
            send_MQ("OK");
            write_log("Novo alerta criado");

        }
        else if (strcmp(strtok(buffer, " "), "remove_alert") == 0){
            char *palavras2[1];
            int i2 = 0;
            char *token2 = strtok(buffer, " ");
            while (buffer != NULL) {
                palavras2[i2] = token2;
                i2++;
                token2 = strtok(NULL, " ");
            }
            for(int i = 0; i < MAX_ALERTS; i++){
                if(strcmp(shm->data_alerts[i].alert_id, palavras2[1]) == 0){
                    strcpy(shm->data_alerts[i].alert_id, "");
                    strcpy(shm->data_alerts[i].sensor_key, "");
                    shm->data_alerts[i].min_value = 0;
                    shm->data_alerts[i].min_value = 0;
                }
            }
            send_MQ("OK");
            write_log("Alerta removido.");
        }
        else if (strcmp(buffer, "list_alerts") == 0){
            for(int i = 0; i < MAX_ALERTS; i++){
                if(strcmp(shm->data_alerts[i].alert_id, "") != 0){
                    strcpy(output, "");
                    sprintf(output, "Alerta %s, Chave %s, valor minimo = %d, valor maximo = %d", shm->data_alerts[i].alert_id, shm->data_alerts[i].sensor_key, shm->data_alerts[i].min_value, shm->data_alerts[i].max_value);
                    sender = output;
                    send_MQ(sender);
                    write_log(sender);
                    strcpy(output, "");
                }
            }
        }
        else if (strchr(buffer, *delim) != NULL){
            char *palavras3[2];
            int i3 = 0;
            char *token3 = strtok(buffer, "#");
            int auxiliar = 0;

            while (buffer != NULL) {
                palavras3[i3] = token3;
                i3++;
                token3 = strtok(NULL, "#");
            }
            for(int i = 0; i < MAX_SENSORS; i++){
                if(strcmp(shm->data_sensor[i].sensor_id, palavras3[0]) == 0){
                    auxiliar++;
                }
            }
            if (auxiliar == 0){
                for(int i = 0; i < MAX_SENSORS; i++){
                    if(strcmp(shm->data_sensor[i].sensor_id, "") == 0){
                        strcpy(shm->data_sensor[i].sensor_id, palavras3[0]);
                    }
                }
            }
            int auxiliar2 = 0;
            int auxiliar3 = 0;
            for(int i = 0; i < MAX_KEYS; i++){
                if(strcmp(shm->data_keys[i].sensor_id, palavras3[0]) == 0){
                    if(strcmp(shm->data_keys[i].sensor_key, palavras3[1]) == 0){
                        shm->data_keys[i].recive_value = atoi(palavras3[2]);
                        shm->data_keys[i].mean_recive = ((shm->data_keys[i].mean_recive * shm->data_keys[i].total_recive) + shm->data_keys[i].recive_value) / (shm->data_keys[i].total_recive + 1);
                        shm->data_keys[i].total_recive = shm->data_keys[i].total_recive + 1;
                        if(shm->data_keys[i].min_recive > atoi(palavras3[2])){
                            shm->data_keys[i].min_recive = atoi(palavras3[2]);
                        }
                        if(shm->data_keys[i].min_recive < atoi(palavras3[2])){
                            shm->data_keys[i].min_recive = atoi(palavras3[2]);
                        }
                        auxiliar3++;
                    }
                    auxiliar2++;
                }
            }
            if(auxiliar2 == 0){
                for(int i = 0; i < MAX_KEYS; i++){
                    if(strcmp(shm->data_keys[i].sensor_id, "") == 0){
                        strcpy(shm->data_keys[i].sensor_id, palavras3[0]);
                        strcpy(shm->data_keys[i].sensor_key, palavras3[1]);
                        shm->data_keys[i].recive_value = atoi(palavras3[2]);
                        shm->data_keys[i].mean_recive = atoi(palavras3[2]);
                        shm->data_keys[i].total_recive = 1;
                        shm->data_keys[i].min_recive = atoi(palavras3[2]);
                        shm->data_keys[i].min_recive = atoi(palavras3[2]);
                    }
                }
            }
            else{
                if(auxiliar3 == 0){
                    for(int i = 0; i < MAX_KEYS; i++){
                        if(strcmp(shm->data_keys[i].sensor_id, "") == 0){
                            shm->data_keys[i].recive_value = atoi(palavras3[2]);
                            shm->data_keys[i].mean_recive = atoi(palavras3[2]);
                            shm->data_keys[i].total_recive = 1;
                            shm->data_keys[i].min_recive = atoi(palavras3[2]);
                            shm->data_keys[i].min_recive = atoi(palavras3[2]);
                        }
                    }
                }
            }
            write_log("Adicionada novo registo de um sensor");
        }
        else{
            write_log("Tarefa nao reconhecida -- Worker.");
        }
        //Liberta o worker
        sem_post(&semaphore_worker[id]);
        strcpy(output, "");
        sprintf(output, "Processo worker com id %d desocupado", id);
        sender = output;
        write_log(sender);
        strcpy(output, "");
    }
    sem_destroy(&semaphore_worker[id]);

    detach_SHM(shm);

    strcpy(output, "");
    sprintf(output, "Processo worker com id %d a terminar", id);
    sender = output;
    write_log(sender);
    strcpy(output, "");

}

void create_IQ(){
    fila = (Fila*) malloc(sizeof(Fila));

    if(fila == NULL) {
        write_log("Erro: não foi possível alocar memória para a fila.\n");
    }

    fila->frente = NULL;
    fila->tras = NULL;

    write_log("Fila innterna de menssagens criada");
}

void insere_frente_IQ(char *message){
    if(fila->size <= QUEUE_SZ){
        Node *node = malloc(sizeof(Node));
        node->message = message;
        node->anterior = NULL;
        node->proximo = fila->frente;
        if(fila->frente == NULL){
            fila->tras = node;
        }
        else{
            fila->frente->anterior = node;
        }
        fila->frente = node;
        fila->size = fila->size++;
    }
    else{
        while(fila->size > QUEUE_SZ){
            continue;
        }
        Node *node = malloc(sizeof(Node));
        node->message = message;
        node->anterior = NULL;
        node->proximo = fila->frente;
        if(fila->frente == NULL){
            fila->tras = node;
        }
        else{
            fila->frente->anterior = node;
        }
        fila->frente = node;
        fila->size = fila->size++;
    }
}

void insere_tras_IQ(char *message){
    if(fila->size <= QUEUE_SZ){
        Node *node = malloc(sizeof(Node));
        node->message = message;
        node->anterior = fila->tras;
        node->proximo = NULL;
        if (fila->frente == NULL){
            fila->frente = node;
        }
        else{
            fila->tras->proximo = node;
        }
        fila->tras = node;
        fila->size = fila->size++;
    }
    else{
        write_log("Impossivel insserir na fila, fila cheia");
    }
}

void percorrerFila() {
    Node *noAtual = fila->frente;
    int i = 1;
    while (noAtual != NULL) {
        printf("Mensagem %d: %s\n", i, noAtual->message);
        noAtual = noAtual->proximo;
        i++;
    }
}

void sigint_handler(int sig) {
    write_log("Terminando processo pai e filhos");

    // Terminar processo alerts_watcher
    if (alerts_watcher_pid > 0) {
        kill(alerts_watcher_pid, SIGINT);
        waitpid(alerts_watcher_pid, NULL, 0);
    }

    // Terminar processos workers
    for(int i = 0; i < N_WORKERS; i++){
        if (worker_pids[i] > 0) {
            kill(worker_pids[i], SIGINT);
            waitpid(worker_pids[i], NULL, 0);
        }
    }

    // Terminar thread dispatcher
    dispatcher_flag = 0;
    pthread_join(thread_dispatcher, NULL);

    // Terminar thread console_reader
    console_reader_flag = 0;
    pthread_cancel(thread_console_reader);
    pthread_join(thread_console_reader, NULL);

    // Terminar thread sensor_reader
    sensor_reader_flag = 0;
    pthread_cancel(thread_sensor_reader);
    pthread_join(thread_sensor_reader, NULL);

    // Fechar named pipes
    close(SENSOR_PIPE);
    close(CONSOLE_PIPE);

    percorrerFila();

    // Liberar memória alocada
    free(fila);
    eleminate_MQ();
    delete_SHM();

    exit(0);
}

void sensor_reader(void *args){
    char mensagem[MAX_BUFFER];
    char *sender;

    write_log("Sensor reader start.");

    while(sensor_reader_flag == 1){
        if(read(CONSOLE_PIPE, mensagem, sizeof(mensagem)) == -1){
            write_log("Erro a ler o CONSOLE_PIPE");
        }
        sender = mensagem;
        insere_tras_IQ(sender);
    }

    write_log("Sensor reader a terminar");
}

void console_reader(void *args){

    char mensagem[MAX_BUFFER];
    char *sender;
    
    write_log("Console reader start");

    while(console_reader_flag == 1){
        if(read(CONSOLE_PIPE, mensagem, sizeof(mensagem)) == -1){
            write_log("Erro a ler o CONSOLE_PIPE");
        }
        sender = mensagem;
        insere_frente_IQ(sender);
    }

    write_log("Console reader a terminar");
}

void dispatcher(void *args){
    int sval;

    write_log("Dispatcher a comecar");

    while(dispatcher_flag == 1){
        for(int i = 0; i < N_WORKERS; i++){
            if (fila->frente != NULL) {
                Node *node = fila->frente;
                fila->frente = node->anterior;
                fila->size = fila->size--;
                if(sem_getvalue(&semaphore_worker[i], sval) == -1){
                    write_log("Erro a ler o estado do worker");
                }
                if (sval == 1){
                    if (write(pipes[i][1], node->message, strlen(node->message) + 1) == -1){
                        write_log("Erro a escrever em unamed pipe");
                    }
                }
            }
        }
    }

    write_log("Dispatcher a terminar");

}

int main(int argc, char *argv[]){

    signal(SIGINT, sigint_handler);

    //Limpa o ficheiro Log
    FILE *fp = fopen("log.txt", "w");
    fclose(fp);

    write_log("Inicio");
    //Recebe e le o ficheiro config
    if (argc < 2) {
        write_log("Deve receber o nome do ficheiro config.");
        return 1;
    }

    read_config(argv[1]);
    //Cria a shared memory
    Data *shm = create_SHM();
    //Cria a fila de menssagem
    create_MQ();

    create_IQ();

    if (access(SENSOR_PIPE_name, F_OK) == 0) {
        if ((SENSOR_PIPE = open(SENSOR_PIPE_name, O_RDWR)) == -1){
            write_log("Erro a criar o SENSOR_PIPE.");
        }
    }
    else{
        if(mkfifo(SENSOR_PIPE_name, 0666) == -1){
            write_log("Erro a criar o named pipe");
        }
        if ((SENSOR_PIPE = open(SENSOR_PIPE_name, O_RDWR)) == -1){
            write_log("Erro a criar o SENSOR_PIPE.");
        }
    }

    if (access(CONSOLE_PIPE_name, F_OK) == 0) {
        if ((CONSOLE_PIPE = open(CONSOLE_PIPE_name, O_RDWR)) == -1){
            write_log("Erro a criar o SENSOR_PIPE.");
        }
    }
    else{
        if(mkfifo(CONSOLE_PIPE_name, 0666) == -1){
            write_log("Erro a criar o named pipe");
        }
        if ((CONSOLE_PIPE = open(CONSOLE_PIPE_name, O_RDWR)) == -1){
            write_log("Erro a criar o SENSOR_PIPE.");
        }
    }

    worker_pids = (pid_t *) malloc(N_WORKERS * sizeof(pid_t));
    pipes = malloc(N_WORKERS * sizeof(*pipes));
    semaphore_worker = (sem_t *) malloc(N_WORKERS * sizeof(sem_t));

    //Inicia processo alerts_watcher

    alerts_watcher_pid = fork();

    if(alerts_watcher_pid < 0){
        write_log("Erro a criar processo worker.");
        exit(1);
    }
    if (alerts_watcher_pid == 0) {
        alerts_watcher();
        exit(0);
    }
    else if(alerts_watcher_pid < 0){
        write_log("Erro a criar o processo alerts_watcher.");
        exit(0);
    }

    //Inicia N_WORKERS processo workers

    for(int i = 0; i < N_WORKERS; i++){
        if(pipe(pipes[i]) < 0) {
            write_log("Erro ao criar o pipe.");
            return 1;
        }

        worker_pids[i] = fork();

        if(worker_pids[i] < 0){
            write_log("Erro a criar processo worker.");
            exit(1);
        }
        if (worker_pids[i] == 0) {
            worker(i);
            exit(0);
        }
        else if (worker_pids[i] < 0) {
            write_log("Erro a criar o processo worker.");
            exit(0);
        }
    }

    dispatcher_flag = 1;
    console_reader_flag = 1;
    sensor_reader_flag = 1;

    pthread_create(&thread_console_reader, NULL, console_reader, NULL);
    pthread_create(&thread_sensor_reader, NULL, sensor_reader, NULL);
    pthread_create(&thread_dispatcher, NULL, dispatcher, NULL);

    while(1){
        continue;
    }

    /*

    shm = attach_SHM();

    strcpy(shm->data_sensor[0].sensor_id, "Sensor 1");
    printf("%s\n", shm->data_sensor[0].sensor_id);

    send_MQ("Ola");

    printf("%s\n", receive_MQ());

    eleminate_MQ();

    detach_SHM(shm);

    delete_SHM();

    sleep(2);

    char *str;
    int len;

    str = "SENS1#HOUSETEM#20";
    len = sizeof(str);

    if (write(pipes[0][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "SENS1#HOUSETEM#10";
    len = sizeof(str);

    if (write(pipes[1][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "SENS2#TEMP#5";
    len = sizeof(str);

    if (write(pipes[2][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "add_alert Alert1 HOUSETEM 0 15";
    len = sizeof(str);

    if (write(pipes[3][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "add_alert Alert2 TEMP 0 10";
    len = sizeof(str);

    if (write(pipes[4][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "add_alert Alert1 HOUSETEM 0 5";
    len = sizeof(str);

    if (write(pipes[5][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "stats";
    len = sizeof(str);

    if (write(pipes[0][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "sensors";
    len = sizeof(str);

    if (write(pipes[1][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "list_alerts";
    len = sizeof(str);

    if (write(pipes[2][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "remove_alert Alert1";
    len = sizeof(str);

    if (write(pipes[3][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "reset";
    len = sizeof(str);

    if (write(pipes[4][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    str = "ola";
    len = sizeof(str);

    if (write(pipes[4][1], str, len) != len) {
        write_log("Erro a escrever no pipe");
        return -1;
    }

    while(1){
        continue;
    }

    */

    return 0;
}