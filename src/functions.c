// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

// inicializa o programa
void init() {
    printf("SIMULATOR STARTING \n");

    // abertura do ficheiro log
    log_file = fopen("log.txt", "a");

    // inicializacao do semaforo para o fich log
    sem_unlink("SEM_LOG");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL, 0700, 1);

#if DEBUG
    if (sem_log == SEM_FAILED)
        write_log("[DEBUG] OPENING SEMAPHORE FOR LOG FAILED");
    else
        write_log("[DEBUG] SEMAPHORE FOR LOG CREATED");
#endif

    // inicializacao do semaforo para a memoria partilhada
    sem_unlink("SEM_SHM");
    sem_shm = sem_open("SEM_SHM", O_CREAT | O_EXCL, 0700, 1);

#if DEBUG
    if (sem_shm == SEM_FAILED)
        write_log("[DEBUG] OPENING SEMAPHORE FOR SHARED MEMORY FAILED");
    else
        write_log("[DEBUG] SEMAPHORE FOR SHARED MEMORY CREATED");
#endif

    // inicializacao sensor pipe
    if ((mkfifo(SENSOR_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        perror("mkfifo() failed");
        exit(1);
    }
    if ((fd_sensor_pipe = open(SENSOR_PIPE, O_RDWR)) == -1) {
        write_log("ERROR OPENING SENSOR PIPE");
        exit(1);
    }

    // inicializacao console pipe
    if ((mkfifo(CONSOLE_PIPE, O_CREAT | O_EXCL | 0600) < 0) && (errno != EEXIST)) {
        write_log("ERROR CREATING CONSOLE PIPE");
        exit(1);
    }

    if ((fd_console_pipe = open(CONSOLE_PIPE, O_RDWR)) == -1) {
        write_log("ERROR OPENING CONSOLE PIPE");
        exit(1);
    }


    init_shared_mem();
}

// inicializa a memoria partilhada
void init_shared_mem() {
    // inicializacao da memoria partilhada
    int size = sizeof(shm) + sizeof(sensor_data) + 1;
    if ((shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777)) == -1) {
        write_log("ERROR CREATING SHARED MEMORY");
        exit(1);
    }

    // attach da memoria partilhada
    if ((shared_memory = (shm*) shmat(shmid, (void*) 0, 0)) == (shm*) -1) {
        write_log("SHMAT ERROR");
        exit(1);
    }

#if DEBUG
    write_log("[DEBUG] SHARED MEMORY CREATED");
#endif

    // inicializacao dos campos da memoria partilhada
    shared_memory->queue_sz = 0;
    shared_memory->n_workers = 0;
    shared_memory->max_keys = 0;
    shared_memory->max_sensors = 0;
    shared_memory->max_alerts = 0;

}

// termina o programa
void terminate() {
    write_log("HOME_IOT SIMULATOR CLOSING");

    // terminar processos
    wait(NULL);

    // terminar semaforo do log
    sem_close(sem_log);
    sem_unlink("SEM_LOG");

    // terminar semaforo da memoria partilhada
    sem_close(sem_shm);
    sem_unlink("SEM_SHM");

    // terminar memoria partilhada
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    // fechar pipes
    close(fd_sensor_pipe);
    unlink(SENSOR_PIPE);

    close(fd_console_pipe);
    unlink(CONSOLE_PIPE);

    // fechar ficheiro log
    fclose(log_file);

    exit(0);
}

// escreve no ficheiro log e no ecra
void write_log(char* msg) {
    char* text = NULL;
    char* current_time = NULL;
    time_t t;
    time(&t);

    // obter tempo atual no formato "DIA_SEMANA MM DD HH:MM:SS YYYY"
    current_time = (char*) malloc(strlen(ctime(&t)) * sizeof(char) + 1);
    strcpy(current_time, ctime(&t));
    strtok(current_time, "\n");

    // obter a hora atual no formato "HH:MM:SS"
    char* hour = strtok(current_time, " ");

    for (int i = 0; i < 3; i++) {
        hour = strtok(NULL, " ");
    }

    // escrever no ficheiro log
    text = (char*) malloc((strlen(hour) + strlen(msg)) * sizeof(char) + 1);
    sprintf(text, "%s %s", hour, msg);
    sem_wait(sem_log);
    fprintf(log_file, "%s\n", text);
    fflush(log_file);
    sem_post(sem_log);

    printf("%s\n", text);

    // libertar memoria
    free(current_time);
    free(text);
}

// verifica se o argumento e um numero
bool is_digit(char argument[]) {
    int i = 0;
    while (argument[i] != '\0') {
        if (!isdigit(argument[i])) {
            return false;
        }
        i++;
    }
    return true;
}

void user_console() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("[DEBUG] USER CONSOLE SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "[DEBUG] USER CONSOLE SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif

    // apresentacao do menu
    printf("Menu:\n");
    printf("\texit\n\tSair do User Console\n\n");
    printf("\tstats\n\tApresenta estatísticas referentes aos dados enviados pelos sensores\n\n");
    printf("\treset\n\tLimpa todas as estatísticas calculadas até ao momento pelo sistema\n\n");
    printf("\tsensors\n\tLista todos os Sensorsque enviaram dados ao sistema\n\n");
    printf("\tadd_alert [id] [chave] [min] [max]\n\tAdiciona uma nova regra de alerta ao sistema\n\n");
    printf("\tremove_alert [id]\n\tRemove uma regra de alerta do sistema\n\n");
    printf("\tlist_alerts\n\tLista todas as regras de alerta que existem no sistema\n\n");

    char line[BUFFER_SIZE], instruction[5][BUFFER_SIZE];
    char* token;
    int fd_named_pipe;

    // abrir o pipe
    if ((fd_named_pipe = open(CONSOLE_PIPE, O_RDWR)) < 0) {
        write_log("ERROR OPENING CONSOLE PIPE");
        exit(1);
    }

    // leitura do input do utilizador
    while (fgets(line, BUFFER_SIZE, stdin) != NULL) {
        if (strlen(line) != 1) {
            int pos = 0;
            token = strtok(line, " \n");
            while (token != NULL) {
                strcpy(instruction[pos++], token);
                token = strtok(NULL, " \n");
            }

            if (strcmp(instruction[0], "exit") == 0) {
                close(fd_named_pipe);
                break;
            }
            else if (strcmp(instruction[0], "stats") == 0) {
                printf("Stats\n\n");
                write(fd_named_pipe, instruction[0], strlen(instruction[0]) + 1);
            }
            else if (strcmp(instruction[0], "reset") == 0) {
                printf("Reset\n\n");
            }
            else if (strcmp(instruction[0], "sensors") == 0) {
                printf("Sensors\n\n");
            }
            else if (strcmp(instruction[0], "add_alert") == 0) {
                // verificar se os argumentos sao validos
                if (strlen(instruction[1]) < 3 || strlen(instruction[1]) > 32) {
                    printf("ID INVALIDO\n\n");
                    continue;
                }
                if (strlen(instruction[2]) < 3 || strlen(instruction[2]) > 32) {
                    printf("CHAVE INVALIDO\n\n");
                    continue;
                }
                if (!is_digit(instruction[3])) {
                    printf("MIN INVALIDO\n\n");
                    continue;
                }
                if (!is_digit(instruction[4])) {
                    printf("MAX INVALIDO\n\n");
                    continue;
                }
                if (atoi(instruction[3]) > atoi(instruction[4])) {
                    printf("MIN MAIOR QUE MAX\n\n");
                    continue;
                }

                printf("OK\n\n");
#if DEBUG
                printf("id: %s\n", instruction[1]);
                printf("chave: %s\n", instruction[2]);
                printf("min: %s\n", instruction[3]);
                printf("max: %s\n\n", instruction[4]);
#endif
            }
            else if (strcmp(instruction[0], "remove_alert") == 0) {
                // verificar se os argumentos sao validos
                if (strlen(instruction[1]) < 3 || strlen(instruction[1]) > 32) {
                    printf("ID INVALIDO\n");
                    continue;
                }

                printf("Alert %s removed successfully!\n\n", instruction[1]);
            }
            else if (strcmp(instruction[0], "list_alerts") == 0) {
                printf("List_alerts\n\n");
            }
            // comando invalido
            else {
                printf("INVALID PARAMETER");
            }
        }
    }

    close(fd_named_pipe);
}

//{identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}
void sensor(char* id, int interval, char* key, int min, int max) {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("[DEBUG] SENSOR SUCCESSFULLY CREATED WITH THE PID.") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "[DEBUG] SENSOR SUCCESSFULLY CREATED WITH THE PID %d.", pid);
    write_log(text);
    free(text);
#endif

    int fd_named_pipe;

    // abrir o pipe
    if ((fd_named_pipe = open(SENSOR_PIPE, O_RDWR)) < 0) {
        write_log("ERROR OPENING SENSOR PIPE");
        exit(1);
    }
    // generate SENS1#HOUSETEM#20 with the 20 a random number between min and max every interval seconds

    char message[BUFFER_SIZE];
    while (1) {
        sprintf(message, "%s#%s#%d", id, key, rand() % (max - min + 1) + min);
        // print message
        printf("%s\n", message);
        write(fd_named_pipe, message, strlen(message) + 1);
        sleep(interval);
    }

    close(fd_named_pipe);
}

void worker() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("[DEBUG] WORKER SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "[DEBUG] WORKER SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif
    // faz coisas de worker
}


void alerts_watcher() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("[DEBUG] ALERTS WATCHER SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "[DEBUG] ALERTS WATCHER SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif
    //faz coisas de alerts watcher
}

// funcao que le o ficheiro de configuracao
void read_config(char* config_file) {
    write_log("READING CONFIG FILE");
    FILE* file;
    char* text;
    long file_size;

    // abrir ficheiro de configuracao
    if ((file = fopen(config_file, "r")) == NULL) {
        write_log("FAILED TO OPEN CONFIG FILE");
        exit(1);
    }

    // ir para o fim do ficheiro
    fseek(file, 0, SEEK_END);
    // obter o tamanho do ficheiro
    file_size = ftell(file);
    // voltar ao inicio do ficheiro
    rewind(file);

    // alocar memoria para o texto escrito no ficheiro de configuracao
    text = (char*) malloc(file_size + 1);

    // ler o ficheiro de configuracao
    fread(text, file_size, 1, file);
    // fechar o ficheiro de configuracao
    fclose(file);
    // adicionar o caracter de fim de string
    text[file_size] = '\0';

    // inicializar o semaforo
    sem_wait(sem_shm);

    // obter cada linha do texto lido do ficheiro de configuracao e verificar se e um numero inteiro
    char* token = strtok(text, "\n\r");
    if (!is_digit(token)) {
        write_log("QUEUE_SZ INVALID");
        exit(1);
    }
    if (atoi(token) < 1) {
        write_log("QUEUE_SZ INVALID");
        exit(1);
    }
    // guardar o tamanho da fila
    shared_memory->queue_sz = atoi(token);

    // obter cada linha do texto lido do ficheiro de configuracao e verificar se e um numero inteiro
    int i = 0;
    while (token != NULL) {
        token = strtok(NULL, "\n\r");
        if (i == 0) {
            if (!is_digit(token)) {
                write_log("N_WORKERS INVALID");
                exit(1);
            }

            if (atoi(token) < 1) {
                write_log("N_WORKERS INVALID");
                exit(1);
            }
            // guardar o numero de workers
            shared_memory->n_workers = atoi(token);
        }
        else if (i == 1) {
            if (!is_digit(token)) {
                write_log("MAX_KEYS INVALID");
                exit(1);
            }
            if (atoi(token) < 1) {
                write_log("MAX_KEYS INVALID");
                exit(1);
            }
            // guardar o numero maximo de chaves
            shared_memory->max_keys = atoi(token);
        }
        else if (i == 2) {
            if (!is_digit(token)) {
                write_log("MAX_SENSORS INVALID");
                exit(1);
            }
            if (atoi(token) < 1) {
                write_log("MAX_SENSORS INVALID");
                exit(1);
            }
            // guardar o numero maximo de sensores
            shared_memory->max_sensors = atoi(token);

            // TODO problema para depois
            shared_memory->sensors = (sensor_data*) malloc(sizeof(sensor_data) * shared_memory->max_sensors);

            // inicializar os sensores
            for (int j = 0; j < shared_memory->max_sensors; j++) {
                strcpy(shared_memory->sensors[j].id, "");
                strcpy(shared_memory->sensors[j].key, "");
                shared_memory->sensors[j].interval = 0;
                shared_memory->sensors[j].min = 0;
                shared_memory->sensors[j].max = 0;
            }
        }
        else if (i == 3) {
            if (!is_digit(token)) {
                write_log("MAX_ALERTS INVALID");
                exit(1);
            }
            if (atoi(token) < 0) {
                write_log("MAX_ALERTS INVALID");
                exit(1);
            }
            // guardar o numero maximo de alertas
            shared_memory->max_alerts = atoi(token);
        }
        i++;
    }
    // libertar o semaforo
    sem_post(sem_shm);

    // libertar memoria
    free(text);
}

// thread console reader
void* console_reader() {
    int received_length;
    char received[BUFFER_SIZE];

    write_log("THREAD CONSOLE_READER CREATED");

    while (1) {
        received_length = read(fd_console_pipe, received, sizeof(received));
        received[received_length - 1] = '\0';
        printf("RECEIVED: %s\n", received);
    }

    pthread_exit(NULL);
    return NULL;
}

// thread sensor reader
void* sensor_reader() {
    int received_length;
    char received[BUFFER_SIZE];

    write_log("THREAD SENSOR_READER CREATED");

    while (1) {
        received_length = read(fd_sensor_pipe, received, sizeof(received));
        received[received_length - 1] = '\0';
        printf("SENSOR: %s\n", received);
    }    pthread_exit(NULL);
    return NULL;
}

// thread dispatcher
void* dispatcher() {
    write_log("THREAD DISPATCHER CREATED");
    // faz coisas de dispatcher
    pthread_exit(NULL);
    return NULL;
}

// funcao que cria as threads, os workers e o alerts watcher
void system_manager(char* config_file) {
    init();

    write_log("HOME_IOT SIMULATOR STARTING");

    // ler config file
    read_config(config_file);

    write_log("CONFIG FILE READ");

    // criar threads
    pthread_create(&console_reader_thread, NULL, console_reader, NULL);
    pthread_create(&sensor_reader_thread, NULL, sensor_reader, NULL);
    pthread_create(&dispatcher_thread, NULL, dispatcher, NULL);

    int n = shared_memory->n_workers;
    // inicializar worker
    for (int i = 0; i < n; i++) {
        int worker_forks;
        if ((worker_forks = fork()) == 0) {
            char* text = (char*) malloc((strlen("WORKER  READY") + sizeof(i)) * sizeof(char) + 1);
            sprintf(text, "WORKER %d READY", i);
            write_log(text);
            free(text);
            worker();
            exit(0);
        }
        else if (worker_forks == -1) {
            write_log("ERROR CREATING WORKER");
            exit(1);
        }
    }

    // inicializar alerts_watcher
    int alerts_watcher_fork;
    if ((alerts_watcher_fork = fork()) == 0) {
        write_log("PROCESS ALERTS_WATCHER CREATED");
        alerts_watcher();
        exit(0);
    }
    else if (alerts_watcher_fork == -1) {
        write_log("ERROR CREATING ALERTS WATCHER");
        exit(1);
    }

    for (int i = 0; i < n; i++) {
        wait(NULL);
    }

    wait(NULL);

    // terminar as threads
    pthread_join(console_reader_thread, NULL);
    pthread_join(sensor_reader_thread, NULL);
    pthread_join(dispatcher_thread, NULL);
}