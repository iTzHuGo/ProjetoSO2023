#include "functions.h"

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    // int user_console_id = atoi(argv[1]);	// identificador da consola

    int f;

    if ((f = fork()) == 0) {
        user_console();
        exit(0);
    } else if(f == -1){
        write_log("ERROR FORKING USER CONSOLE");
    }

    // enviar esta informacao para o sys manager via named pipe

    return 0;
}