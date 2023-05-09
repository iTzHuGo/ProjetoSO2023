/*
    Miguel Filipe de Andrade Sérgio
    2020225643

    João Miguel Carmo Pino
    2020210945

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define CONSOLE_PIPE "CONSOLE_PIPE"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("console {id}\n");
        return 1;
    }

    int fd;
    char buffer[1024];

    // Open the named pipe with write-only access
    fd = open(CONSOLE_PIPE, O_WRONLY);

    if (fd == -1) {
        perror("Error opening pipe");
        exit(1);
    }

    while (1) {
        // Read input from the user
        printf("Enter a message to send: ");
        fgets(buffer, 1024, stdin);

        // Write the message to the pipe
        write(fd, buffer, strlen(buffer) + 1);
    }

    return 0;
}
