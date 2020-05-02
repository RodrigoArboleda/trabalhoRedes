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

#define PORT_SERVER 1515
#define MAX_CLIENT 1
#define MENS_SIZE 4096

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
Esta função inverte um valor 16bits de little endian para big endian e vice-versa
@PARAMETROS
    - uint16_t x - valor 16 bits de entrada.
@RETORNO
    - uint16_t - valor com a endian mudada.
*/
uint16_t invert_endian_16B(uint16_t x);

#endif