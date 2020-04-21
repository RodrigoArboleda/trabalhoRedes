#include "menssage.h"

void *send_menssage(void *sock){

    int sock_server = *((int*)sock);

    int cod_error;
    char buffer[MENS_SIZE+1] = {0};

    while (1)
    {
        scanf("%4096s", buffer);
        cod_error = send(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            pthread_exit(NULL);
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
            pthread_exit(NULL);
        }
        printf("receive:%s\n", buffer);
    }

    pthread_exit(NULL);
}
