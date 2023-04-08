// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332
#include "functions.h"

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    // Verifica se o numero de argumentos e valido
    if (argc != 6) {
        fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}\n");
        return 0;
    }

    // Verifica se os argumentos sao validos
    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32) {
        printf("SENSOR ID INVALID");
    }

    if (atoi(argv[2]) < 0) {
        printf("SENSOR INTERVAL INVALID");
    }

    if (strlen(argv[3]) < 3 || strlen(argv[3]) > 32) {
        printf("SENSOR KEY INVALID");
    }

    // Inicializa o processo do sensor
    int f;
    if ((f = fork()) == 0) {
        sensor(argv[1], argv[2], argv[3], argv[4], argv[5]);
        exit(0);
    }
    else if (f == -1) {
        printf("ERROR FORKING SENSOR");
    }

    // Espera que o sensor termine
    wait(NULL);

    return 0;
}