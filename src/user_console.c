#include <stdio.h>

#define BUFFER_SIZE 512

int main(int argc, char* argv[])
{
    if (argc != 2) {
        fprintf(stderr, "user_console {identificador da consola}\n");
        return 0;
    }

    int user_console_id = atoi(argv[1]);	// identificador da consola

    if(fork() == 0) {
        user_console();
        exit(0);
    }

    // enviar esta informacao para o sys manager via named pipe

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