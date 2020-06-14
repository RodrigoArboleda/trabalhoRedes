#include "utilities.h"

/*
Semaforos das threads

Modificar o ack-signal
*/
sem_t sem_ack;

/*
Enviar uma messagem para o servidor
*/
sem_t sem_send;

/*
Modificar o ping_signal
*/
sem_t sem_ping;

/*
Modificar o numero de fails de tentativa de se conectar ao
servidor
*/
sem_t sem_connect_fail;


/*
Armazena o sock de comunucacao do cliente, caso estaja -1
o cliente nao esta conectado
*/
int sock_server = -1;

/*
Armazena se o cliente esta esperando um ack do servidor
-1: aguardando
0: nao aguardando
1: ack recebido
*/
int ack_signal = 0;

/*
Armazena se o cliente esta esperando um retorno de ping do servidor
-1: aguardando
0: nao aguardando
1: pong recebido
*/
int ping_signal = 0;

/*
Armazena o numero de tentativas falhas de enviar mensagem
*/
int num_connect_fail = 0;


/*
Guarda as threads do sistema, a thread[0] é a de envio
e a thread[1] a de recebimento de mensagem
*/
pthread_t thread[2];

/*
Esta funcção trata o caso de o programa receber um sinal
@RETORNO
    void
*/
void stop_client(int sig){
    printf("\nPara encerrar o cliente digite: /quit\n");
    return;
}

/*
Esta função conecta o client a o servidor do endereço IP passado como parametro,
a porta eh a macro PORT_SERVER definida em servidor.h, salvando o socket 
conectado ao servidor
@PARAMETROS
    char *ip - endereço IP do server no formato string(X.X.X.X)
*/
int creat_connect(char* ip){

    sock_server = -1;

    int cod_error = 0;

    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0){
        error(ERROR_SOCK);
        exit(ERROR_CONNECT_COD);
    }

    sockaddr_ip addrserver;
    /*montando a struct do endereço do socket*/
    addrserver.sin_family = AF_INET; /*utilizando TCP*/
    addrserver.sin_addr.s_addr = string_to_byte_ip_adress(ip);
    addrserver.sin_port = invert_endian_16B(PORT_SERVER);


    cod_error = connect(sock_server, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));
    if (cod_error != 0)
    {
        sock_server = -1;
        return ERROR_CONNECT_COD;
    }

    return 0;
}

/*
Esta função espera um ACK do servidor. O tempo máximo de espera é de
500000000 nanosegundos
OBS: ack deve ser setado para -1
@RETORNO
    - int - 1 - caso tenha o receba o ACK
            0 - caso ACK seja recebido
*/
int wait_ack(){

    struct timespec time_sleep;

    time_sleep.tv_nsec = 50000000;
    time_sleep.tv_sec = 0;

    if (ack_signal == 0)
    {
        return 1;
    }

    int i;
    for (i = 0; i < 10; i++)
    {
        if (ack_signal == 1)
        {
            return 0;
        }

        nanosleep(&time_sleep, NULL);
    }
    
    return 1;

}

/*
Agurda o retorno do ping do servidor, caso o tempo maximo de espera(1250000000 nanosegundos) seja
atingido a funcao retorna, ou caso seja recebido algum retorno (PONG)
OBS: o ping_signal deve ser setado para -1
@RETORNO
    - int - 0 - caso o ping seja recebido corretamente
            1 - caso nao seja recebido o ping
*/
int wait_ping(){

    struct timespec time_sleep;

    time_sleep.tv_nsec = 25000000;
    time_sleep.tv_sec = 0;

    if (ping_signal == 0)
    {
        return 1;
    }
    
    int i;
    for (i = 0; i < 50; i++)
    {
        if (ping_signal == 1)
        {
            return 0;
        }

        nanosleep(&time_sleep, NULL);
    }
    
    return 1;

}

/*
Desconecta o cliente e encerra as threads de recive e send do progama.
OBS:Caso seja a thread recive ou send que tenha chamado a funcao ela nao
ira matar esta thread e ela deve encerrar apos o retorno da funcao
*/
void disconect(){
    printf("Encerrando conexão com o servidor...\n");
    
    sem_wait(&sem_ack);
    ack_signal = 0;
    sem_post(&sem_ack);

    sem_wait(&sem_ping);
    ping_signal = 0;
    sem_post(&sem_ping);

    if (thread[0] != pthread_self())
    {
        pthread_cancel(thread[0]);
    }
    
    
    if (thread[1] != pthread_self())
    {
        pthread_cancel(thread[1]);
    }

    char quit[6];
    strcpy(quit,"/quit");
    quit[5] = '\0';
    send(sock_server, quit, 6, 0);

    shutdown(sock_server, 1);
    sock_server = -1;
    
    printf("Conexão encerrada.\n");
    return;
}

/*
Esta funcao eh chamada quando acontece uma desconeccao por o 
servidor ser desligado por erro ligado a conecao com o servidor.
Desconecta o cliente e encerra as threads de recive e send do progama.
OBS:Caso seja a thread recive ou send que tenha chamado a funcao ela nao
ira matar esta thread e ela deve encerrar apos o retorno da funcao
*/
void disconect_erro(){
    printf("Encerrando conexão com o servidor...\n");
    
    sem_wait(&sem_ack);
    ack_signal = 0;
    sem_post(&sem_ack);

    sem_wait(&sem_ping);
    ping_signal = 0;
    sem_post(&sem_ping);

    if (thread[0] != pthread_self())
    {
        pthread_cancel(thread[0]);
    }
    
    
    if (thread[1] != pthread_self())
    {
        pthread_cancel(thread[1]);
    }

    shutdown(sock_server, 1);
    sock_server = -1;
    
    printf("Conexão encerrada.\n");
    return;
}

/*
Esta funcao envia a mensagem para o servidor conectado
a funcao espera o retorno ACK do servidor, caso nao receba
tenta eviar novamente 5 vezes, apos isso ela informa o erro
e registra uma falha ao enviar a mensagem
OBS: Esta funcao deve ser chamada em uma thread
@PARAMETROS
    - void* - endereco do buffer com a mensagem
*/
void *send_mensage(void* buffer_par){

    char* buffer = (char*)buffer_par;
    int ret_thread = 0;

    if (sock_server < 0)
    {
        ret_thread = -1;
        pthread_exit(&ret_thread);
    }

    int cod_error;

    sem_wait(&sem_send);
    cod_error = send(sock_server, buffer, MENS_SIZE, 0);
    sem_post(&sem_send);
    if (cod_error < 0)
    {
        error(ERROR_CONNECT);
        ret_thread = ERROR_CONNECT_COD;
        pthread_exit(&ret_thread);
    }

    sem_wait(&sem_ack);
    ack_signal = -1;
    sem_post(&sem_ack);

    /*loop que faz 5 tentativas para enviar a mensagem*/
    int i;
    for (i = 0; i < 5; i++)
    {
        int ack_ret;

        ack_ret = wait_ack();

        if (ack_ret == 0)
        {
            break;
        }

        /*reenvia caso nao tenha recebido retorno*/
        else
        {   
            sem_wait(&sem_send);
            cod_error = send(sock_server, buffer, MENS_SIZE, 0);
            sem_post(&sem_send);
            if (cod_error < 0)
            {
                error(ERROR_CONNECT);
                ret_thread = ERROR_CONNECT_COD;
                pthread_exit(&ret_thread);
            }
        }
        
    }

    /*verifica se a mensagem foi enviada e encerra a funcao*/
    sem_wait(&sem_ack);

    if (ack_signal == -1)
    {
        printf("Problema de conexao com servidor.\n");
        
        sem_wait(&sem_connect_fail);
        num_connect_fail++;
        sem_post(&sem_connect_fail);
        
        ret_thread = ERROR_CONNECT_COD;
    }

    ack_signal = 0;
    sem_post(&sem_ack);

    pthread_exit(&ret_thread);
}

/*
Esta funcao fica sempre rodando enquando a conexao estiver ativa. Ela
recebe todos os dados o qual o servidor envia para o cliente
OBS: Deve ser chama em uma thread
*/
void *receive_menssage(){

    int ret_thread = 0;

    int cod_error;
    
    /*loop de leitura*/
    while (sock_server >= 0)
    {
        char buffer[MENS_SIZE] = {0};

        /*le a mensagem*/
        cod_error = recv(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            ret_thread = ERROR_CONNECT_COD;
            disconect_erro();
            pthread_exit(&ret_thread);
        }
        /*verifica se e um ACK do servidor*/
        if (strcmp(buffer, "ACK") == 0)
        {
            sem_wait(&sem_ack);
            if (ack_signal == -1)
            {
                ack_signal = 1;
            }
            sem_post(&sem_ack);

            sem_wait(&sem_connect_fail);
            num_connect_fail = 0;
            sem_post(&sem_connect_fail);
        }
        /*verifica se é uma resposta do ping*/
        else if (strcmp(buffer, "PONG") == 0)
        {
            sem_wait(&sem_ping);
            if (ping_signal == -1)
            {
                ping_signal = 1;
                printf("%s\n", buffer);
            }
            sem_post(&sem_ping);
        }
        /*caso seja um mensagem mostra ao usuario*/
        else if(strlen(buffer) != 0)
        {
            
            sem_wait(&sem_send);
            /*retorna o ACK para o servidor*/
            char ack[6];
            strcpy(ack,"<ACK>");
            ack[5] = '\0';
            cod_error = send(sock_server, ack, 6, 0);
            sem_post(&sem_send);
            /*caso tenha algum erro ao enviar, encerra a conexao com servidor*/
            if (cod_error <= 0)
            {
                error(ERROR_CONNECT);
                ret_thread = ERROR_CONNECT_COD;
                disconect_erro();
                pthread_exit(&ret_thread);
            }

            printf("%s\n", buffer);
        }

        
    }

    pthread_exit(&ret_thread);
}

/*
Esta funcao trata caso um comando seja inserido pelo usuario no terminal do irc
@PARAMETROS
    - char* - o buffer com o commando dado pelo usuario
@RETORNO
    - int - 0 - encerro sem erros
            1 - o programa deve ser encerrado
            2 - tentativa de se conectar sendo que ja existe uma conexao ativa
            3 - comando desconhecido
            4 - sem conexao estabelecida
            5 - sem retorno do servidor
*/
int command(char* buffer){

    /*cada if verifica qual comando deve ser execultado*/
    if (strcmp(buffer, "/connect") == 0)
    {
        if (sock_server >= 0)
        {
            printf("Já conectado a um servido.\n");
            return 2;
        }

        else
        {
            char ip[15] = {0};

            printf("Digite o IP do servidor:\n");
            scanf("%15s", ip);
        
            printf("Se conectando a: %s...\n", ip);

            creat_connect(ip);

            /*verifica se foi conectado*/
            if(sock_server > 0){
                pthread_create(&thread[1], NULL, receive_menssage, NULL);
                printf("Connectado!\n");
            }   

            else
            {
                printf("Falha ao se conectar ao servidor\n");
            }
                     
            
            return 0;
        }
        
        
    }
    
    /*fecha as conexaos, mata as threads e retorna o pedido para encerar o programa*/
    else if (strcmp(buffer, "/quit") == 0)
    {
        printf("Encerrando cliente...\n");

        if (sock_server >= 0)
        {
            disconect();
        }

        return 1;
    }

    /*envia o ping e agurda o retorno do sevidor e calcula o tempo de resposta*/
    else if (strcmp(buffer, "/ping") == 0)
    {
        if (sock_server < 0)
        {
            printf("Nenhuma conexao estabelecida, digite: /connect\n");
            return 4;
        }

        else
        {
            strcpy(buffer, "/ping");
            
            sem_wait(&sem_ping);
            ping_signal = -1;
            sem_post(&sem_ping);
            
            sem_wait(&sem_send);
            send(sock_server, buffer, MENS_SIZE, 0);
            sem_post(&sem_send);

            int ping_ret;

            ping_ret = wait_ping();

            sem_wait(&sem_ping);
            ping_signal = 0;
            sem_post(&sem_ping);

            if (ping_ret == 0)
            {
                return 0;
            }

            else
            {
                printf("Servidor não repondeu ao ping.\n");
                return 5;
            }

            return 0;

        }
        
    }

    else
    {
        printf("comando desconhecido.\n");
        return 3;
    }
    
    

    return 0;

}

int main(int argc, char *argv[]){

    char buffer[4096] = {0};
    void *thread_ret;
    int ret_command;

    /*inicia os semaforos do programa*/
    sem_init(&sem_ack,0,1);
    sem_init(&sem_send,0,1);
    sem_init(&sem_ping,0,1);
    sem_init(&sem_connect_fail,0,1);

    /*modifica o tratamento do sinal SIGING*/
    signal(SIGINT, stop_client);

    printf("Bem-vindo ao IRC!!!\n");

    /*loop principal do programa*/
    while (1)
    {
        /*verifica o numero de falhas que o programa teve ao se conectar com o servidor*/
        if (num_connect_fail >= 3)
        {
            printf("O servidor não respondeu a 3 mensagens.\n");
            disconect_erro();
            sem_wait(&sem_connect_fail);
            num_connect_fail = 0;
            sem_post(&sem_connect_fail);
        }
        
        /*le entrada do usuario*/
        int input_size;
        input_size = scanf(" %4096[^\n]", buffer);

        if (input_size <= 0)
        {
            printf("Encerrando cliente...\n");

            if (sock_server >= 0)
            {
                disconect();
            }
            
            break;
        }
        

        /*verifica se e um comando*/
        if (buffer[0] == '/')
        {
            ret_command = command(buffer);

            if (ret_command == 1)
            {
                break;
            }
            
        }
        
        /*verifica se existe uma conexao*/
        else if (sock_server != -1)
        {
            pthread_create(&thread[0], NULL, send_mensage, (void*)(buffer));
            pthread_join(thread[0],  &thread_ret);
        }

        /*nao esta conectado e nao e um comando*/
        else
        {
            printf("Nao esta conectado a nenhum servidor.\n");
            printf("Para se conectar digite: /connect\n");
        }

    }

    return 0;
}