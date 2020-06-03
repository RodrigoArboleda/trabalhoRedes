#include "servidor2.h"
#include "list.h"



sem_t sem_lista_clientes;
LIST *lista_clientes;
int num_cliente = 0;

/*A STRUCT CLIENT esta definida em utilities.h, por a lista necessitar de saber da struct*/

typedef struct SEND_MESSAGE_STRUCT_{
    char client_from_nickname[40];
    char client_from_message[40];
    CLIENT*client;
}SEND_MESSAGE_STRUCT;

typedef struct RECEIVE_MESSAGE_STRUCT_{
    char *message;
    CLIENT*client;

}RECEIVE_MESSAGE_STRUCT;


/*Esta função faz com que a execucação de quem o chamou espere por x*10 nanosegundos
@PARAMETROS
    - int x - tempo que vai ficar parado
*/
void wait(int x){

    struct timespec time_sleep;

    time_sleep.tv_nsec = x;
    time_sleep.tv_sec = 0;

    for (int i = 0; i < 10; i++)
    {


        nanosleep(&time_sleep, NULL);
    }
    
}

/*Esta prepara o socket do servidor, associando o endereco e o deixando pronto para os
clientes se conectarem
@RETORNO
    - int - socket aberto para coneccao.
*/
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

/*Esta função faz o processo de desconectar um cliente do servidor
@PARAMETROS
    - int x - tempo que vai ficar parado
*/
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
    /*utilizado para a thread_listen finalizar*/
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

/*Esta função deve ser iniciada por uma thread, ela vai receber as mensagens que um client enviou e guardado
em char*message da struct RECEIVE_MESSAGE_STRUCT.
Esta funcao sempre eh chamada com exec_n_segundos, pois precisa para de tentar de ler apos um tempo.
Se da timeout na função por causa do recv, pois ele eh bloqueante, e se colocar como nao bloqueante, 
teria que lidar com a perca de sincronismo. 
@PARAMETROS
    - void * param - na verdade eh um RECEIVE_MESSAGE_STRUCT*recv_msg, se passa na struct onde 
sera guardado a mensagem, de que CLIENT vai receber a mensagem.
*/
void *receive_message_aux(void *param){

    int oldtype;

    /* allow the thread to be killed at any time */
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);


    RECEIVE_MESSAGE_STRUCT recv_msg = *( (RECEIVE_MESSAGE_STRUCT*)param);
    CLIENT*client = recv_msg.client;
    int ret_thread = 0;

    int cod_error;

    /*Para ter certeza que esta lendo uma mensagem*/
    while(strlen(recv_msg.message) == 0)
        cod_error = recv(client->socket, recv_msg.message, MENS_SIZE,0);

    
    if (cod_error <= 0)
    {
        error(ERROR_CONNECT);
        ret_thread = ERROR_CONNECT_COD;
        printf(MSG_END_CONNECT);

    }
    /*enviando o sinal pra exec_n_segundos, para liberar a funcao que a chamou*/
    pthread_cond_signal(&(client->cond));
    pthread_exit((void*)&ret_thread);
}

/*Esta função deve ser iniciada por uma thread, esta funcao tem como objetivo enviar a um 
cliente, uma mensagem que outro client enviou.
@PARAMETROS
    - void * param - na verdade eh um SEND_MESSAGE_STRUCT*param, na struct eh definido 
o nickname de quem enviou, a mensagem, e o CLIENT*cli para quem vai enviar.
*/
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
    
    /*montando a mensagem, com o nome de quem enviou*/
    char buffer[4097];
    strcpy(buffer,send_msg.client_from_nickname);
    strcat(buffer,": ");
    strcat(buffer, send_msg.client_from_message);
    strcat(buffer, "\0");
    //a concatenacao com o nickname pode passar os 4096 bytes, por isto a linha abaixo
    buffer[4096] = '\0';
    

    int i;
    int ack = 0;
    /*precisasse reserva a leitura, pois precisase ler os acks*/
    sem_wait( &(client->sem_read));

    char buffer_msg[4097] = {0};
    int have_msg = 0;

    /*O for para as 5 tentativas de enviar a mensagem*/
    for(i = 0; i < 5; i++){

        /*enviando a mensagem*/
        int cod_error;
        cod_error = send(client->socket, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {

            error(ERROR_CONNECT);
            
            //break;

        }

        /*montando a struct RECEIVE_MESSAGE_STRUCT para se receber o ack*/
        char buffer_aux[4097] = {0};
        RECEIVE_MESSAGE_STRUCT recv_msg;
        recv_msg.message = buffer_aux;
        recv_msg.client = client;
        /*chamando a receibe_message_aux atraves do exec_n_segundos, para dar timeout nela*/
        /*esperasse um segundo para enviar a mensagem*/
        exec_n_segundos(1,receive_message_aux,&recv_msg,&(client->mutex),&(client->cond));
        /*verificando se o que foi lido, eh um ack*/
        if(strcmp(buffer_aux,"<ACK>") == 0){
            ack = 1;
            break;

        
        }/*se nao for um ack, eh uma mensagem que o cliente enviou, 
        vai se guardar a mensagem, para enviala depois de confirmar a atual*/
        else if(strlen(buffer_aux) > 0){
            strcpy(buffer_msg,buffer_aux);
            have_msg = 1;

            /*por nao ter lido o ack por causa de ter lido outra mensagem antes, o cliente pode ainda ter mandado o ack,
            entao tenta recebelo denovo*/
            recv_msg.message[0] = '\0';
            exec_n_segundos(1,receive_message_aux,&recv_msg,&(client->mutex),&(client->cond));
        }

        //esperar um pouco para tentar enviar denovo.
        wait(500000);
        
    }
    /*liberando os semaforos, e decrementando o contador.*/
    sem_post( &(client->sem_read) );
    sem_wait( &(client->sem_sending_msg));
    client->cont_sending_msg = client->cont_sending_msg-1;
    sem_post( &(client->sem_sending_msg));


    /*se nao foi confirmado o recebimento da mensagem, disconecta o cliente*/
    if(ack == 0){

        pthread_t disconect;
        pthread_create(&(disconect), NULL, disconnect_cli, (void*)(client));
        printf("Conexão perdida com o cliente %s.\n", client->nickname);    
        wait(500000);

    }
    /*se foi lido uma mensagem que nao seja o ack, 
    o else eh porque nao faz sentido mandar mensagem de um cliente desconectado*/
    else if(have_msg){
        char nick[50];
        strcpy(nick,client->nickname);

        send_message(nick,buffer_msg);
    }


    pthread_exit((void*)&ret_thread);

}

/*Esta funcao manda a mensagem de um cliente para os outros, 
utiliza a send_message_thread para enviar para cada um
@PARAMETROS
    char *sender_nickname - o nickname de quem enviou a mensagem
    char *message - a mensagem que ele enviou
*/
void send_message(char*sender_nickname, char*message){

    sem_wait( &(sem_lista_clientes) );

    pthread_t threads_sends[num_cliente];
    SEND_MESSAGE_STRUCT send_msg[num_cliente];
    int i = 0;

    
    LIST_ELEMENT* no = lista_clientes->listfirst;
    
    /*criando as threads para mandar mensagens*/
    /*percorrendo a lista de clientes*/
    while(no != NULL){

        CLIENT *cli = no->client_ele;
        /*nao devese enviar a mensagem para que a mandou*/
        if(strcmp(sender_nickname,cli->nickname) != 0){
            /*montando a SEND_MESSAGE_STRUCT especifica*/
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

/*Esta funcao vai ser chamada pela thread_listen de cada client,
Esta funcao fica escutando o client para ver se ele enviou alguma mensagem.
@PARAMETROS
    - void *param- na verdade eh um CLIENT*param - o client que vamos escutar
*/
void *receive_message(void *param){

    CLIENT*client = (CLIENT*)param;
    int ret_thread = 0;    

    /*escutando indefinidamente*/
    while(1)
    {   
        /*caso o cliente esteja se desconectando do servidor, o se connect_status fica 0 e ele sai do loop e encerra a thread.*/
        sem_wait( &(client->sem_connect_status) );
        if(client->connect_status == 0){
            sem_post( &(client->sem_connect_status) );
            break;
        }

        sem_post( &(client->sem_connect_status) );
        /*reservando o semaforo de leitura*/
        sem_wait( &(client->sem_read) );

        /*montando a struct RECEIVE_MESSAGE_STRUCT para se receber o ack*/
        char buffer[MENS_SIZE+1] = {0};
        RECEIVE_MESSAGE_STRUCT recv_msg;
        recv_msg.message = buffer;
        recv_msg.client = client;
        /*chamando a receibe_message_aux atraves do exec_n_segundos, para dar timeout nela*/
        /*esperasse um segundo para enviar a mensagem*/
        exec_n_segundos(2,receive_message_aux,&recv_msg,&(client->mutex),&(client->cond));
        /*verificando se o que foi lido, eh um ack*/

        char nickname[50];
        strcpy(nickname,client->nickname);


        sem_post( &(client->sem_read) );

        if( strlen(buffer) > 0 ){
            /*se for um PING, responde PONG*/
            if(strcmp(buffer,"/ping") == 0){
                char pong[5] = {0};
                strcpy(pong,"PONG");
                
                send(client->socket, pong, 4, 0);


                printf("Ping recebido de %s.\n",nickname);
            }
            /*se for um quit, desconecta o client*/
            else if(strcmp(buffer,"/quit") == 0){

                pthread_t disconect;
                pthread_create(&(disconect), NULL, disconnect_cli, (void*)(client));
                printf("Cliente %s desconectando.\n",nickname);    
                wait(500000);

                break;
            }
            /*mensagem normal, retorna o ACK, e envia a mensagem aos outros clients.*/
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

/*Cria uma struct CLIENT
@PARAMETROS
    int sock - socket do client
    struct sockaddr_in addr - endereco do socket do client
    unsigned int size_addr - tamanho do endereco
    char*  nickname - nickname do cliente.
@RETORNO
    CLIENT * - cliente criado.
*/
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

/*Esta funcao eh a thread que fica conectando o servidor com os clientes que estao tentando
se conectar.
@PARAMETROS
    void * param - na verdade eh um int*param, eh um ponteiro para o socket aberto do servidor.
*/
void* conecta_cliente(void *param){

    int*sock_task_end = (int*)param;
    int sock_task = *sock_task_end;
    
    int cont = 0;
    /*loop infinito*/
    while(1){

        /*recebendo os clientes que estao se conectando.*/
        int sock_client;
        unsigned int size_client_addr;
        struct sockaddr_in addrclient;
        /*Esperando conecção de um cliente*/
        sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
        if(sock_client < 0){
            printf("ERRO ao tentar conectar um cliente.\n");
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
        
        /*inserindo na lista de clientes.*/
        sem_wait( &(sem_lista_clientes) );
        insert_list(lista_clientes, new);
        /*incrementando o numero de usuarios conectados*/
        num_cliente++;
        sem_post( &(sem_lista_clientes) );
        
        
        

        cont++;
    }
}

/*Esta funcao desliga o servidor, necessario cancelar que novos clientes se conectem.
@ARAMETROS
    int sock_task - socket aberto do servidor.
*/
void shutdown_server(int sock_task){
    sem_wait(&(sem_lista_clientes));
    pthread_t threads_disconnect[num_cliente];
    int i = 0;
    LIST_ELEMENT* no = lista_clientes->listfirst;
    
    /*criando as threads para desconectar todos os clientes*/
    while(no != NULL){

        CLIENT *client = no->client_ele;

        pthread_create(&(threads_disconnect[i]), NULL, disconnect_cli, (void*)(client));
        i++;
        no = no->next;
    }
    sem_post(&(sem_lista_clientes));

    /*esperando as threads que desconectam os clientes acabarem*/
    int j;
    for(j = 0; j < i;j++){
        int thread_ret;
        pthread_join (threads_disconnect[j], (void*)&thread_ret);
        
    }

    sem_destroy(&(sem_lista_clientes));    
    shutdown(sock_task,1);
    delete_list(lista_clientes);
    
    printf("Servidor Desligado\n");

}

int main(){

    sem_init(&sem_lista_clientes,0,1);
    lista_clientes = creat_list();

    int sock_task = prepare_server_socket();

    pthread_t conectando_clientes;

    printf("Servidor Pronto!\n");
    printf("Esperando Clientes se conectarem.\n");

    /*iniciando a thread de conectar clientes.*/
    pthread_create(&(conectando_clientes), NULL, conecta_cliente, (void*)(&(sock_task)));

    /*lendo se deseja desligar o servidor, ou ver o numero de clientes conectados*/
    char comando[4097];
    while(scanf("%s",comando) != EOF){
        if(strcmp(comando,"/shutdown") == 0){
            break;
        }else if(strcmp(comando,"/numero_clientes") == 0){
            printf("O número de clientes conectados eh: %d\n",num_cliente);
        }
        
    }

    printf("Desligando o Servidor...\n");

    /*matando a thread que conecta novos clientes*/
    pthread_cancel(conectando_clientes);
    int thread_ret;
    pthread_join (conectando_clientes, (void *)&thread_ret);

    shutdown_server(sock_task);

    return 0;
}