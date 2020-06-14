#include "servidor3.h"
#include "list.h"
#include "channel.h"


/*FALTA

    - decidir o que fazer caso o adm saia do canal
        -- tratamento caso de se o adm sair, o canal continuar
    - verificar se o nome do canal no join eh valido, ver o que eh um nome valido no rfc
*/

sem_t sem_lista_canais;
LIST_CHANNEL *lista_canais;
int num_canais = 0;

sem_t sem_lista_clientes_sem_canal;
LIST *lista_clientes_sem_canal;
int num_cliente_sem_canal;

/*Contam quantos clientes estao conectados no servidor, apenas por questao de controle, nao tem muita utilidade,
serve so pro comando /num_clientes */
sem_t sem_num_clientes;
int num_clientes;

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

void servidor_send_message_to_client(CLIENT *cli,char*message){
    pthread_t threads_send_erro;
    SEND_MESSAGE_STRUCT send_msg;
    strcpy(send_msg.client_from_nickname, "<SERVIDOR>:");
    strcpy(send_msg.client_from_message, message);
    send_msg.client = cli;
    pthread_create(&(threads_send_erro), NULL, send_menssage_thread, (void*)(&(send_msg)));

    int thread_ret;
    pthread_join (threads_send_erro, (void*)&thread_ret);
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
    - void*cli_ - cliente a ser desconectado
*/
void *disconnect_cli(void *cli_){

    CLIENT* client = (CLIENT*)cli_;

    if(client == NULL)
        pthread_exit(NULL);

    /*utilizado para a thread_listen finalizar*/
    sem_wait( &(client->sem_connect_status) );
    /*isto significa que ja existe outra thread para desconectar este cliente.*/
    if(client->connect_status == 0){
        sem_post( &(client->sem_connect_status) );
        pthread_exit(NULL);
    }

    client->connect_status = 0;
    sem_post( &(client->sem_connect_status) );

    /*remover da lista de clientes sem canal, ou do canal que esta no momento, primeira coisa a se fazer, 
    /pois isto impossibilita de mandarem novas mensagens a este cliente, evitando seg fault e deadlock
    */
    /*Se o cliente nao esta em um canal*/
    if(client->channel == NULL){
        sem_wait( &(sem_lista_clientes_sem_canal) );
        sem_wait(&(client->sem_nickname));
        CLIENT* verifica = remove_list(lista_clientes_sem_canal, client->nickname);
        sem_post(&(client->sem_nickname));
        /*Abaixando o número de clientes conectados*/
        if(verifica != NULL)
            num_cliente_sem_canal--;
        sem_post( &(sem_lista_clientes_sem_canal) );
        /*se entrar aqui, significa que foi chamada duas funcoes para desligar */

    }
    else{

        //fazer
    }

    sem_wait(&(sem_num_clientes));
    num_clientes--;
    sem_post(&(sem_num_clientes));
    
    

    sem_wait( &(client->sem_read) );
    sem_post( &(client->sem_read) );

    //esperar ateh nao ter nenhuma thread mandando msg a este cliente.
    while(1){

        sem_wait( &(client->sem_sending_msg));

        if(client->cont_sending_msg == 0){
            break;
        }
        sem_post( &(client->sem_sending_msg));

        wait(50000);
    }

    void *thread_ret;
    pthread_join (client->thread_listen, &thread_ret);

    wait(5000000);

    /*utilizado para a thread_listen finalizar*/
    sem_wait( &(client->sem_connect_status) );
    sem_post( &(client->sem_connect_status) );
    sem_destroy(&(client->sem_connect_status));
    sem_destroy(&(client->sem_read));
    sem_destroy(&(client->sem_sending_msg));
    sem_destroy(&(client->sem_nickname));

    shutdown(client->socket, 1);

    

    free(client);

    client = NULL;
    

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
    if(client == NULL)
        pthread_exit(NULL);
    
    /*verificando se o cliente esta definitivamente conectado, ou se esta esperando seus processos em execucao acabar para desconectar*/
    sem_wait( &(client->sem_connect_status) );
    if(client->connect_status == 0){
        sem_post( &(client->sem_connect_status) );
        pthread_exit(NULL);
    }
    sem_post( &(client->sem_connect_status) );

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

    if(client == NULL)
        pthread_exit(NULL);

    /*verificando se o cliente esta definitivamente conectado, ou se esta esperando seus processos em execucao acabar para desconectar*/
    sem_wait( &(client->sem_connect_status) );
    if(client->connect_status == 0){
        sem_post( &(client->sem_connect_status) );
        pthread_exit(NULL);
    }
    sem_post( &(client->sem_connect_status) );

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

        send_message(client,buffer_msg);
    }


    pthread_exit((void*)&ret_thread);

}

/*Esta funcao manda a mensagem de um cliente para os outros, 
utiliza a send_message_thread para enviar para cada um
@PARAMETROS
    CLIENT* - o client que esta enviando a mensagem
    char *message - a mensagem que ele enviou
*/
void send_message(CLIENT*sender, char*message){

    if(sender == NULL || message == NULL)
        return;

    /*verificando se o cliente sender esta definitivamente conectado, ou se esta esperando seus processos em execucao acabar para desconectar*/
    sem_wait( &(sender->sem_connect_status) );
    if(sender->connect_status == 0){
        sem_post( &(sender->sem_connect_status) );
        return;
    }
    sem_post( &(sender->sem_connect_status) );

    /*verificando se o sender esta em um canal*/
    if(sender->channel == NULL){
        servidor_send_message_to_client(sender,"Nao Esta Conectado a Nenhum canal. Se conecte a um canal antes de enviar mensagens.");
        return;
    }
    /*verificando se o sender nao esta mutado*/
    sem_wait( &(sender->sem_muted) );              
    if(sender->muted){
        sem_post( &(sender->sem_muted) );
        servidor_send_message_to_client(sender,"Voce esta mutado no momento.");
        return;
    }
    sem_post( &(sender->sem_muted) );


    /*pegando o nickname do sender*/
    char sender_nickname[50];
    sem_wait( &(sender->sem_nickname) );  
    strcpy(sender_nickname,sender->nickname);
    sem_post( &(sender->sem_nickname) );


    CHANNEL*channel = (CHANNEL*)sender->channel;

    sem_wait( &(channel->sem_lista_clientes) );

    pthread_t threads_sends[num_cliente_sem_canal];
    SEND_MESSAGE_STRUCT send_msg[num_cliente_sem_canal];
    int i = 0;

    
    LIST_ELEMENT* no = channel->lista_clientes->listfirst;
    
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
    sem_post( &(channel->sem_lista_clientes) );

    /*esperando as threads acabarem*/
    int j;
    for(j = 0; j < i;j++){
        int thread_ret;
        pthread_join (threads_sends[j], (void*)&thread_ret);
        
    }

}

/*Esta funcao verifica se um cliente eh o admistrador do canal em que esta,
pela flag pode se fazer com que o servidor envie mensagem para o cliente, o avisando caso nao seja o adminstrador,
ou caso nao esteja em um canal,(nao avisa caso seja o adminstrador do canal)
@PARAMETROS
    - CLIENT * cli - cliente que queremos ver se eh o administrador
    - int flag - valores possiveis
        0 - nao envia mensagem ao cliente
        1 - envia mensagem ao cliente
@RETORNO
    int - 1 caso seja o administrador e 0 caso contrario.
*/
int client_is_adm(CLIENT *client, int flag){
    if(client == NULL)
        return 0;

    CHANNEL*channel = (CHANNEL*)client->channel;
    
    /*verificando se o canal existe, e se o cliente que mandou o comando eh o adm*/
    if(channel == NULL){
        if(flag == 1)
            servidor_send_message_to_client(client,"Nao esta conectado em um Canal");
        return 0;

    }else{
        sem_wait( &(client->sem_nickname) );
        int cmp = strcmp(channel->admin_name,client->nickname);
        sem_post( &(client->sem_nickname) );
        if(cmp != 0){

            if(flag == 1){
                char message[4096];
                sprintf(message,"Comando permitido apenas para o Administrador do Canal.%s eh o administrador do canal",channel->admin_name);
                servidor_send_message_to_client(client,message);
            }
            
            return 0;
        }

    } 

    return 1;
}

/*Esta funcao muta ou desmuta um cliente em um determinado canal, mediante ao parametro mute, mandando mensagem 
de confirmacao para o administrador do canal
@PARAMETROS
    - CHANNEL*channel - canal onde mutara o cliente
    - CLIENT *admin - administrador do canal
    - char *nickname - nickname do cliente que sera mutado
    - int mute - valores que pode assumir:
        0 - desmutar o cliente.
        1 - mutar o cliente.
@RETORNO
    int - 0 com operacao bem sucessida, 1 se nao foi possivel achar um cliente com o nickname, -1 em caso de erro
*/
int mute_client(CHANNEL*channel,CLIENT*admin,char*nickname,int mute){

    if(channel == NULL || admin == NULL || nickname == NULL)
        return -1;

    if(mute != 0 && mute != 1)
        return -1;

    /*pesquisando na lista de clientes no canal um cliente com o nickname passado no parametro*/
    sem_wait( &(channel->sem_lista_clientes) );
    CLIENT* cli_muted = list_get_client_by_name(channel->lista_clientes, nickname);
    sem_post( &(channel->sem_lista_clientes) );
    /*se achou um cliente*/
    if(cli_muted != NULL){

        /*mutando o cliente atraves da flag*/
        sem_wait( &(cli_muted->sem_muted) );
        cli_muted->muted = mute;
        sem_post( &(cli_muted->sem_muted) );
        /*enviando mensagem de confirmacao ao adm*/
        char message[4096];

        if(mute == 1)
            sprintf(message,"%s mutado com sucesso.",nickname);

        if(mute == 0)
            sprintf(message,"%s desmutado com sucesso.",nickname);
        servidor_send_message_to_client(admin,message);
        return 0;
    }else{
        
        servidor_send_message_to_client(admin,"Nao foi achado um cliente com o nickname passado");
        return 1;
    }
}

/*Esta funcao eh responsavel por tentar mudar o nickname de um cliente
@PARAMETROS
    - CLIENT * client - cliente que se deseja mudar o nickname
    - char *nickname_atual - nickname atual do cliente
    - char *novo_nickname - nickname para o qual se deseja mudar
@RETORNO
    - int - 1 caso tenha mudado o nickname, 0 caso nao seja possivel mudar o nickname por ja existir alguem com 
    este nick ou o cliente estar mutado no momento
*/
int client_change_name(CLIENT*client, char* nickname_atual, char*novo_nickname){

    if(client == NULL || nickname_atual == NULL || novo_nickname == NULL)
        return -1;

    /*Verifica se o client esta mutado, um cliente mutado nao pode mudar seu nickname*/
    sem_wait( &(client->sem_muted) );
    if(client->muted){
        sem_post( &(client->sem_muted) );
        servidor_send_message_to_client(client,"Nao se pode mudar o nickname estando mutado, saia do canal ou espere o administrador te desmutar.");
        return 0;
    }
    sem_post( &(client->sem_muted) );

    /*verificando se tem algum cliente ja com este nome no canal que ele esta
    caso ele esteja na lista de clientes sem canais, verifica se nao tem nenhum com o nickname
    que deseja na lista de sem canais*/
                
    CLIENT *cli_exist = NULL;
    CHANNEL *channel = (CHANNEL*)client->channel;
    /*Se o cliente esta sem canal*/
    if(client->channel == NULL){
        sem_wait( &(sem_lista_clientes_sem_canal) );
        cli_exist = list_get_client_by_name(channel->lista_clientes, novo_nickname);
        /*nao existe um cliente com este nickname ainda*/
        if(cli_exist == NULL){

            
            sem_wait( &(client->sem_nickname) );
            strcpy(client->nickname,novo_nickname);
            
            sem_post( &(client->sem_nickname) );
        }else{
            servidor_send_message_to_client(client,"Ja existe alguem com este nickname sem canal.");
        }

        sem_post( &(sem_lista_clientes_sem_canal) );
    }
    /*Se o cliente esta em um canal*/
    else{
        /*pesquisando na lista de clientes no canal um cliente com o nickname que deseja*/
        sem_wait( &(channel->sem_lista_clientes) );
        cli_exist = list_get_client_by_name(channel->lista_clientes, novo_nickname);
        /*nao existe um cliente com este nickname ainda*/
        if(cli_exist == NULL){

            int is_adm = client_is_adm(client, 0);
            sem_wait( &(client->sem_nickname) );
            strcpy(client->nickname,novo_nickname);
            /*se eh o adm do canal, tbm mudar o nickname dele na struct canal*/
            if(is_adm){
                strcpy(channel->admin_name,novo_nickname);
            }
            sem_post( &(client->sem_nickname) );
        }else{
            servidor_send_message_to_client(client,"Ja existe alguem com este nickname no canal em que esta.");
        }
        sem_post( &(channel->sem_lista_clientes) );
    }
    /*enviando mensagem de confirmacao de operacao bem sucedida, 
    e avisando os outros clientes do canal que este cliente mudou de nome*/
    if(cli_exist == NULL){
        servidor_send_message_to_client(client,"Nickname mudado com sucesso");
        char message[4096];
        sprintf(message,"%s mudou o seu nickname para %s.",nickname_atual,novo_nickname);
        send_message(client,message);
        return 1;
    }
    return 0;
}

/*Esta funcao eh responsavel por tentar kickar um cliente de um canal
@PARAMETROS
    - CHANNEL* channel - canal de onde o cliente sera kickado
    - CLIENT * admin - o administrador do canal
    - char *nickname_kick - nickname de quem sera kickado
*/
void channel_kick(CHANNEL*channel,CLIENT*admin,char*nickname_kick){
    
    /*pegando o nickname do administrador*/
    char nickname[50];
    sem_wait( &(admin->sem_nickname) );
    strcpy(nickname,admin->nickname);
    sem_post( &(admin->sem_nickname) );

    /*O adm nao pode se kickar, para se sair do canal, deve utilizar join*/
    if(strcmp(nickname,nickname_kick) == 0){

        servidor_send_message_to_client(admin,"Nao pode-se kickar do canal, utilize o comando /join para se retirar.");
        return;
    }

    CLIENT*to_remove = channel_remove_client(channel,nickname_kick);

    if(to_remove != NULL){

        /*adicionando o cliente removido a lista de clientes sem canal*/
        sem_wait( &(sem_lista_clientes_sem_canal) );
        insert_list(lista_clientes_sem_canal, to_remove);
        /*incrementando o numero de usuarios conectados*/
        num_cliente_sem_canal++;
        sem_post( &(sem_lista_clientes_sem_canal) );

        /*avisando o administrador e o cliente removido que se foi removido com sucesso*/
        servidor_send_message_to_client(admin,"Cliente removido com sucesso.");
        servidor_send_message_to_client(to_remove,"Voce foi removido do canal pelo administrador, esta sem canal no momento.");
    }else{
        servidor_send_message_to_client(admin,"Nao foi encontrado nenhum cliente no canal com o nickname passado.");
    }

    
}

/*Esta funcao tem o proposito de colocar um cliente em um canal ou na lista de clientes sem canal, dependendo da flag
@PARAMETROS
    - CLIENTE*client - cliente que sera movido
    - int flag - valores possiveis
        0 - entrasse em um canal
        1 - eh colocado na lista de clientes sem canal
    char*channel_name - caso for entrar em um canal, especificar o seu nomes
@RETORNO
    - 0 caso operacao em sucedida, -1 caso contrario
*/
int join_channel(CLIENT*client, int flag, char*channel_name){
    if(client == NULL)
        return -1;

    if(flag != 0 && flag != 1){
        return -1;
    }

    if(flag == 0 && channel_name == NULL)
        return -1;

    /*pegando o nickname do administrador*/
    char nickname[50];
    sem_wait( &(client->sem_nickname) );
    strcpy(nickname,client->nickname);
    sem_post( &(client->sem_nickname) );

    /*removendo o cliente do canal em que esta, ou da lista de sem canal caso esteja sem canal*/
    CHANNEL* channel_ant = (CHANNEL*)client->channel;
    /*se nao estava em um canal*/
    if(channel_ant == NULL){
        sem_wait( &(sem_lista_clientes_sem_canal) );
        remove_list(lista_clientes_sem_canal, nickname);
        num_cliente_sem_canal--;
        sem_post( &(sem_lista_clientes_sem_canal) );
    }
    /*removendo o cliente do canal em que estava*/
    else{
        /*verificando se era o adm do canal que esta saindo*/
        int is_adm = client_is_adm(client, 0);

        channel_remove_client(channel_ant,nickname);
                    
        if(is_adm){
            /*decidir o que fazer caso o adm esteja saindo do canal*/

        }


    }
                
    if(flag == 0){
        CHANNEL *novo_channel;
        /*vendo se ja existe o canal que o cliente quer se conectar*/
        sem_wait(&(sem_lista_canais));
        novo_channel = list_get_channel_by_name(lista_canais,channel_name);
        int is_adm = 0;
        /*nao existe o canal ainda*/
        if(novo_channel == NULL){
            /*ainda deve se inserir o cliente no canal*/
            novo_channel = create_channel(channel_name,nickname);
            list_insert_channel(lista_canais,novo_channel);
            is_adm = 1;
        }
        sem_post(&(sem_lista_canais));

        /*inserindo o cliente  no canal*/
        int err = channel_insert_client(novo_channel,client);

        /*caso nao se conseguiu entrar no canal, pois ha outro cliente com o mesmo nickname no canal*/
        if(err == -2){
            /*fazendo um nickname que com toda certeza eh valido, atraves do contador de quantas pessoas ja se conectaram ao canal*/
            char standand_nickname[50];
            char aux[10];
            sprintf(aux,"%d",novo_channel->num_clientes_ja_conectados);

            strcpy(standand_nickname,"user");
            strcat(standand_nickname,aux);
            strcat(standand_nickname,"\0");

            sem_wait(&(client->sem_nickname));
            strcpy(client->nickname,standand_nickname);
            sem_post(&(client->sem_nickname));

            /*adicionando novamente ao canal, agora com garatia de nao ter nenhum nome repetido*/
            channel_insert_client(novo_channel,client);
            char message[4096];
            sprintf(message,"Seu nickname foi mudado para %s, pois ja existe um cliente com o nickname %s neste canal",standand_nickname,nickname);
            servidor_send_message_to_client(client,message);

        }
        client->channel = (void*)novo_channel;
        if(is_adm){
            servidor_send_message_to_client(client,"Canal criado com Sucesso, voce eh o administrador do canal");
        }
        else{
            servidor_send_message_to_client(client,"Se juntou ao canal com sucesso");
        }
                    


    /*caso queira dar unjoin no canal*/
    }else if(flag == 1){

        /*inserindo na lista de clientes sem canal.*/
        sem_wait( &(sem_lista_clientes_sem_canal) );
        insert_list(lista_clientes_sem_canal, client);
        client->channel = NULL;
        /*incrementando o numero de usuarios conectados*/
        num_cliente_sem_canal++;
        sem_post( &(sem_lista_clientes_sem_canal) );

        servidor_send_message_to_client(client,"Voce esta agora sem nenhum Canal!!");

    }

    return 0;
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
        if(client == NULL)
            pthread_exit(NULL);

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

        sem_wait( &(client->sem_nickname) );
        char nickname[50];
        strcpy(nickname,client->nickname);
        sem_post( &(client->sem_nickname) );

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
            /*Conectando este cliente a um novo canal*/
            else if(strncmp(buffer,"/join ",6) == 0){

                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                
                join_channel(client, 0, parametro);
                
            }
            /*Retirando o cliente do canal*/
            else if(strncmp(buffer,"/unjoin ",6) == 0){

                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                
                join_channel(client, 1, NULL);
                
            }
            /*Tentando mudar o apelido do cliente*/
            else if(strncmp(buffer,"/nickname ",10) == 0){
                /*tratando o buffer, separando o comando de seu parametro*/
                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );

                client_change_name(client, nickname, parametro);


            }
            /*A partir daqui sao comandos de adm*/
            /*Remove um usuario do canal, */
            else if(strncmp(buffer,"/kick ",6) == 0){
                /*tratando o buffer, separando o comando de seu parametro*/
                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                /*verificando se este usuario eh o administrador do canal*/
                CHANNEL*channel = (CHANNEL*)client->channel;
                int is_adm = client_is_adm(client, 1);

                if(is_adm){
                    channel_kick(channel,client,parametro);
                }
                
            }
            /**/
            else if(strncmp(buffer,"/mute ",6) == 0){
                /*tratando o buffer, separando o comando de seu parametro*/
                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                /*verificando se este usuario eh o administrador do canal*/
                CHANNEL*channel = (CHANNEL*)client->channel;
                int is_adm = client_is_adm(client, 1);
                

                if( is_adm ){
                    /*chamando a funcao para mutar o cliente passado como parametro*/
                    mute_client(channel,client,parametro,1);
                }
            }
            else if(strncmp(buffer,"/unmute ",8) == 0){
                /*tratando o buffer, separando o comando de seu parametro*/
                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                /*verificando se este usuario eh o administrador do canal*/
                CHANNEL*channel = (CHANNEL*)client->channel;

                int is_adm = client_is_adm(client, 1);

                if( is_adm ){
                    /*chamando a funcao para desmutar o cliente passado como parametro*/
                    mute_client(channel,client,parametro,0);
                }
            }
            else if(strncmp(buffer,"/whois ",7) == 0){
                
                /*tratando o buffer, separando o comando de seu parametro*/
                char comando[10];
                char parametro[50];
                sscanf( buffer,"%s %s",comando,parametro );
                /*verificando se este usuario eh o administrador do canal*/
                CHANNEL*channel = (CHANNEL*)client->channel;
                int is_adm = client_is_adm(client, 1);

                if( is_adm ){
                    /*pesquisando na lista de clientes no canal um cliente com o nickname passado no parametro*/
                    sem_wait( &(channel->sem_lista_clientes) );
                    CLIENT* cli_muted = list_get_client_by_name(channel->lista_clientes, parametro);
                    sem_post( &(channel->sem_lista_clientes) );
                    /*se achou um cliente*/
                    if(cli_muted != NULL){
                        /*pegando ip do cliente achado*/
                        char ip[20];
                        byte_to_string_ip_adress(ip,cli_muted->addr.sin_addr.s_addr);
                        /*enviando o ip do cliente achado ao adm*/
                        char message[4096];
                        sprintf(message,"O IP de %s eh %s.",parametro,ip);
                        servidor_send_message_to_client(client,message);
                    }else{
                        servidor_send_message_to_client(client,"Nao foi achado um cliente com o nickname passado");
                    }
                }
            }
            /*mensagem normal, retorna o ACK, e envia a mensagem aos outros clients.*/
            else{
                char ack[5] = {0};
                strcpy(ack,"ACK");
                
                send(client->socket, ack, 4, 0);

                printf("Mensagem \"%s\" recebida de %s, enviando aos outros clientes do canal conectado.\n",buffer,nickname);
                send_message(client, buffer);           
                
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
    cli->channel = NULL;
    cli->muted = 0;
     
    sem_init(&(cli->sem_nickname),0,1);
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
        sem_wait( &(sem_lista_clientes_sem_canal) );
        insert_list(lista_clientes_sem_canal, new);
        /*incrementando o numero de usuarios conectados*/
        num_cliente_sem_canal++;
        sem_post( &(sem_lista_clientes_sem_canal) );

        sem_wait(&(sem_num_clientes));
        num_clientes++;
        sem_post(&(sem_num_clientes));
        
        char message[4096];
        sprintf(message,"Foi lhe atributo o nickname %s.",standand_nickname);
        servidor_send_message_to_client(new,message);
        

        cont++;
    }
}

/*Esta funcao nao remove o canal da lista de canais, deve se tirar o canal da lista antes de chamar esta funcao caso nao esteja 
dando shutdown no servidor todo.
Esta função tem o proposito de remover um canal que ja foi retirado da lista de canais, podendo se escolher o que acontece
com os clientes que estao no canal atraves da flag
@PARAMETROS
    - CHANNEL* channel - canal a ser deletado
    - int flag - valores possiveis:
        0 - coloca os clientes que estao no canal na lista de clientes sem canal
        1 - desconecta os clientes que estao no canal do servidor
*/
void shutdown_channel(CHANNEL* channel, int flag){

    if(flag != 0 && flag != 1){
        return;
    }

    if(channel == NULL){
        return;
    }

    sem_wait( &(channel->sem_lista_clientes) );
    pthread_t threads_disconnect[channel->num_cliente];
    int i = 0;
    LIST_ELEMENT* no = channel->lista_clientes->listfirst;
    /*iterando sobre os nos da lista, para pegar os clientes que estao nela*/
    while(no != NULL){

        CLIENT *client = no->client_ele;

        /*Se eh para desconectar os clientes*/
        if(flag == 1){
            /*criando as threads para desconectar o cliente*/
            pthread_create(&(threads_disconnect[i]), NULL, disconnect_cli, (void*)(client));
        }
        /*se eh para colocar os clientes na lista de clientes sem canal*/
        else if(flag == 0){
            client->channel = NULL;
            sem_wait( &(client->sem_muted) );
            client->muted = 0;
            sem_post( &(client->sem_muted) );

            /*adicionando o cliente */
            sem_wait(&(sem_lista_clientes_sem_canal));
            insert_list(lista_clientes_sem_canal, client);
            /*incrementando o numero de usuarios conectados*/
            num_cliente_sem_canal++;
            sem_post(&(sem_lista_clientes_sem_canal));
        }
        
        i++;
        no = no->next;
    }
    sem_post( &(channel->sem_lista_clientes) );
    /*Se eh para desconectar os clientes*/
    if(flag == 1){
        /*esperando as threads que desconectam os clientes acabarem*/
        int j;
        for(j = 0; j < i;j++){
            int thread_ret;
            pthread_join (threads_disconnect[j], (void*)&thread_ret);
            
        }
    }

    delete_channel(channel);
}

/*Esta funcao desliga o servidor, necessario cancelar que novos clientes se conectem.
@PARAMETROS
    int sock_task - socket aberto do servidor.
*/
void shutdown_server(int sock_task){

    sem_wait(&(sem_lista_canais));

    NO* no_it = lista_canais->ini;
    /*iterando sobre a lista de canais, chamando a funcao shutdown_channel para deletalos*/
    while(no_it != NULL){
        /*usando a flag de desconectar os clientes*/
        shutdown_channel(no_it->channel, 1);
        no_it = no_it->next;
    }


    sem_post(&(sem_lista_canais));

    sem_wait(&(sem_lista_clientes_sem_canal));
    pthread_t threads_disconnect[num_cliente_sem_canal];
    int i = 0;
    LIST_ELEMENT* no = lista_clientes_sem_canal->listfirst;
    
    /*criando as threads para desconectar todos os clientes*/
    while(no != NULL){

        CLIENT *client = no->client_ele;

        pthread_create(&(threads_disconnect[i]), NULL, disconnect_cli, (void*)(client));
        i++;
        no = no->next;
    }
    sem_post(&(sem_lista_clientes_sem_canal));

    /*esperando as threads que desconectam os clientes acabarem*/
    int j;
    for(j = 0; j < i;j++){
        int thread_ret;
        pthread_join (threads_disconnect[j], (void*)&thread_ret);
        
    }

    sem_destroy(&(sem_lista_clientes_sem_canal));  
    sem_destroy(&(sem_lista_canais));  
    sem_destroy(&(sem_num_clientes)); 
    shutdown(sock_task,1);
    delete_list(lista_clientes_sem_canal);
    list_channel_delete(lista_canais);
    
    printf("Servidor Desligado\n");

}

int main(){

    sem_init(&sem_lista_canais,0,1);
    lista_canais = create_list_channel();
    
    sem_init(&sem_lista_clientes_sem_canal,0,1);
    lista_clientes_sem_canal = creat_list();

    sem_init(&sem_num_clientes,0,1);
    num_clientes = 0;

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
            printf("O número de clientes conectados eh: %d\n",num_clientes);
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