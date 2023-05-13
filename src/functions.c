// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

/*
    * meter as posicoes da lista a -1 e se for -1 e porque esta vazia escrever la
    * meter a cena das condicoes dos pids e depois o kill no terminate
    * meter os mutexes
    * meter as variaveis de condicao
    * no dispatcher ele faz o signal
    * nos readers ele faz o cond wait pelo signal do dispatcher
    *
    * criar a internal queue dentro do system manager como diz no enunciado
    *
    * as mensagens enviadas pelo sensor tem menor prioridade que as mensagens enviadas pelo console reader
    * enviando primeiro as mensagens do console reader e depois as do sensor para os processos worker
    *
    * se a fila estiver cheia, descarta a mensagem do sensor
    * se a fila estiver cheia, da lock no mutex e espera que a fila fique vazia para meter a cena do console reader
    *
    * os workers  e o dispatcher comunicam com unnamed pipes
    * quando uma entrada é enviada para um worker, essa entrada é removida da lista e o worker passa a estar 'busy' para que o dispatcher nao lhe volte a enviar uma tarefa ate ele acabar a que recebeu antes
    *
    * quando um worker termina de processar o pedido, deve colocar o seu estado como 'free'
    *
    * ha 2 tipos de mensagens, as que sao enviadas pelo user console e as que sao enviadas peloo sensor
*/

#include "functions.h"

// inicializa o programa
void init() {
    printf("SIMULATOR STARTING \n");

    terminate_threads = 0;

    // abertura do ficheiro log
    log_file = fopen("log.txt", "a");

    // inicializacao do semaforo para o fich log
    sem_unlink("SEM_LOG");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL, 0700, 1);

    if (sem_log == SEM_FAILED) {
        write_log("[DEBUG] OPENING SEMAPHORE FOR LOG FAILED");
        terminate();
    }
#if DEBUG
    write_log("[DEBUG] SEMAPHORE FOR LOG CREATED");
#endif

    // inicializacao do semaforo para a memoria partilhada
    sem_unlink("SEM_SHM");
    sem_shm = sem_open("SEM_SHM", O_CREAT | O_EXCL, 0700, 1);

    if (sem_shm == SEM_FAILED) {
        write_log("OPENING SEMAPHORE FOR SHARED MEMORY FAILED");
        terminate();
    }
#if DEBUG
    write_log("[DEBUG] SEMAPHORE FOR SHARED MEMORY CREATED");
#endif

    // inicializacao do semaforo para os workers
    sem_unlink("SEM_WORKERS");
    sem_workers = sem_open("SEM_WORKERS", O_CREAT | O_EXCL, 0700, 1);

    if (sem_workers == SEM_FAILED) {
        write_log("OPENING SEMAPHORE FOR WORKERS FAILED");
        terminate();
    }

    // inicializacao do semaforo para os workers
    sem_unlink("SEM_ALERTS");
    sem_alerts = sem_open("SEM_ALERTS", O_CREAT | O_EXCL, 0700, 1);

    if (sem_alerts == SEM_FAILED) {
        write_log("OPENING SEMAPHORE FOR ALERTS FAILED");
        terminate();
    }
#if DEBUG
    write_log("[DEBUG] SEMAPHORE FOR WORKERS CREATED");
#endif

    // inicializacao a raiz da internal queue
    //root = NULL;

    // inicializacao dos mutexes
    mutex_internal_queue = (pthread_mutex_t) PTHREAD_MUTEX_INITIALIZER;

    // inicializacao da variavel de condicao
    cond_internal_queue = (pthread_cond_t) PTHREAD_COND_INITIALIZER;

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


}

// inicializa a memoria partilhada
void init_shared_mem() {
    // inicializacao da memoria partilhada
    int size = sizeof(shm) + sizeof(sensor_data) * config.max_sensors + sizeof(key_data) * config.max_keys + sizeof(alert_data) * config.max_alerts + sizeof(int*) + 10;
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
    // shared_memory->queue_sz = 0;
    // shared_memory->n_workers = 0;
    // shared_memory->max_keys = 0;
    // shared_memory->max_sensors = 0;
    // shared_memory->max_alerts = 0;

}

// termina o programa
void terminate() {
    write_log("HOME_IOT SIMULATOR CLOSING");

    terminate_threads = 1;

    // terminar processos
    wait(NULL);

    // terminar semaforo do log
    sem_close(sem_log);
    sem_unlink("SEM_LOG");

    // terminar semaforo da memoria partilhada
    sem_close(sem_shm);
    sem_unlink("SEM_SHM");

    // terminar semaforo dos workers
    sem_close(sem_workers);
    sem_unlink("SEM_WORKERS");

    // terminar semaforo dos alerts
    sem_close(sem_alerts);
    sem_unlink("SEM_ALERTS");

    pthread_mutex_destroy(&mutex_internal_queue);

    // terminar memoria partilhada
    shmdt(shared_memory);
    shmctl(shmid, IPC_RMID, NULL);

    // fechar pipes
    close(fd_sensor_pipe);
    unlink(SENSOR_PIPE);

    close(fd_console_pipe);
    unlink(CONSOLE_PIPE);

    // fechar message queue
    //msgctl(msgq_id, IPC_RMID, NULL);

    // fechar ficheiro log
    fclose(log_file);

    exit(0);
}

// escreve no ficheiro log e no ecra
void write_log(char* msg) {
    char* text = NULL;
    char* current_time = NULL;
    char* hour = NULL;
    time_t t;
    time(&t);

    // obter tempo atual no formato "DIA_SEMANA MM DD HH:MM:SS YYYY"
    current_time = (char*) malloc(strlen(ctime(&t)) * sizeof(char) + 1);
    strcpy(current_time, ctime(&t));
    strtok(current_time, "\n");

    // obter a hora atual no formato "HH:MM:SS"
    hour = strtok(current_time, " ");

    for (int i = 0; i < 3; i++) {
        hour = strtok(NULL, " ");
    }

    // escrever no ficheiro log
    text = (char*) malloc((strlen(hour) + strlen(msg)) * sizeof(char) + 3);
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
    char line[BUFFER_SIZE], instruction[5][BUFFER_SIZE];
    char* token;
    int fd_named_pipe;

    // abrir o pipe
    if ((fd_named_pipe = open(CONSOLE_PIPE, O_RDWR)) < 0) {
        printf("ERROR OPENING CONSOLE PIPE");
        exit(1);
    }
    /* // inicializar message queue
    if ((msgq_id = msgget((int) QUEUE_NAME, IPC_CREAT | 0777)) == -1) {
        write_log("ERROR CREATING MESSAGE QUEUE");
        exit(1);
    } */

    // apresentacao do menu
    printf("Menu:\n");
    printf("\texit\n\tSair do User Console\n\n");
    printf("\tstats\n\tApresenta estatísticas referentes aos dados enviados pelos sensores\n\n");
    printf("\treset\n\tLimpa todas as estatísticas calculadas até ao momento pelo sistema\n\n");
    printf("\tsensors\n\tLista todos os Sensorsque enviaram dados ao sistema\n\n");
    printf("\tadd_alert [id] [chave] [min] [max]\n\tAdiciona uma nova regra de alerta ao sistema\n\n");
    printf("\tremove_alert [id]\n\tRemove uma regra de alerta do sistema\n\n");
    printf("\tlist_alerts\n\tLista todas as regras de alerta que existem no sistema\n\n");


    // leitura do input do utilizador
    while (fgets(line, BUFFER_SIZE, stdin) != NULL) {
        instruction[0][0] = '\0';
        instruction[1][0] = '\0';
        instruction[2][0] = '\0';
        instruction[3][0] = '\0';
        instruction[4][0] = '\0';

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
                write(fd_named_pipe, instruction[0], strlen(instruction[0]) + 1);

                printf("Key Last Min Max Avg Count\n");

                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 1, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
            }

            else if (strcmp(instruction[0], "reset") == 0) {
                write(fd_named_pipe, instruction[0], strlen(instruction[0]) + 1);

                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 2, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
            }

            else if (strcmp(instruction[0], "sensors") == 0) {
                write(fd_named_pipe, instruction[0], strlen(instruction[0]) + 1);

                printf("ID\n");

                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 3, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
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
                char msg[BUFFER_SIZE * 5];
                sprintf(msg, "%s %s %s %s %s", instruction[0], instruction[1], instruction[2], instruction[3], instruction[4]);
                write(fd_named_pipe, msg, strlen(msg) + 1);
#if DEBUG
                printf("id: %s\n", instruction[1]);
                printf("chave: %s\n", instruction[2]);
                printf("min: %s\n", instruction[3]);
                printf("max: %s\n\n", instruction[4]);
#endif
                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 4, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
            }

            else if (strcmp(instruction[0], "remove_alert") == 0) {
                // verificar se os argumentos sao validos
                if (strlen(instruction[1]) < 3 || strlen(instruction[1]) > 32) {
                    printf("ID INVALIDO\n");
                    continue;
                }
                char msg[BUFFER_SIZE * 2];
                sprintf(msg, "%s %s", instruction[0], instruction[1]);
                printf("remover: %s\n", instruction[1]);
                printf("msg: %s\n", msg);
                write(fd_named_pipe, msg, strlen(msg) + 1);

                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 5, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
            }

            else if (strcmp(instruction[0], "list_alerts") == 0) {
                printf("List_alerts\n\n");
                write(fd_named_pipe, instruction[0], strlen(instruction[0]) + 1);

                printf("ID Key MIN MAX\n");

                key_t key = ftok("msgfile", 'A');
                int msgq_id = msgget(key, 0666 | IPC_CREAT);
                msgq message;

                if (msgq_id == -1) {
                    printf("Erro ao recuperar a fila de mensagens.\n");
                    exit(EXIT_FAILURE);
                }

                int max_msg_size = BUFFER_SIZE;

                if (msgrcv(msgq_id, &message, max_msg_size, 6, 0) == -1) {
                    printf("Erro ao receber mensagem.\n");
                    exit(EXIT_FAILURE);
                }

                printf("%s\n", message.mtext);
            }

            // comando invalido
            else {
                printf("INVALID PARAMETER\n\n");
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
        printf("ERROR OPENING SENSOR PIPE");
        exit(1);
    }

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

void worker(int id) {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char*) malloc((strlen("[DEBUG] WORKER SUCCESSFULLY CREATED WITH THE PID") + sizeof(pid)) * sizeof(char) + 1);
    sprintf(text, "[DEBUG] WORKER SUCCESSFULLY CREATED WITH THE PID %d", pid);
    write_log(text);
    free(text);
#endif
    char* textx = (char*) malloc((strlen("WORKER  READY") + sizeof(id)) * sizeof(char) + 1);
    sprintf(textx, "WORKER %d READY", id);
    write_log(textx);
    free(textx);

    char buffer[BUFFER_SIZE];

    /* // inicializar message queue
    if ((msgq_id = msgget(QUEUE_NAME, IPC_CREAT | 0777)) == -1) {
        write_log("ERROR CREATING MESSAGE QUEUE");
        exit(1);
    } */

    while (1) {
        // read [0] || write [1]
        if (read(unnamed_pipes[id][0], buffer, BUFFER_SIZE) < 0) {
            printf("ERROR READING FROM PIPE");
            terminate();
        }
        sem_wait(sem_shm);
        // worker busy
        shared_memory->workers_list[id] = 1;
        sem_post(sem_shm);
        sem_wait(sem_workers);


#ifdef DEBUG
        printf("[DEBUG] WORKER %d BUSY\n", id);
#endif

        printf("buffer: %s\n", buffer);

        int aux = 0;
        char instruction[5][BUFFER_SIZE];
        instruction[0][0] = '\0';
        instruction[1][0] = '\0';
        instruction[2][0] = '\0';
        instruction[3][0] = '\0';
        instruction[4][0] = '\0';

        char* token = strtok(buffer, " ");
        while (token != NULL) {
            strcpy(instruction[aux++], token);
            token = strtok(NULL, " ");
        }

        // receber dados consol
        if (strcmp(instruction[0], "stats") == 0) {
            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 1;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';
            char msg_stats[BUFFER_SIZE];
            msg_stats[0] = '\0';
            for (int i = 0; i < config.max_keys; i++) {
                if (strcmp(shared_memory->keys[i].name, "") != 0) {
                    sprintf(msg_stats, "%s %d %d %d %lf %d\n", shared_memory->keys[i].name, shared_memory->keys[i].last, shared_memory->keys[i].min, shared_memory->keys[i].max, shared_memory->keys[i].mean, shared_memory->keys[i].changes);
                    strcat(msg, msg_stats);
                }
                else {
                    break;
                }
            }

            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        else if (strcmp(instruction[0], "reset") == 0) {
            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 2;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';

            for (int i = 0; i < config.max_sensors; i++) {
                strcpy(shared_memory->sensors[i].id, "");
            }

            for (int i = 0; i < config.max_keys; i++) {
                strcpy(shared_memory->keys[i].name, "");
                shared_memory->keys[i].last = 0;
                shared_memory->keys[i].min = 0;
                shared_memory->keys[i].max = 0;
                shared_memory->keys[i].mean = 0;
                shared_memory->keys[i].changes = 0;
            }

            strcat(msg, "OK\n");

            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        else if (strcmp(instruction[0], "sensors") == 0) {
            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 3;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';
            char msg_stats[BUFFER_SIZE];
            msg_stats[0] = '\0';
            for (int i = 0; i < config.max_sensors; i++) {
                if (strcmp(shared_memory->sensors[i].id, "") != 0) {
                    sprintf(msg_stats, "%s\n", shared_memory->sensors[i].id);
                    strcat(msg, msg_stats);
                }
                else {
                    break;
                }
            }


            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        else if (strcmp(instruction[0], "add_alert") == 0) {
            printf("WORKER: Add_alert\n");

            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 4;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';
            int space_alert = 0;
            for (int i = 0; i < config.max_alerts; i++) {
                if (strcmp(shared_memory->alerts[i].id, "") == 0) {
                    space_alert = 1;
                    strcpy(shared_memory->alerts[i].id, instruction[1]);
                    strcpy(shared_memory->alerts[i].key, instruction[2]);
                    shared_memory->alerts[i].min = atoi(instruction[3]);
                    shared_memory->alerts[i].max = atoi(instruction[4]);
                    strcat(msg, "OK\n");
                    break;
                }
            }

            if (space_alert == 0) {
                strcat(msg, "ERROR\n");
            }

            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        else if (strcmp(instruction[0], "remove_alert") == 0) {
            printf("WORKER: Remove_alert\n");

            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 5;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';
            int find_alert = 0;
            int pos;

            for (int i = 0; i < config.max_alerts; i++) {
                if (strcmp(instruction[1], shared_memory->alerts[i].id) == 0) {
                    find_alert = 1;
                    pos = i;
                    strcpy(shared_memory->alerts[i].id, "");
                    strcpy(shared_memory->alerts[i].key, "");
                    shared_memory->alerts[i].min = 0;
                    shared_memory->alerts[i].max = 0;
                    break;
                }
            }

            if (find_alert == 0) {
                strcat(msg, "ERROR\n");
            }

            for (int i = pos; i < config.max_alerts; i++) {
                if (i == config.max_alerts - 1) {
                    strcpy(shared_memory->alerts[i].id, "");
                    strcpy(shared_memory->alerts[i].key, "");
                    shared_memory->alerts[i].min = 0;
                    shared_memory->alerts[i].max = 0;
                    break;
                }
                else if (strcmp(shared_memory->alerts[i].id, "") == 0 && strcmp(shared_memory->alerts[i + 1].id, "") == 0) {
                    break;
                }
                strcpy(shared_memory->alerts[i].id, shared_memory->alerts[i + 1].id);
                strcpy(shared_memory->alerts[i].key, shared_memory->alerts[i + 1].key);
                shared_memory->alerts[i].min = shared_memory->alerts[i + 1].min;
                shared_memory->alerts[i].max = shared_memory->alerts[i + 1].max;
            }

            strcat(msg, "OK\n");

            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        else if (strcmp(instruction[0], "list_alerts") == 0) {
            printf("WORKER: List_alerts\n");

            key_t key = ftok("msgfile", 'A');
            int msgq_id = msgget(key, 0666 | IPC_CREAT);
            msgq message;

            if (msgq_id == -1) {
                perror("Erro ao criar ou recuperar a fila de mensagens");
                exit(EXIT_FAILURE);
            }

            message.mtype = 6;

            char msg[BUFFER_SIZE];
            msg[0] = '\0';
            char msg_stats[BUFFER_SIZE];
            msg_stats[0] = '\0';
            for (int i = 0; i < config.max_alerts; i++) {
                if (strcmp(shared_memory->alerts[i].id, "") != 0) {
                    printf("Aqui\n");
                    sprintf(msg_stats, "%s %s %d %d\n", shared_memory->alerts[i].id, shared_memory->alerts[i].key, shared_memory->alerts[i].min, shared_memory->alerts[i].max);
                    strcat(msg, msg_stats);
                }
                else {
                    printf("break\n");
                    break;
                }
            }

            int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem
            strncpy(message.mtext, msg, max_msg_size);

            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                printf("Erro ao enviar mensagem.\n");
                exit(EXIT_FAILURE);
            }
        }

        // receber dados sensor
        else {
            char* token = strtok(buffer, "#");
            char* name = token;

            int space = 0;

            sem_wait(sem_shm);
            int found_sensor = 0;
            for (int i = 0; i < config.max_sensors; i++) {
                if (strcmp(shared_memory->sensors[i].id, name) == 0) {
                    found_sensor = 1;
                    space = 1;
                    break;
                }
            }
            sem_post(sem_shm);

            sem_wait(sem_shm);
            if (found_sensor == 0) {
                for (int i = 0; i < config.max_sensors; i++) {
                    if (strcmp(shared_memory->sensors[i].id, "") == 0) {
                        space = 1;
                        strcpy(shared_memory->sensors[i].id, name);
                        break;
                    }
                }
            }
            sem_post(sem_shm);

            if (space == 0) {
                write_log("NO SPACE FOR NEW SENSOR INFORMATION DISCARDED");
            }
            else {


                token = strtok(NULL, "#");
                char* key = token;



                token = strtok(NULL, "#");
                char* value = token;
                // verificar se a key existe na lista de keys
                int found = 0;
                sem_wait(sem_shm);
                for (int i = 0; i < config.max_keys; i++) {
                    if (strcmp(shared_memory->keys[i].name, key) == 0) {
                        found = 1;
                        // atualizar os valores
                        shared_memory->keys[i].last = atoi(value);

                        if (atoi(value) < shared_memory->keys[i].min) {
                            shared_memory->keys[i].min = atoi(value);
                        }

                        if (atoi(value) > shared_memory->keys[i].max) {
                            shared_memory->keys[i].max = atoi(value);
                        }

                        shared_memory->keys[i].mean = (shared_memory->keys[i].mean * shared_memory->keys[i].changes + atoi(value)) / (shared_memory->keys[i].changes + 1);

                        shared_memory->keys[i].changes++;

                        shared_memory->keys[i].alert = 0;

                        char* send = (char*) malloc((strlen("WORKER:  DATA PROCESSING COMPLETED") + sizeof(id) + sizeof(key)) * sizeof(char) + 1);
                        sprintf(send, "WORKER%d: %s DATA PROCESSING COMPLETED", id, key);
                        write_log(send);
                        free(send);

                        break;
                    }
                }

                if (found == 0) {
                    int key_flag = 0;
                    // se nao existir
                    // procurar por um espaco vazio e adicionar a key
                    for (int i = 0; i < config.max_keys; i++) {
                        if (strcmp(shared_memory->keys[i].name, "") == 0) {
                            key_flag = 1;
                            strcpy(shared_memory->keys[i].name, key);
                            shared_memory->keys[i].last = atoi(value);
                            shared_memory->keys[i].min = atoi(value);
                            shared_memory->keys[i].max = atoi(value);
                            shared_memory->keys[i].mean = atoi(value);
                            shared_memory->keys[i].changes = 1;
                            break;
                        }
                    }

                    if (key_flag == 0) {
                        write_log("NO SPACE FOR NEW KEY INFORMATION DISCARDED");
                    }
                }
            }

            sem_post(sem_shm);
        }




        // worker ready
        sem_post(sem_workers);
        sem_wait(sem_shm);
        shared_memory->workers_list[id] = 0;
        sem_post(sem_shm);
#ifdef DEBUG
        printf("[DEBUG] WORKER %d READY\n", id);
#endif

    }
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

    key_t key = ftok("msgfile", 'A');
    int msgq_id = msgget(key, 0666 | IPC_CREAT);
    msgq message;

    if (msgq_id == -1) {
        perror("Erro ao criar ou recuperar a fila de mensagens");
        exit(EXIT_FAILURE);
    }

    message.mtype = 7;

    char msg[BUFFER_SIZE];
    char msg_alert[BUFFER_SIZE];
    int max_msg_size = BUFFER_SIZE; // tamanho máximo para a mensagem


    while (1) {
        //percorrer as keys
        // percorrer os alertas se alert for != 1 e encontrar o alerta com a key
        // verificar se o valor atual é maior ou menor que o valor do alerta
        // se for, enviar mensagem para a msgq
        // se nao for, nao fazer nada
        // se nao encontrar o alerta, nao fazer nada
        // se o alerta ja tiver sido enviado, nao fazer nada
        sem_wait(sem_alerts);
        for (int i = 0; i < config.max_keys; i++) {
            msg[0] = '\0';
            msg_alert[0] = '\0';
            if (strcmp(shared_memory->keys[i].name, "") != 0) {
                for (int j = 0; j < config.max_alerts; j++) {
                    if (strcmp(shared_memory->alerts[j].key, shared_memory->keys[i].name) == 0) {
                        printf("KEY: %s\n", shared_memory->keys[i].name);
                        if ((shared_memory->keys[i].last > shared_memory->alerts[j].max) || (shared_memory->keys[i].last < shared_memory->alerts[j].min)) {
                            sprintf(msg_alert, "ALERT:  KEY %s VALUE %d", shared_memory->keys[i].name, shared_memory->keys[i].last);
                            write_log(msg_alert);
                            strcat(msg, msg_alert);

                            strncpy(message.mtext, msg, max_msg_size);

                            if (msgsnd(msgq_id, &message, max_msg_size, 0) == -1) {
                                printf("Erro ao enviar mensagem.\n");
                                exit(EXIT_FAILURE);
                            }
                        }
                    }
                }
            }
        }
    }
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
    config.queue_sz = atoi(token);

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
            config.n_workers = atoi(token);
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
            config.max_keys = atoi(token);
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
            config.max_sensors = atoi(token);


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
            config.max_alerts = atoi(token);

        }
        i++;
    }
    init_shared_mem();

    shared_memory->sensors = (sensor_data*) (((void*) shared_memory) + sizeof(shm));

    for (int j = 0; j < config.max_sensors; j++) {
        strcpy(shared_memory->sensors[j].id, "");
    }

    shared_memory->keys = (key_data*) (((void*) shared_memory->sensors) + sizeof(sensor_data) * config.max_sensors);

    // inicializar a lista de chaves
    for (int j = 0; j < config.max_keys; j++) {
        strcpy(shared_memory->keys[j].name, "");
        shared_memory->keys[j].last = 0;
        shared_memory->keys[j].min = 0;
        shared_memory->keys[j].max = 0;
        shared_memory->keys[j].mean = 0;
        shared_memory->keys[j].changes = 0;
    }

    shared_memory->alerts = (alert_data*) (((void*) shared_memory->keys) + sizeof(key_data) * config.max_keys);

    // inicializar a lista de alertas
    for (int j = 0; j < config.max_alerts; j++) {
        printf("MAX_KEYS: %d\n", config.max_keys);
        printf("MAX_ALERTS: %d\n", config.max_alerts);
        printf("j: %d\n", j);
        strcpy(shared_memory->alerts[j].id, "");
        strcpy(shared_memory->alerts[j].key, "");
        shared_memory->alerts[j].min = 0;
        shared_memory->alerts[j].max = 0;
    }
    // libertar o semaforo
    sem_post(sem_shm);

    // libertar memoria
    free(text);
}

// cria um novo no na internal queue
node* create_new_node(char* msg, int priority) {
    node* temp = (node*) malloc(sizeof(node));
    temp->msg = (char*) malloc((strlen(msg) + 1) * sizeof(char));
    strcpy(temp->msg, msg);
    temp->priority = priority;
    temp->next = NULL;

    return temp;
}

// insere um novo no na internal queue
void push(node** root, char* msg, int priority) {
    node* start = (*root);
    node* temp = create_new_node(msg, priority);

    printf("PUSH: %s\n", msg);

    if ((*root)->priority > priority) {
        temp->next = *root;
        (*root) = temp;
    }
    else {
        while (start->next != NULL && start->next->priority < priority) {
            start = start->next;
        }

        temp->next = start->next;
        start->next = temp;
    }
}

// remove um no da internal queue
void pop(node** root) {
    node* temp = *root;
    (*root) = (*root)->next;
    free(temp->msg);
    free(temp);
}

// verifica se a internal queue esta vazia
int is_empty(node** root) {
    return (*root) == NULL;
}

// retorna o tamanho da internal queue
int size(node* root) {
    int count = 0;
    node* aux = root;

    while (aux != NULL) {
        count++;
        aux = aux->next;
    }

    return count;
}

// thread console reader
void* console_reader() {
    if (getppid() == getpid()) {
        write_log("CONSOLE_READER ALREADY CREATED");
        pthread_exit(NULL);
        return NULL;
    }
    int received_length;
    char received[BUFFER_SIZE];

    write_log("THREAD CONSOLE_READER CREATED");

    while (!terminate_threads) {
        received_length = read(fd_console_pipe, received, sizeof(received));
        received[received_length - 1] = '\0';
        printf("RECEIVED: %s\n", received);
        pthread_mutex_lock(&mutex_internal_queue);

        if (size(root) > config.queue_sz) {  // TODO verificar se esta correto -> congif.queue_sz ou shared_memory->queue_sz??????????????
            printf("QUEUE FULL - bloqueada\n");
            // thread bloqueada ate que a queue nao esteja cheia
            pthread_cond_wait(&cond_internal_queue, &mutex_internal_queue);
        }
        else {
            if (!is_empty(&root)) {
                push(&root, received, 2);
            }
            else {
                root = create_new_node(received, 2);
                //push(&root, received, 2);
            }
        }

        pthread_mutex_unlock(&mutex_internal_queue);
    }

    pthread_exit(NULL);
    return NULL;
}

// thread sensor reader
void* sensor_reader() {
    if (getppid() == getpid()) {
        write_log("SENSOR_READER ALREADY CREATED");
        pthread_exit(NULL);
        return NULL;
    }
    int received_length;
    char received[BUFFER_SIZE];

    write_log("THREAD SENSOR_READER CREATED");

    while (!terminate_threads) {
        received_length = read(fd_sensor_pipe, received, sizeof(received));
        received[received_length - 1] = '\0';
        printf("SENSOR: %s\n", received);
        pthread_mutex_lock(&mutex_internal_queue);

        if (size(root) > config.queue_sz) {
            char text[BUFFER_SIZE];
            sprintf(text, "ORDER %s DELETED", received);  // TODO verificar frase
            write_log(text);
        }
        else {
            if (!is_empty(&root)) {
                push(&root, received, 1);
            }
            else {
                root = create_new_node(received, 1);
                //push(&root, received, 1);
            }
        }

        pthread_mutex_unlock(&mutex_internal_queue);
    }

    pthread_exit(NULL);
    return NULL;
}

char* peek(node** root) {
    return (*root)->msg;
}

// thread dispatcher
void* dispatcher() {
    if (getppid() == getpid()) {
        write_log("DISPATCHER ALREADY RUNNING");
        exit(1);
    }
    write_log("THREAD DISPATCHER CREATED");

    int* worker_sem_val = NULL;

    while (!terminate_threads) {
        pthread_mutex_lock(&mutex_internal_queue);

        while (!is_empty(&root)) {
            // print shared memory sensors
            // sem_wait(sem_shm);
            // for (int i = 0; i < config.max_keys; i++) {
            //     printf("\n\nSENSOR %d: %d\n\n", i, shared_memory->keys[i].last);
            // }
            // sem_post(sem_shm);
            // procurar um worker livre e escrever no pipe dele
            for (int i = 0; i < config.n_workers; i++) {
                /* if (sem_getvalue(&sems_worker[i], worker_sem_val) == -1) {
                    write_log("ERROR GETTING SEM VALUE");
                    terminate();
                } */

                if (shared_memory->workers_list[i] == 1) {
                    write_log("WORKER OCUPADO");
                }
                else {
                    // if (worker_sem_val == 1) {
                        // escrever no pipe do worker
                    if (write(unnamed_pipes[i][1], peek(&root), strlen(peek(&root)) + 1) == -1) {
                        write_log("ERROR WRITING TO WORKER PIPE");
                        terminate();
                    }
                    break;
                    // }
                }
            }
            // só para verificar
            printf("POP: %s\n", peek(&root));
            pop(&root);
            // Depois de reteriar uma mensagem da internal queue desbloqueia o console reader
            pthread_cond_signal(&cond_internal_queue);
        }

        pthread_mutex_unlock(&mutex_internal_queue);
    }
    printf("DISPATCHER FINISHED\n");
    pthread_exit(NULL);
    return NULL;
}

// funcao que cria as threads, os workers e o alerts watcher
void system_manager(char* config_file) {
    if (getppid() == getpid()) {
        write_log("SYSTEM MANAGER ALREADY RUNNING");
        exit(1);
    }

    init();

    write_log("HOME_IOT SIMULATOR STARTING");

    // ler config file
    read_config(config_file);

    write_log("CONFIG FILE READ");

    // alocar memoria para os unnamed pipes
    unnamed_pipes = malloc(config.n_workers * sizeof(int*));

    // alocar memoria para os unnamed semaphores dos workers
    // sems_worker = malloc(config.n_workers * sizeof(sem_t));

    shared_memory->workers_list = (int*) (((void*) shared_memory->keys) + sizeof(key_data) * config.max_keys);


    // inicializar workers
    for (int i = 0; i < config.n_workers; i++) {
        // inicializar a lista de workers
        // 0 -> livre; 1 -> ocupado 
        shared_memory->workers_list[i] = 0;

        if (pipe(unnamed_pipes[i]) == -1) {
            write_log("ERROR CREATING PIPE");
            exit(1);
        }
        //print do workers list
        int worker_forks;
        if ((worker_forks = fork()) == 0) {
            worker(i);
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

    // criar threads
    pthread_create(&console_reader_thread, NULL, console_reader, NULL);
    pthread_create(&sensor_reader_thread, NULL, sensor_reader, NULL);
    pthread_create(&dispatcher_thread, NULL, dispatcher, NULL);

    // terminar as threads
    pthread_join(console_reader_thread, NULL);
    pthread_join(sensor_reader_thread, NULL);
    pthread_join(dispatcher_thread, NULL);

    for (int i = 0; i < config.n_workers; i++) {
        wait(NULL);
    }

    free(shared_memory->workers_list);
    // wait(NULL);

}