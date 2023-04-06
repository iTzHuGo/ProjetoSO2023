#include "functions.h"

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    if (argc != 6) {
        fprintf(stderr, "sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {min} {max}\n");
        return 0;
    }

    if (strlen(argv[1]) < 3 || strlen(argv[1]) > 32) {
        write_log("SENSOR ID INVALID");
    }

    if (atoi(argv[2]) < 0) {
        write_log("SENSOR INTERVAL INVALID");
    }

    if (strlen(argv[3]) < 3 || strlen(argv[3]) > 32) {
        write_log("SENSOR KEY INVALID");
    }

    int f;
    
    if((f = fork()) == 0) {
        sensor(argv[1], argv[2], argv[3], argv[4], argv[5]);
        exit(0);
    } else if(f == -1){
        write_log("ERROR FORKING SENSOR");
    }

    // enviar esta informacao para o sys manager via named pipe

    return 0;
}