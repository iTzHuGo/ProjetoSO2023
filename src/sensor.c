// Ana Rita Martins Oliveira 2020213684
// Hugo Sobral de Barros 2020234332

#include "functions.h"

#define BUFFER_SIZE 512

int main(int argc, char* argv[]) {
    // Verifica se o numero de argumentos e valido
    if (argc != 6) {
        fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}\n");
        return 0;
    }

    // Verifica se os argumentos sao validos
    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32) {
        printf("SENSOR ID INVALID");
        exit(1);
    }

    if (!is_digit(argv[2]) || !is_digit(argv[4]) || !is_digit(argv[5])) {
        printf("PARAMETER INVALID");
        exit(1);
    }

    if (atoi(argv[2]) < 0) {
        printf("SENSOR INTERVAL INVALID");
        exit(1);
    }

    if (strlen(argv[3]) < 3 || strlen(argv[3]) > 32) {
        printf("SENSOR KEY INVALID");
        exit(1);
    }

    if (atoi(argv[4]) > atoi(argv[5])) {
        printf("SENSOR MIN/MAX INVALID");
        exit(1);
    }



    sensor(argv[1], atoi(argv[2]), argv[3], atoi(argv[4]), atoi(argv[5]));

    return 0;
}