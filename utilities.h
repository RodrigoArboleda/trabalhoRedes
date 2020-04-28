#ifndef UTILITIES
#define UTILITIES

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>


#define ERROR "ERRO INTERNO."
#define ERROR_SOCK "FALHA AO CRIAR SOCKET."
#define ERROR_CONNECT "ERRO AO SE CONECTAR AO SERVIDOR."
#define ERROR_BIND "ERRO AO ASSOCIAR O ENDERECO"

#define ERROR_COD -1
#define ERROR_SOCK_COD 1
#define ERROR_CONNECT_COD 2
#define ERROR_BIND_COD 3

#define PORT_SERVER 1515
#define MAX_CLIENT 1
#define MENS_SIZE 4096

typedef struct sockaddr_in sockaddr_ip;

void error(char* mens);

#endif