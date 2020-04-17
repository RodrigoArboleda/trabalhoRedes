#ifndef DISNEY
#define DISNEY

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define ERRO "ERRO INTERNO."
#define ERRO_SOCK "FALHA AO CRIAR SOCKET."
#define ERRO_CONNECT "ERRO AO SE CONECTAR AO SERVIDOR."
#define ERRO_BIND "ERRO AO ASSOCIAR O ENDERECO"

#define ERRO_COD -1
#define ERRO_SOCK_COD 1
#define ERRO_CONNECT_COD 2
#define ERRO_BIND_COD 3

#define PORT_SERVER 1515
#define MAX_CLIENT 1
#define MENS_SIZE 4096

typedef struct sockaddr_in sockaddr_ip;

int erro(char* mens, int ret);

#endif