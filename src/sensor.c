#include <stdio.h>

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    if (argc != 6) {
        fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}\n");
        return 0;
    }

    if(fork() == 0) {
        sensor(argv[1], argv[2], argv[3], argv[4], argv[5]);
        exit(0);
    }

    // enviar esta informacao para o sys manager via named pipe

    return 0;
}