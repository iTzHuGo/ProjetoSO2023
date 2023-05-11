// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    // int user_console_id = atoi(argv[1]);	// identificador da consola

    // Inicializa o processo do user console
    user_console();

    return 0;
}