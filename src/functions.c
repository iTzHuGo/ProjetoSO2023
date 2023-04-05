//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

#include "functions.h"

void write_log(char* msg) {
    char* text = NULL, current_time = NULL;
    time_t t;
    time(&t);
    FILE *log;

    current_time = (char *)malloc(strlen(ctime(&t)) * sizeof(char)+1);
    strcpy(current_time, ctime(&t));
    
    text = (char *)malloc((strlen(current_time) + strlen(msg)) * sizeof(char)+1);
    sprintf(text, "%s %s", current_time, msg);

    log = fopen("log.txt", "a");

    sem_wait(log_mutex);
    fprintf(log, "%s", text);
    fclose(log);
    fflush(log);
    fflush(stdout);
    sem_post(log_mutex);
    printf("%s", text);

    free(current_time);
    free(text);
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


    // inicializacao do semaforo log
    void init_sem() {
    sem_unlink("SEM_LOG");
    sem_log = sem_open("SEM_LOG", O_CREAT | O_EXCL , 0700, 1);
    if(sem_log == SEM_FAILED){
        write_log("Erro %d ao inicializar o semáforo\n", errno);
    }
}
}