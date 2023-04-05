#include <stdio.h>

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    int user_console_id = atoi(argv[1]);	// identificador da consola

    if (fork() == 0) {
        menu();

        char line[BUFFER_SIZE], instruction[5][BUFFER_SIZE];
        char* token;

        while (fgets(line, BUFFER_SIZE, stdin) != NULL) {
            if (strlen(line) != 1) {
                int pos = 0;
                token = strtok(line, " \n");
                while (token != NULL) {
                    strcpy(instruction[pos++], token);
                    token = strtok(NULL, " \n");
                }

                if (strcmp(instruction[0], "exit") == 0) {
                    break;
                }
                else if (strcmp(instruction[0], "stats") == 0) {
                    printf("Stats\n");
                }
                else if (strcmp(instruction[0], "reset") == 0) {
                    printf("Reset\n");
                }
                else if (strcmp(instruction[0], "sensors") == 0) {
                    printf("Sensors\n");
                }
                else if (strcmp(instruction[0], "add_alert") == 0) {
                    if (atoi(instruction[1]) < 3 || atoi(instruction[1]) > 32) {
                        printf("id invalido\n");
                        continue;
                    }
                    if (!isdigit(instruction[2]) || !isdigit(instruction[3])){
                        printf("min ou max invalido\n");
                        continue;
                    }

                    printf("Add_alert\n");
                    printf("id: %s\n", instruction[1]);
                    printf("chave: %s\n", instruction[2]);
                    printf("min: %s\n", instruction[3]);
                    printf("max: %s\n", instruction[4]);
                }
                else if (strcmp(instruction[0], "remove_alert") == 0) {
                    if (atoi(instruction[1]) < 3 || atoi(instruction[1]) > 32) {
                        printf("id invalido\n");
                        continue;
                    }

                    printf("Remove_alert\n");
                    printf("id: %s\n", instruction[1]);
                }
                else if (strcmp(instruction[0], "list_alerts") == 0) {
                    printf("List_alerts\n");
                }
                else {
                    printf("Invalid parameter\n");
                }
            }
        }
    }

    return 0;
}

void menu() {
    printf("exit\nSair do User Console\n\n",
        "stats\nApresenta estatísticas referentes aos dados enviados pelos sensores\n\n",
        "reset\nLimpa todas as estatísticas calculadas até ao momento pelo sistema\n\n",
        "sensors\nLista todos os Sensorsque enviaram dados ao sistema\n\n",
        "add_alert [id] [chave] [min] [max]\nAdiciona uma nova regra de alerta ao sistema\n\n",
        "remove_alert [id]\nRemove uma regra de alerta do sistema\n\n",
        "list_alerts\nLista todas as regras de alerta que existem no sistema\n\n");
}