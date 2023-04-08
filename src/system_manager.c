// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

int main(int argc, char* argv[])
{
    // Verifica se o número de argumentos é válido
    if (argc != 2) {
        fprintf(stderr, "home_iot {ficheiro de configuração}\n");
        return 0;
    }

    // Inicializa o programa
    init();

    // Cria o processo do system manager
    int f;
    if ((f = fork()) == 0) {
        system_manager(argv[1]);
        exit(0);
    } else if (f == -1) { // Erro ao criar o processo
        write_log("ERROR CREATING SYSTEM MANAGER");
    }

    // Espera que o system manager termine
    wait(NULL);

    // Termina o programa
    terminate();

    return 0;
}
