/*
    Miguel Filipe de Andrade Sérgio
    2020225643

    João Miguel Carmo Pino
    2020210945

*/
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <signal.h>


/* TODO
3. Add the way of processing a SIGSTOP
*/

bool check_id(char * id);
bool check_key(char * key);
int value_generator(int min, int max);
void sig_handler(int sig);
void clear_system();
time_t t;
int sent_packets = 0;
int fd;
/*
Main function arguments:
1. ID do sensor
2. Send time interval
3. Key
4. Minimum value
5. Maximum value
*/

int main(int argc, char *argv[]) {
  if (argc != 6) {
    printf("sensor {identificador do sensor} {intervalo entre envios em segundos (>=0)} {chave} {valor inteiro mínimo a ser enviado} {valor inteiro máximo a ser enviado}\n");
    return 1;
  }
  else if(!check_id(argv[1])){
    printf("Wrong input of ID\n");
    return 1;
  }
  else if(!check_key(argv[3])){
    printf("Wrong input of key\n");
    return 1;
  }

  signal(SIGSTOP, sig_handler);
  signal(SIGINT, sig_handler);

  char * id = argv[1];
  int time_interval = 1;
  char * key = argv[3];
  int minimum = atoi(argv[4]);
  int maximum = atoi(argv[5]);

  printf("ID: %s\nInterval: %d\nKey: %d\n %d - %d\n",argv[1],atoi(argv[2]),atoi(argv[3]),atoi(argv[4]),atoi(argv[5]));
  char msg[1024] = "";
  strcat(msg,id);
  strcat(msg,"#");
  strcat(msg,key);
  strcat(msg,"#");


  
  char buffer[1024];
  fd = open("SENSOR_PIPE", O_WRONLY);

  if (fd == -1) {
    perror("Error opening pipe");
    exit(1);
  }

  while (1) {
    char msg_aux[1024];
    
    sprintf(msg_aux, "%s%d",msg,value_generator(minimum,maximum));
    write(fd, msg_aux, 1024);
    sent_packets++;
    
    printf("Sent package: %s \n",msg_aux);
    sleep(time_interval);
  }
  close(fd);



/*
  //Sending packet
  while(1){
    char msg_aux[1024];
    strcpy(msg_aux,msg);
    sprintf(msg_aux, "%s%d",msg,value_generator(minimum,maximum));
    printf("Sent package: %s \n",msg_aux);
    sleep(time_interval);

  }
*/
  return 0;
}


bool check_id(char * id){
    
    int len = strlen(id);

    if (len < 3 || len > 32) {
        return false;
    }

   for(int c = 0; c < strlen(id); c++){
    //Checks for the grammar: [a-z][A-Z][0-9]
    if(!(('a'<=id[c]&&id[c]<= 'z') || 
        ( 'A' <= id[c] && id[c] <= 'Z') || 
        ( '0' <= id[c] && id[c] <= '9'))) {
        return false;
    }
   }

    return true;
}

bool check_key(char * key){
    int len = strlen(key);

    if (len < 3 || len > 32) {
        return false;
    }

   for(int c = 0; c < strlen(key); c++){
    //Checks for the grammar: [a-z][A-Z][0-9]_
    if(!(('a'<=key[c]&&key[c]<= 'z') || 
        ( 'A' <= key[c] && key[c] <= 'Z') || 
        ( '0' <= key[c] && key[c] <= '9') || 
        key[c] == '_')) {
        return false;
    }
   }

    return true;
}

int value_generator(int min, int max) {
    srand((unsigned) time(&t));
    return (rand() % (max - min + 1) + min);
}

void sig_handler(int sig)
{
    switch (sig) {
        case SIGSTOP:
            printf("Received SIGSTOP signal\n");
            printf("Number of packets sent: %d\n",sent_packets);
            break;
        case SIGINT:
            printf("Received SIGINT signal\n");
            clear_system();
            break;
        default:
            break;
    }
}

void clear_system(){
  printf("Cleaning everything...\n");
  close(fd);
  printf("Bye bye!!\n");
  exit(0);
}