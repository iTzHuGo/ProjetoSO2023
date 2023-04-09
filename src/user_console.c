// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

#define BUFFER_SIZE 512

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    // int user_console_id = atoi(argv[1]);	// identificador da consola

    // Inicializa o processo do user console
    int f;
    if ((f = fork()) == 0) {
        user_console();
        exit(0);
    } else if (f == -1) {
        write_log("ERROR FORKING USER CONSOLE");
    }

    // Espera que o user console termine
    wait(NULL);

    return 0;
}