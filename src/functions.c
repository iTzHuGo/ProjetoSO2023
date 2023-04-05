// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

void start() {
    log_file = fopen("log.txt", "w+");

    // inicializacao do semaforo para o fich log
    sem_unlink("SEM_LOG");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL , 0700, 1);
#if DEBUG
    if(sem_log == SEM_FAILED){
        write_log("Sem_open() failed. errno: %d\n", errno);
    } else {
        write_log("Semaphore to log created")
    }
#endif
}

void terminate() {
    write_log("Close");

    sem_close(sem_log);
    sem_unlink("SEM_LOG");

    fclose("log.txt");

    exit 0;
}

void write_log(char* msg) {
    char* text = NULL, current_time = NULL;
    time_t t;
    time(&t);
    FILE *log;

    current_time = (char *)malloc(strlen(ctime(&t)) * sizeof(char)+1);
    strcpy(current_time, ctime(&t));
    
    text = (char *)malloc((strlen(current_time) + strlen(msg)) * sizeof(char)+1);
    sprintf(text, "%s %s", current_time, msg);

    sem_wait(log_mutex);
    fprintf(log, "%s", text);
    fflush(log);
    sem_post(log_mutex);

    printf("%s", text);

    free(current_time);
    free(text);
}

void user_console() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char *)malloc((strlen("User Console successfully created with the pid.") + sizeof(pid)) * sizeof(char)+1);
    sprintf(text, "User Console successfully created with the pid %d.", pid);
    write_log(text);
    free(text);
#endif
    
    menu();

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
            else if (strcmp(instruction[0], "add_alert") == 0) {
                if (atoi(instruction[1]) < 3 || atoi(instruction[1]) > 32) {
                    printf("id invalido\n");
                    continue;
                }
                if (!isdigit(instruction[2]) || !isdigit(instruction[3])){
                    printf("min ou max invalido\n");
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
                    printf("id invalido\n");
                    continue;
                }

                printf("Remove_alert\n");
                printf("id: %s\n", instruction[1]);
            }
            else if (strcmp(instruction[0], "list_alerts") == 0) {
                printf("List_alerts\n");
            }
            else {
                printf("Invalid parameter\n");
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
    text = (char *)malloc((strlen("Sensor successfully created with the pid.") + sizeof(pid)) * sizeof(char)+1);
    sprintf(text, "Sensor successfully created with the pid %d.", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void worker() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char *)malloc((strlen("Worker successfully created with the pid.") + sizeof(pid)) * sizeof(char)+1);
    sprintf(text, "Worker successfully created with the pid %d.", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void alerts_watcher() {
#if DEBUG
    char* text = NULL;
    int pid = getpid();
    text = (char *)malloc((strlen("Alerts Watcher successfully created with the pid.") + sizeof(pid)) * sizeof(char)+1);
    sprintf(text, "Alerts Watcher successfully created with the pid %d.", pid);
    write_log(text);
    free(text);
#endif
    exit(0);
}

void system_manager() {
    // inicializar worker
    if ((fork()) == 0) {
        worker();
        exit(0);
    }

    // inicializar alerts_watcher
    if ((fork()) == 0) {
        alerts_watcher();
        exit(0);
    }

    // criacao de threads
}