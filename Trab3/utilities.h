#ifndef UTILITIES
#define UTILITIES

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <time.h>


#define ERROR "ERRO INTERNO."
#define ERROR_SOCK "FALHA AO CRIAR SOCKET."
#define ERROR_CONNECT "ERRO AO SE CONECTAR AO SERVIDOR."
#define ERROR_BIND "ERRO AO ASSOCIAR O ENDERECO"
#define MSG_CONNECT "Conectado! Pode Começar a mandar mensagens!!\n"
#define MSG_END_CONNECT "Digite algo para encerrar o programa.\n"

#define ERROR_COD -1
#define ERROR_SOCK_COD 1
#define ERROR_CONNECT_COD 2
#define ERROR_BIND_COD 3

#define PORT_SERVER 1523
#define MAX_CLIENT 1
#define MENS_SIZE 4096

void* ff(void *param);

typedef struct sockaddr_in sockaddr_ip;

/*
Esta função printa uma mensagem de erro passada como parametro
@PARAMETROS
    - char* mens - mensagem a ser imprimida na saida padrao
@RETORNO
    - void - sem retorno
*/
void error(char* mens);

/*
Esta função recebe um endereço IP no formato string e o convert para o seu formato em 32bits
@PARAMETROS
    - char *ip_string - endereço IP no formato string
@RETORNO
    - int - endereço IP no formato 32bits
*/
int string_to_byte_ip_adress(char *ip_string);

/*
Esta função recebe um endereço IP no formato 32bits e o converte para o seu formato string
@PARAMETROS
    - char *ip_string - onde sera guardado o endereço IP no formato string
    - int - endereço IP no formato 32bits
@RETORNO
*/
void byte_to_string_ip_adress(char*ip_string,int ip);

/*
Esta função inverte um valor 16bits de little endian para big endian e vice-versa
@PARAMETROS
    - uint16_t x - valor 16 bits de entrada.
@RETORNO
    - uint16_t - valor com a endian mudada.
*/
uint16_t invert_endian_16B(uint16_t x);


typedef struct client{

    int socket; /*-1*/
    struct sockaddr_in addr;
    unsigned int size_addr;

    sem_t sem_nickname;
    char nickname[50];

    /*utilizado para thread com timeout*/
    pthread_mutex_t mutex;/* = PTHREAD_MUTEX_INITIALIZER;*/
    pthread_cond_t cond;/* = PTHREAD_COND_INITIALIZER;*/
    
    sem_t sem_connect_status;
    int connect_status; /*=1;*/

    sem_t sem_read;

    pthread_t thread_listen;

    sem_t sem_sending_msg;
    /*contador de quantas mensagens estao sendo enviadas a este cliente*/
    int cont_sending_msg;

    /*eh um CHANNEL*channel definido em channel.c
    indica qual o canal que o cliente esta no momento*/
    void *channel;
    /*se o cliente esta mutado*/
    sem_t sem_muted;
    int muted; 

} CLIENT;

int exec_n_segundos(int n, void *(*funcao) (void *),void *param, pthread_mutex_t *mutex_end, pthread_cond_t *cond_end);

#endif
