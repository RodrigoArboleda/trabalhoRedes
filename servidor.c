#include "servidor.h"

int main(int argc, char *argv[]){

    int cod_erro = 0;

    printf("Aguardando cliente...\n");

    int sock_task = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_task < 0){
        erro(ERRO_SOCK, ERRO_SOCK_COD);
    }

    sockaddr_ip addrserver;

    addrserver.sin_family = AF_INET;
    addrserver.sin_addr.s_addr = INADDR_ANY;
    addrserver.sin_port = htons(PORT_SERVER);

    sockaddr_ip addrclient;

    cod_erro = bind(sock_task, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_erro != 0)
    {
        erro(ERRO_BIND, ERRO_BIND_COD);
    }
    
    cod_erro = listen(sock_task, MAX_CLIENT);
    if (cod_erro != 0)
    {
        erro(ERRO, ERRO_COD);
    }

    int sock_client;
    unsigned int size_client_addr;

    sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
    if(sock_client < 0){
        erro(ERRO_CONNECT, ERRO_CONNECT_COD);
    }
    
    char buffer[MENS_SIZE] = {0};

    recv(sock_client, buffer, MENS_SIZE, 0);

    printf("%s\n", buffer);

    return 0;
}