#include "cliente.h"

int main(int argc, char *argv[]){

    char ip[15] = {0};

    int cod_erro = 0;

    printf("Digite o IP do servidor:\n");
    scanf("%s", ip);

    int sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0){
        erro(ERRO_SOCK, ERRO_SOCK_COD);
    }

    sockaddr_ip addrserver;

    addrserver.sin_family = AF_INET;
    addrserver.sin_addr.s_addr = inet_addr(ip);
    addrserver.sin_port = htons(PORT_SERVER);


    cod_erro = connect(sock_server, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_erro != 0)
    {
        erro(ERRO_CONNECT, ERRO_CONNECT_COD);
    }

    char buffer[MENS_SIZE] = {0};
    buffer[0] = 'O';
    buffer[1] = 'I';
    buffer[2] = '!';
    buffer[3] = '\0';
    
    send(sock_server, buffer, MENS_SIZE, 0);


    return 0;
}