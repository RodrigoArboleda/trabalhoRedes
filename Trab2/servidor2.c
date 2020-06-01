#include "servidor2.h"
#include "list.h"

sem_t sem_lista_clientes;
LIST *lista_clientes;
int num_cliente = 0;



typedef struct SEND_MESSAGE_STRUCT_{
    char client_from_nickname[40];
    char client_from_message[40];
    CLIENT*client;
}SEND_MESSAGE_STRUCT;

typedef struct RECEIVE_MESSAGE_STRUCT_{
    char *message;
    CLIENT*client;
    int x;
}RECEIVE_MESSAGE_STRUCT;


int wait(int x){

    struct timespec time_sleep;

    time_sleep.tv_nsec = x;
    time_sleep.tv_sec = 0;

    for (int i = 0; i < 10; i++)
    {


        nanosleep(&time_sleep, NULL);
    }
    
    return 1;

}

int prepare_server_socket(){

    int sock_task = -1;
    int cod_error = 0;

    sock_task = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_task < 0){
        error(ERROR_SOCK);
        exit(ERROR_SOCK_COD);
    }

    struct sockaddr_in addrserver;
    /*montando a struct do endereço do socket*/
    addrserver.sin_family = AF_INET;/*utilizando TCP*/
    addrserver.sin_addr.s_addr = INADDR_ANY;
    addrserver.sin_port = invert_endian_16B(PORT_SERVER);

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


void *disconnect_cli(void *cli_){

    CLIENT* cli = (CLIENT*)cli_;

    /*remover da lista de clientes, primeira coisa a se fazer, 
    /pois isto impossibilita de mandarem novas mensagens a este cliente, evitando seg fault e deadlock
    */
    sem_wait( &(sem_lista_clientes) );
    remove_list(lista_clientes, cli->nickname);
    /*Abaixando o número de clientes conectados*/
    num_cliente--;
    sem_post( &(sem_lista_clientes) );

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

        wait(50000);
    }

    void *thread_ret;
    pthread_join (cli->thread_listen, &thread_ret);

    sem_destroy(&(cli->sem_connect_status));
    sem_destroy(&(cli->sem_read));
    sem_destroy(&(cli->sem_sending_msg));

    shutdown(cli->socket, 1);

    wait(500000);

    free(cli);

    pthread_exit(NULL);

}


void *receive_message_aux(void *param){

    int oldtype;

    /* allow the thread to be killed at any time */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);


    RECEIVE_MESSAGE_STRUCT recv_msg = *( (RECEIVE_MESSAGE_STRUCT*)param);
    CLIENT*client = recv_msg.client;
    int ret_thread = 0;

    int cod_error;

    while(strlen(recv_msg.message) == 0)
        cod_error = recv(client->socket, recv_msg.message, MENS_SIZE,0);

    
    if (cod_error <= 0)
    {
        error(ERROR_CONNECT);
        ret_thread = ERROR_CONNECT_COD;
        printf(MSG_END_CONNECT);

    }
    pthread_cond_signal(&(client->cond));
    pthread_exit((void*)&ret_thread);
}

void *send_menssage_thread(void *param){

    //recebimento de parametros nao esta completo
    SEND_MESSAGE_STRUCT send_msg = *( (SEND_MESSAGE_STRUCT*)param);
    
    CLIENT*client = send_msg.client;

    //incrementando o contador sending_msg
    sem_wait( &(client->sem_sending_msg));
    client->cont_sending_msg = client->cont_sending_msg+1;
    sem_post( &(client->sem_sending_msg));

    int ret_thread = 0;
    /*copiando para buffer a mensagem que devemos enviar*/
    
    char buffer[4097];
    strcpy(buffer,send_msg.client_from_nickname);
    strcat(buffer,": ");
    strcat(buffer, send_msg.client_from_message);
    strcat(buffer, "\0");
    //a concatenacao com o nickname pode passar os 4096 bytes, por isto a linha abaixo
    buffer[4096] = '\0';
    

    int i;
    int ack = 0;

    sem_wait( &(client->sem_read));
    for(i = 0; i < 5; i++){

        int cod_error;
        cod_error = send(client->socket, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {

            error(ERROR_CONNECT);
            
            //break;

        }

        char buffer_aux[4097] = {0};
        RECEIVE_MESSAGE_STRUCT recv_msg;
        recv_msg.message = buffer_aux;
        recv_msg.client = client;
        recv_msg.x = 1;
        exec_n_segundos(1,receive_message_aux,&recv_msg,&(client->mutex),&(client->cond));

        if(strcmp(buffer_aux,"<ACK>") == 0){
            ack = 1;
            break;

        }
        //esperar um pouco para tentar enviar denovo.
        wait(500000);
        
    }
    sem_post( &(client->sem_read) );
    sem_wait( &(client->sem_sending_msg));
    client->cont_sending_msg = client->cont_sending_msg-1;
    sem_post( &(client->sem_sending_msg));



    if(ack == 0){

        pthread_t disconect;
        pthread_create(&(disconect), NULL, disconnect_cli, (void*)(client));
        printf("Conexão perdida com o cliente %s.\n", client->nickname);    
        wait(500000);

    }


    pthread_exit((void*)&ret_thread);

}

/*pode se virar thread para melhor paralelismo, mas nao eh necessario*/
void send_message(char*sender_nickname, char*message){

    sem_wait( &(sem_lista_clientes) );

    pthread_t threads_sends[num_cliente];
    SEND_MESSAGE_STRUCT send_msg[num_cliente];
    int i = 0;

    
    LIST_ELEMENT* no = lista_clientes->listfirst;
    
    /*criando as threads para mandar mensagens*/
    while(no != NULL){

        CLIENT *cli = no->client_ele;

        if(strcmp(sender_nickname,cli->nickname) != 0){
            strcpy(send_msg[i].client_from_nickname, sender_nickname);
            strcpy(send_msg[i].client_from_message, message);
            send_msg[i].client = cli;
            pthread_create(&(threads_sends[i]), NULL, send_menssage_thread, (void*)(&(send_msg[i])));
            i++;
        }
        
        no = no->next;
    }
    sem_post( &(sem_lista_clientes) );

    /*esperando as threads acabarem*/
    int j;
    for(j = 0; j < i;j++){
        int thread_ret;
        pthread_join (threads_sends[j], (void*)&thread_ret);
        
    }

}

void *receive_message(void *param){

    CLIENT*client = (CLIENT*)param;
    int ret_thread = 0;    

    while(1)
    {   

        sem_wait( &(client->sem_connect_status) );
        if(client->connect_status == 0){
            sem_post( &(client->sem_connect_status) );
            break;
        }

        sem_post( &(client->sem_connect_status) );

        sem_wait( &(client->sem_read) );


        char buffer[MENS_SIZE+1] = {0};
        RECEIVE_MESSAGE_STRUCT recv_msg;
        recv_msg.message = buffer;
        recv_msg.client = client;
        recv_msg.x = 2;
        exec_n_segundos(2,receive_message_aux,&recv_msg,&(client->mutex),&(client->cond));

        char nickname[50];
        strcpy(nickname,client->nickname);


        sem_post( &(client->sem_read) );

        if( strlen(buffer) > 0 ){

            if(strcmp(buffer,"PING") == 0){
                char pong[5] = {0};
                strcpy(pong,"PONG");
                
                send(client->socket, pong, 4, 0);


                printf("Ping recebido de %s.\n",nickname);
            }
            else if(strcmp(buffer,"/quit") == 0){

                pthread_t disconect;
                pthread_create(&(disconect), NULL, disconnect_cli, (void*)(client));
                printf("Cliente %s desconectando.\n",nickname);    
                wait(500000);

                break;
            }
            else{
                char ack[5] = {0};
                strcpy(ack,"ACK");
                
                send(client->socket, ack, 4, 0);
                printf("Mensagem \"%s\" recebida de %s, enviando aos outros clientes.\n",buffer,nickname);
                send_message(nickname, buffer);
                
            }

        }

        wait(50000);
    }

    pthread_exit(&ret_thread);
}

CLIENT *create_client(int sock,struct sockaddr_in addr,unsigned int size_addr, char*nickname){
    CLIENT* cli = (CLIENT*)malloc(sizeof(CLIENT));


    cli->socket = sock;
    cli->addr = addr;
    cli->size_addr = size_addr;
    strcpy(cli->nickname,nickname);

    cli->connect_status = 1;

    pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
    pthread_cond_t cond = PTHREAD_COND_INITIALIZER;
    cli->mutex = mutex;
    cli->cond = cond;

    sem_init(&(cli->sem_connect_status),0,1);
    sem_init(&(cli->sem_read),0,1);
    sem_init(&(cli->sem_sending_msg),0,1);
    pthread_create(&(cli->thread_listen), NULL, receive_message, (void*)(cli));

    return cli;
}


void* conecta_cliente(void *param){

    int*sock_task_end = (int*)param;
    int sock_task = *sock_task_end;
    
    int cont = 0;

    while(1){
        int sock_client;
        unsigned int size_client_addr;
        struct sockaddr_in addrclient;
        /*Esperando conecção de um cliente*/
        sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
        if(sock_client < 0){
            printf("Servidor Cheio, impossivel adicionar mais clientes\n");
            continue;
        }
                
        /*fazendo o nome padrao do usuario*/
        char standand_nickname[50];
        char aux[10];
        sprintf(aux,"%d",cont);

        strcpy(standand_nickname,"user");
        strcat(standand_nickname,aux);
        strcat(standand_nickname,"\0");

        printf("Cliente %s Conectado!\n",standand_nickname);

        
        /*Criando a struct CLIENT e o adicionando a lista de clientes*/
        CLIENT*new = create_client(sock_client,addrclient,size_client_addr, standand_nickname);
        
        sem_wait( &(sem_lista_clientes) );
        insert_list(lista_clientes, new);
        /*incrementando o numero de usuarios conectados*/
        num_cliente++;
        sem_post( &(sem_lista_clientes) );
        
        
        

        cont++;
    }
}

int main(){

    sem_init(&sem_lista_clientes,0,1);
    lista_clientes = creat_list();

    int sock_task = prepare_server_socket();

    pthread_t conectando_clientes;

    printf("Servidor Pronto!\n");
    printf("Esperando Clientes se conectarem.\n");

    pthread_create(&(conectando_clientes), NULL, conecta_cliente, (void*)(&(sock_task)));

    int thread_ret;
    pthread_join (conectando_clientes, (void *)&thread_ret);

    return 0;
}