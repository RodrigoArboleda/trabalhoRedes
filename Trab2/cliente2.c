#include "utilities.h"
#include <semaphore.h>
#include <unistd.h>
#include <time.h>

sem_t sem_ack;
sem_t sem_send;
sem_t sem_ping;
sem_t sem_connect_fail;
sem_t sem_connect;

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
Armazena se o programa esta tentando se conectar a um servidor
0 - não tentando;
1 - tentnado conectar
*/
int connect_signal = 0;



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
void *creat_connect(void* ip_par){

    char* ip = (char*)ip_par;

    sock_server = -1;

    int cod_error = 0;

    int ret_thread = 0;

    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0){
        error(ERROR_SOCK);
        
        sem_wait(&sem_connect);
        connect_signal = 0;
        sem_post(&sem_connect);
        
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
        ret_thread = ERROR_CONNECT_COD;
        
        sem_wait(&sem_connect);
        connect_signal = 0;
        sem_post(&sem_connect);

        sock_server = -1;

        pthread_exit(&ret_thread);
    }

    sem_wait(&sem_connect);
    connect_signal = 0;
    sem_post(&sem_connect);

    pthread_exit(&ret_thread);
}

int wait_ack(){

    struct timespec time_sleep;

    time_sleep.tv_nsec = 50000000;
    time_sleep.tv_sec = 0;

    if (ack_signal == 0)
    {
        return 1;
    }

    for (int i = 0; i < 10; i++)
    {
        if (ack_signal == 1)
        {
            return 0;
        }

        nanosleep(&time_sleep, NULL);
    }
    
    return 1;

}

int wait_connect(pthread_t connect_thread){

    struct timespec time_sleep;

    time_sleep.tv_nsec = 100000000;
    time_sleep.tv_sec = 0;

    if (connect_signal == 0)
    {
        return 0;
    }

    for (int i = 0; i < 20; i++)
    {
        if (connect_signal == 0)
        {
            return 0;
        }

        nanosleep(&time_sleep, NULL);
    }

    pthread_cancel(connect_thread);
    
    sem_wait(&sem_connect);
    connect_signal = 0;
    sem_post(&sem_connect);
    
    return 1;

}

int wait_ping(){

    struct timespec time_sleep;

    time_sleep.tv_nsec = 25000000;
    time_sleep.tv_sec = 0;

    if (ping_signal == 0)
    {
        return 1;
    }
    

    for (int i = 0; i < 50; i++)
    {
        if (ping_signal == 1)
        {
            return 0;
        }

        nanosleep(&time_sleep, NULL);
    }
    
    return 1;

}

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

    for (int i = 0; i < 5; i++)
    {
        int ack_ret;

        ack_ret = wait_ack();

        if (ack_ret == 0)
        {
            break;
        }

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

void *receive_menssage(){

    int ret_thread = 0;

    int cod_error;
    

    while (sock_server >= 0)
    {
        char buffer[MENS_SIZE] = {0};

        cod_error = recv(sock_server, buffer, MENS_SIZE, 0);
        if (cod_error <= 0)
        {
            error(ERROR_CONNECT);
            ret_thread = ERROR_CONNECT_COD;
            disconect();
            pthread_exit(&ret_thread);
        }

        if (strcmp(buffer, "ACK") == 0)
        {
            sem_wait(&sem_ack);
            if (ack_signal == -1)
            {
                ack_signal = 1;
            }
            sem_post(&sem_ack);
        }

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
        
        else if(strlen(buffer) != 0)
        {
            
            sem_wait(&sem_send);
            char ack[6];
            strcpy(ack,"<ACK>");
            ack[5] = '\0';
            cod_error = send(sock_server, ack, 6, 0);
            sem_post(&sem_send);
            if (cod_error <= 0)
            {
                error(ERROR_CONNECT);
                ret_thread = ERROR_CONNECT_COD;
                disconect();
                pthread_exit(&ret_thread);
            }

            printf("%s\n", buffer);
        }

        
    }

    pthread_exit(&ret_thread);
}

int command(char* buffer){

    
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

            pthread_t thread_connect;
            void *thread_ret;

            sem_wait(&sem_connect);
            connect_signal = 1;
            sem_post(&sem_connect);

            pthread_create(&thread_connect, NULL, creat_connect, (void*)(buffer));
            wait_connect(thread_connect);            

            if(sock_server > 0){
                pthread_create(&thread[1], NULL, receive_menssage, NULL);
                printf("Connectado!\n");
            }   

            else
            {
                printf("Falha ao conectar ao ip %s\n", ip);
            }
                     
            
            return 0;
        }
        
        
    }
    
    else if (strcmp(buffer, "/quit") == 0)
    {
        printf("Encerrando cliente...\n");

        if (sock_server >= 0)
        {
            disconect();
        }

        return 1;
    }

    else if (strcmp(buffer, "/ping") == 0)
    {
        if (sock_server < 0)
        {
            printf("Nenhuma conexao estabelecida, digite: /connect\n");
            return 4;
        }

        else
        {

            clock_t clocks[2];
            void *thread_ret;

            strcpy(buffer, "PING");
            
            sem_wait(&sem_ping);
            ping_signal = -1;
            sem_post(&sem_ping);
            
            clocks[0] = clock();

            sem_wait(&sem_send);
            send(sock_server, buffer, MENS_SIZE, 0);
            sem_post(&sem_send);

            int ping_ret;

            ping_ret = wait_ping();

            clocks[1] = clock();

            sem_wait(&sem_ping);
            ping_signal = 0;
            sem_post(&sem_ping);

            if (ping_ret == 0)
            {
                printf("Tempode de resposta: %.0lfms\n", ((clocks[1] - clocks[0])/(double)CLOCKS_PER_SEC)*1000.0);
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

    sem_init(&sem_ack,0,1);
    sem_init(&sem_send,0,1);
    sem_init(&sem_ping,0,1);
    sem_init(&sem_connect_fail,0,1);
    sem_init(&sem_connect,0,1);

    signal(SIGINT, stop_client);

    printf("Bem-vindo ao IRC!!!\n");

    while (1)
    {
        if (num_connect_fail >= 3)
        {
            printf("O servidor não respondeu a 3 mensagens.\n");
            disconect();
            sem_wait(&sem_connect_fail);
            num_connect_fail = 0;
            sem_post(&sem_connect_fail);
        }
        
        scanf(" %4096[^\n]", buffer);

        if (buffer[0] == '/')
        {
            ret_command = command(buffer);

            if (ret_command == 1)
            {
                break;
            }
            
        }
        
        else if (sock_server != -1)
        {
            pthread_create(&thread[0], NULL, send_mensage, (void*)(buffer));
            pthread_join(thread[0],  &thread_ret);
        }

        else
        {
            printf("Nao esta conectado a nenhum servidor.\n");
            printf("Para se conectar digite: /connect\n");
        }

    }

    return 0;
}