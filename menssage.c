#include "menssage.h"

void *send_menssage(void *sock){

    int sock_server = *((int*)sock);

    int cod_error;
    char buffer[MENS_SIZE+1] = {0};

    while (1)
    {
        scanf("\r\n%4096[^\n]", buffer);
        cod_error = send(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            exit(ERROR_CONNECT_COD);
        }
    }

    pthread_exit(NULL);
}

void *receive_menssage(void *sock){

    int sock_server = *((int*)sock);

    int cod_error;
    char buffer[MENS_SIZE] = {0};

    while (1)
    {
        cod_error = recv(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            exit(ERROR_CONNECT_COD);
        }
        printf("receive:%s\n", buffer);
    }

    pthread_exit(NULL);
}
