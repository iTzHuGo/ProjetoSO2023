// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

int main(int argc, char* argv[]) {
    // Verifica se o número de argumentos é válido
    if (argc != 2) {
        fprintf(stderr, "home_iot {ficheiro de configuração}\n");
        return 0;
    }

    pid_system_manager = getpid();

    system_manager(argv[1]);

    return 0;
}
