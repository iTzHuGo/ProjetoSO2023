#include "functions.h"

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "home_iot {ficheiro de configuração}\n");
        return 0;
    }
    int f;

    if ((f = fork()) == 0) {
        system_manager(argv[1]);
        exit(0);
    }
    else if (f == -1) {
        write_log("ERROR CREATING SYSTEM MANAGER");
    }

    return 0;
}
