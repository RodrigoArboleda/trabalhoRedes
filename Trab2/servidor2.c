#include "servidor.h"

#include <time.h>
#include <pthread.h>
#include <stdio.h>
/* for ETIMEDOUT */
#include <errno.h>
#include <string.h>
#include<semaphore.h>

typedef struct client{

    int socket; //-1
    sockaddr_in addr;
    unsigned int size_addr;
    char nickname[50];
    pthread_mutex_t mutex;// = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond;// = PTHREAD_COND_INITIALIZER;
    
    sem_t sem_connect_status;
    int connect_status; //=1;

    sem_t sem_read;
    char buffer_read[4097]; //buffer utilizado para receber dados ao cliente

    pthread_t thread_listen;

    sem_t sem_sending_msg;
    //contador de quantas mensagens estao sendo enviadas a este cliente
    int cont_sending_msg; 
} CLIENT;

CLIENT *create_client(int sock,sockaddr_in addr,unsigned int size_addr, char*nickname){
    CLIENT* cli = (CLIENT*)malloc(sizeof(CLIENT));

    cli->socket = socket;
    cli->addr = addr;
    cli->size_addr = size_addr;
    strcpy(cli->nickname,nickname);

    cli->connect_status = 1;parameter_threadparameter_thread
    cli->mutex = PTHREAD_MUTEX_INITIALIZER;
    cli->cond = PTHREAD_COND_INITIALIZER;
    cli->cont_sending_msg = 0;

    sem_init(&(cli->sem_connect_status),0,1);
    sem_init(&(cli->sem_read),0,1);
    sem_init(&(cli->sem_sending_msg),0,1);
    pthread_create(&(cli->thread_listen), NULL, receive_message, (void*)(cli));

    return cli;
}

//vai ter que ser uma tread, mudar o retorno e parametro
void *disconnect_cli(void *cli_){
    CLIENT* cli = (CLIENT*)cli_;
    /*remover da lista de clientes, primeira coisa a se fazer, 
    /pois isto impossibilita de mandarem novas mensagens a este cliente, evitando seg fault e deadlock
    */
    sem_wait( &(cli->sem_connect_status) );
    cli->connect_status = 0;
    sem_post( &(cli->sem_connect_status) );

    sem_wait( &(cli->sem_read) );
    sem_post( &(cli->sem_read) );

    //esperar ateh nao ter nenhuma thread mandando msg a este cliente.
    while(1){

        sem_wait( &(cli->sem_sending_msg));
        if(cli->cont_sending_msg == 0){
            break;
        }
        sem_post( &(cli->sem_sending_msg));
    }

    sem_destroy(&(cli->sem_connect_status));
    sem_destroy(&(cli->sem_read));
    sem_destroy(&(cli->sem_sending_msg));

    void *thread_ret;
    pthread_join (&(cli->thread_listen), &thread_ret);

    shutdown(cli->socket, 1);
    //not complete, falta muita coisa nessa funcao, falta:
    //remover da lista de clientes
    //...


    free(cli);
}


int num_cliente = 0;
sem_t sem_num_cliente;

/*
Esta funcção prepara a porta padrao para se conectar aos clientes, o MAX_CLIENT e a PORTA e ,
retorna 
@RETORNO
    int - socket conectado ao cliente
*/
int connect_server(){

    int sock_task = -1;
    int cod_error = 0;

    sock_task = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_task < 0){
        error(ERROR_SOCK);
        exit(ERROR_SOCK_COD);
    }

    sockaddr_in addrserver;
    /*montando a struct do endereço do socket*/
    addrserver.sin_family = AF_INET;/*utilizando TCP*/
    addrserver.sin_addr.s_addr = INADDR_ANY;
    addrserver.sin_port = invert_endian_16B(PORT_SERVER);

    sockaddr_in addrclient;
    /*associando o socket ao seu respectivo endereço*/
    cod_error = bind(sock_task, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_error != 0)
    {
        error(ERROR_BIND);
        exit(ERROR_BIND_COD);
    }
    /*Quantos clientes podem se conectarem a este socket*/
    cod_error = listen(sock_task, MAX_CLIENT);
    if (cod_error != 0)
    {
        error(ERROR);
        exit(ERROR_COD);
    }

    return sock_task;

}

void* connecta_cliente(void *sock_task_){

    int*sock_task_end = (int*)sock_task_;
    int sock_task = *sock_task_end;

    while(1){

        int sock_client;
        unsigned int size_client_addr;
        /*Esperando conecção de um cliente*/
        sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
        if(sock_client < 0){
            printf("Servidor Cheio, impossivel adicionar mais clientes\n");
            continue;
        }
        //create client


        sem_wait(&sem_num_cliente);
        num_cliente++;

        //adicionar client a lista de clientes.
        sem_post(&sem_num_cliente);


    }
}


void init(){
    sem_init(&sem_num_cliente,0,1);
}

int exec_n_segundos(int n, void *(*funcao) (void *),void *param, pthread_mutex_t *mutex_end, pthread_cond_t *cond_end){
    struct timespec abs_time;
    pthread_t tid;

    pthread_mutex_lock(mutex_end);

    /* pthread cond_timedwait expects an absolute time to wait until */
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += n;

    int erro;

    pthread_create(&tid, (void*)erro, funcao, param);

    /* pthread_cond_timedwait can return spuriously: this should
    * be in a loop for production code
    */
    int err = pthread_cond_timedwait(cond_end, mutex_end, &abs_time);

    pthread_mutex_unlock(mutex_end);

    return err;
}

void *receive_message_aux(void *param){

    CLIENT*client = ((CLIENTE*)param);
    int ret_thread = 0;

    cod_error = recv(sock_server, client->buffer_read, MENS_SIZE, 0);
    if (cod_error <= 0)
    {
        error(ERROR_CONNECT);
        ret_thread = ERROR_CONNECT_COD;
        printf(MSG_END_CONNECT);
        pthread_exit(ret_thread);
    }
    pthread_cond_signal(&(client->cond)));
    pthread_exit(ret_thread);
}

/*Esta função escuta indefinidamente a um socket e escreve na saida padrao
o que ouvir do socket
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
void *receive_message(void *param){

    CLIENT*client = ((CLIENTE*)param);
    int ret_thread = 0;

    int cod_error;
    char buffer[MENS_SIZE] = {0};

    while(1)
    {   
        sem_wait( &(cli->sem_connect_status) );
        if(!(cli->connect_status)){
            sem_post( &(cli->sem_connect_status) );
            break;
        }
        sem_post( &(cli->sem_connect_status) );

        sem_wait( &(client->sem_read) );
        int err = exec_n_segundos(1,receive_message_aux,param,&(cliente->mutex),&(client->cond));

        char buffer[4097];
        strcpy(buffer,client->buffer_read);

        sem_post( &(client->sem_read) );

        if( strlen(buffer) > 0 ){
            //enviar aos outros clientes
        }
    }


    pthread_exit(&ret_thread);
}

/*Esta função manda para o socket passado como parametros
todas as mensagens escritas na entrada padrão
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - int - codigo de erro
*/
void *send_menssage(void *param){

    //recebimento de parametros nao esta completo

    char* client_from_nickname; //= ((char**)param)[0];
    char* client_from_buffer_read; //= ((char**)param)[0];
    CLIENT*client; //= ((CLIENTE**)param)[1];

    //incrementando o contador sending_msg
    sem_wait( &(client->sem_sending_msg));
    client->sending_msg = client->cont_sending_msg+1;
    sem_post( &(client->sem_sending_msg));

    int ret_thread = 0;
    /*nao se deve enviar a mensagem para a pessoal que a enviou*/
    if(strcmp( client_from_nickname,client->nickname ) == 0){
        pthread_exit(ret_thread);
    }
    /*copiando para buffer a mensagem que devemos enviar*/

    char buffer[4097];
    strcpy(buffer_aux,client_from_nickname);
    strcat(buffer_aux,": ");
    strcat(buffer, client_from_buffer_read);
    strcat(buffer, "\0");
    //a concatenacao com o nickname pode passar os 4096 bytes, por isto a linha abaixo
    buffer[4096] = "\0";
    
    int i;
    int ack = 0;
    for(i = 0; i < 5; i++){
        sem_wait( &(client->sem_read));
        int cod_error;
        cod_error = send(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {

            sem_post( &(client->sem_read) );

            sem_wait( &(client->sem_sending_msg));
            client->sending_msg = client->cont_sending_msg-1;
            sem_post( &(client->sem_sending_msg));

            error(ERROR_CONNECT);
            ret_thread = ERROR_CONNECT_COD;

            printf(MSG_END_CONNECT);
            //desconectar cliente tbm aqui
            pthread_exit(&ret_thread);)

            
        }
    
        int err = exec_n_segundos(2,receive_message_aux,param,&(client->mutex),&(client->cond));

        if( err == 0  && strcmp(client->buffer_read,"<ACK>") == 0){
            ack = 1;
            break;
        }
        sem_post( &(client->sem_read) );
    }



    if(ack = 0){

        //mate a conexao;

        sem_wait( &(client->sem_sending_msg));
        client->sending_msg = client->cont_sending_msg-1;
        sem_post( &(client->sem_sending_msg));

        pthread_exit(ret_thread);


        return NULL;
    }

    sem_wait( &(client->sem_sending_msg));
    client->sending_msg = client->cont_sending_msg-1;
    sem_post( &(client->sem_sending_msg));

    pthread_exit(ret_thread);

    
}