/*
Ana Rita Martins Oliveira 2020213684
Hugo Sobral de Barros 2020234332
*/


#include "functions.h"

void init() {
    log_file = fopen("log.txt", "a");

    // inicializacao do semaforo para o fich log
    sem_unlink("SEM_LOG");
    sem_unlink("SEM_SHM");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL, 0700, 1);
    sem_shm = sem_open("SEM_SHM", O_CREAT | O_EXCL, 0700, 1);
#if DEBUG
    if (sem_log == SEM_FAILED)
        write_log("OPENING SEMAPHORE FOR LOG FAILED");
    else
        write_log("SEMAPHORE FOR LOG CREATED");

    if (sem_shm == SEM_FAILED)
        write_log("OPENING SEMAPHORE FOR SHARED MEMORY FAILED");
    else
        write_log("SEMAPHORE FOR SHARED MEMORY CREATED");
#endif

    init_shared_mem();
}

void init_shared_mem() {
    // inicializacao da memoria partilhada
    int size = sizeof(shm) + sizeof(sensor_data);
    if ((shmid = shmget(IPC_PRIVATE, size, IPC_CREAT | 0777)) == -1) {
        write_log("ERROR CREATING SHARED MEMORY");
        exit(1);
    }
    if ((shared_memory = (shm*) shmat(shmid, (void*) 0, 0)) == (shm*) -1) {
        write_log("SHMAT ERROR");
        exit(1);
    }
#if DEBUG
    write_log("SHARED MEMORY CREATED");
#endif
    shared_memory->queue_sz = 0;
    shared_memory->n_workers = 0;
    shared_memory->max_keys = 0;
    shared_memory->max_sensors = 0;
    shared_memory->max_alerts = 0;
    shared_memory->sensors = NULL;
}

void terminate() {
    write_log("HOME_IOT SIMULATOR CLOSING");

    sem_close(sem_log);
    sem_unlink("SEM_LOG");

    sem_close(sem_shm);
    sem_unlink("SEM_SHM");

    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    fclose(log_file);

    exit(0);
}

void write_log(char* msg) {
    char* text = NULL;
    char* current_time = NULL;
    time_t t;
    time(&t);

    current_time = (char*) malloc(strlen(ctime(&t)) * sizeof(char) + 1);
    strcpy(current_time, ctime(&t));
    strtok(current_time, "\n");

    char* hour = strtok(current_time, " ");

    for (int i = 0; i < 3; i++) {
        hour = strtok(NULL, " ");
    }

    text = (char*) malloc((strlen(hour) + strlen(msg)) * sizeof(char) + 1);
    sprintf(text, "%s %s", hour, msg);
    sem_wait(sem_log);
    fprintf(log_file, "%s\n", text);
    fflush(log_file);
    sem_post(sem_log);

    printf("%s\n", text);

    free(current_time);
    free(text);
}

bool is_digit(char argument[]) {
    int i = 0;
    while (argument[i] != '\0') {
        if (!isdigit(argument[i])) {
            write_log("INVALID ARGUMENT");
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
    text = (char*) malloc((strlen("USER CONSOLE SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "USER CONSOLE SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif

    printf("exit\nSair do User Console\n\n");
    printf("stats\nApresenta estatísticas referentes aos dados enviados pelos sensores\n\n");
    printf("reset\nLimpa todas as estatísticas calculadas até ao momento pelo sistema\n\n");
    printf("sensors\nLista todos os Sensorsque enviaram dados ao sistema\n\n");
    printf("add_alert [id] [chave] [min] [max]\nAdiciona uma nova regra de alerta ao sistema\n\n");
    printf("remove_alert [id]\nRemove uma regra de alerta do sistema\n\n");
    printf("list_alerts\nLista todas as regras de alerta que existem no sistema\n\n");

    char line[BUFFER_SIZE], instruction[5][BUFFER_SIZE];
    char* token;

    while (fgets(line, BUFFER_SIZE, stdin) != NULL) {
        if (strlen(line) != 1) {
            int pos = 0;
            token = strtok(line, " \n");
            while (token != NULL) {
                strcpy(instruction[pos++], token);
                token = strtok(NULL, " \n");
            }

            if (strcmp(instruction[0], "exit") == 0) {
                break;
            }
            else if (strcmp(instruction[0], "stats") == 0) {
                printf("Stats\n");
            }
            else if (strcmp(instruction[0], "reset") == 0) {
                printf("Reset\n");
            }
            else if (strcmp(instruction[0], "sensors") == 0) {
                printf("Sensors\n");
            }
            else if (strcmp(instruction[0], "add_alert") == 0) { // TODO verificar se id já existe
                if (atoi(instruction[1]) < 3 || atoi(instruction[1]) > 32) {
                    write_log("ID INVALIDO");
                    continue;
                }
                if (!is_digit(instruction[2]) || !is_digit(instruction[3])) { // TODO faltam coisas
                    write_log("MIN OU MAX INVALIDO");
                    continue;
                }

                printf("Add_alert\n");
                printf("id: %s\n", instruction[1]);
                printf("chave: %s\n", instruction[2]);
                printf("min: %s\n", instruction[3]);
                printf("max: %s\n", instruction[4]);
            }
            else if (strcmp(instruction[0], "remove_alert") == 0) {
                if (atoi(instruction[1]) < 3 || atoi(instruction[1]) > 32) {
                    write_log("ID INVALIDO");
                    continue;
                }

                printf("Remove_alert\n");
                printf("id: %s\n", instruction[1]);
            }
            else if (strcmp(instruction[0], "list_alerts") == 0) {
                printf("List_alerts\n");
            }
            else {
                write_log("INVALID PARAMETER\n");
            }
        }
    }

    exit(0);
}

//{identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}
void sensor(char* id, char* interval, char* key, char* min, char* max) {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("SENSOR SUCCESSFULLY CREATED WITH THE PID.") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "SENSOR SUCCESSFULLY CREATED WITH THE PID %d.", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void worker() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("WORKER SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "WORKER SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void alerts_watcher() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("ALERTS WATCHER SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "ALERTS WATCHER SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void read_config(char* config_file) {
    write_log("READING CONFIG FILE");
    FILE* file;
    char* text;
    long file_size;

    if ((file = fopen(config_file, "r")) == NULL) {
        write_log("FAILED TO OPEN CONFIG FILE");
        exit(1);
    }

    fseek(file, 0, SEEK_END);
    file_size = ftell(file);
    rewind(file);

    text = (char*) malloc(file_size + 1);

    fread(text, file_size, 1, file);
    text[file_size] = '\0';

    char* token = strtok(text, "\n");
    sem_wait(sem_shm);
    shared_memory->queue_sz = atoi(token);

    int i = 0;
    while (token != NULL) {
        token = strtok(NULL, "\n");
        if (i == 0) {
            shared_memory->n_workers = atoi(token);
        }
        else if (i == 1) {
            shared_memory->max_keys = atoi(token);
        }
        else if (i == 2) {
            shared_memory->max_sensors = atoi(token);
        }
        else if (i == 3) {
            shared_memory->max_alerts = atoi(token);
        }
        i++;
    }
    sem_post(sem_shm);

    free(text);
    fclose(file);
}

void system_manager(char* config_file) {
    // inicializar programa
    init();

    write_log("HOME_IOT SIMULATOR STARTING");
    
    // ler config file
    read_config(config_file);

    write_log("CONFIG FILE READ");
    
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
    }

    // criacao de threads vvvv

    terminate();
}