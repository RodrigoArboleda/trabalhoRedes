#include "menssage.h"


/*Esta função manda para o socket passado como parametros
todas as mensagens escritas na entrada padrão
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - int - codigo de erro
*/
void *send_menssage(void *param){

    int sock_server = ((int*)param)[0];
    int* connect_status = (int*)param+1;

    int ret_thread = 0;

    int cod_error;
    char buffer[MENS_SIZE+1] = {0};

    while (*connect_status)
    {
        scanf(" %4096[^\n]", buffer);
        cod_error = send(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            ret_thread = ERROR_CONNECT_COD;
            *connect_status = 0;
            printf("Precione ENTER para encerrar.\n");
            pthread_exit(&ret_thread);
        }
    }

    *connect_status = 0;
    pthread_exit(&ret_thread);
}

/*Esta função escuta indefinidamente a um socket e escreve na saida padrao
o que ouvir do socket
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
void *receive_menssage(void *param){

    int sock_server = ((int*)param)[0];
    int* connect_status = (int*)param+1;

    int ret_thread = 0;

    int cod_error;
    char buffer[MENS_SIZE] = {0};

    while (*connect_status)
    {
        cod_error = recv(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            ret_thread = ERROR_CONNECT_COD;
            *connect_status = 0;
            printf("Digite qualquer coisa para encerrar.\n");
            pthread_exit(&ret_thread);
        }
        if(strlen(buffer) != 0){
            printf("receive: %s\n", buffer);
        }
    }

    *connect_status = 0;
    pthread_exit(&ret_thread);
}
