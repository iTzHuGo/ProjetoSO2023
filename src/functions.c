//Ana Rita Martins Oliveira 2020213684
//Hugo Sobral de Barros 2020234332

#include "functions.h"

void worker() {
#if DEBUG
    char* text;
    sprintf(text, "Worker successfully created with the pid %d.", getpid());
    write_log(text);
#endif
    exit(0);
}

void alerts_watcher() {
#if DEBUG
    char* text;
    sprintf(text, "Alerts Watcher successfully created with the pid %d.", getpid());
    write_log(text);
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

    //criacao de threads
}