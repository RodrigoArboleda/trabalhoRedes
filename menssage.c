#include "menssage.h"

/*Esta função manda para o socket passado como parametros
todas as mensagens escritas na entrada padrão
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
void *send_menssage(void *sock){

    int sock_server = *((int*)sock);

    int cod_error;
    char buffer[MENS_SIZE+1] = {0};

    while (1)
    {
        scanf(" %4096[^\n]", buffer);
        cod_error = send(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            exit(ERROR_CONNECT_COD);
        }
    }

    pthread_exit(NULL);
}

/*Esta função escuta indefinidamente a um socket e escreve na saida padrao
o que ouvir do socket
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
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
        if(strlen(buffer) != 0){
            printf("receive: %s\n", buffer);
        }
    }

    pthread_exit(NULL);
}
