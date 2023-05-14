// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

#define BUFFER_SIZE 1024

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    if (!is_digit(argv[1])) {
        printf("CONSOLE IDENTIFIER INVALID\n");
        exit(1);
    }

    if (atoi(argv[1]) < 0) {
        printf("CONSOLE IDENTIFIER INVALID\n");
        exit(1);
    }

    int user_console_id = atoi(argv[1]);  // identificador da consola

    // Inicializa o processo do user console
    user_console(user_console_id);

    return 0;
}