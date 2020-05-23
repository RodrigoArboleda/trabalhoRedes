#include "cliente.h"

/*
A váriavel globas a seguir armazena o socket que deve ser fechado caso o programa
seja encerrado de forma assincrona
*/
int sock_open_server = -1;

/*
Esta funcção trata o caso de o programa receber um sinal, fechando o socket
antes de encerrar 
@RETORNO
    void
*/
void stop_client(int sig){
    printf("encerrando conexão...\n");
    if (sock_open_server != -1){
        shutdown(sock_open_server, 1);
    }
    sock_open_server = -1;
    exit(1);
}

/*
Esta função conecta o client a o servidor do endereço IP passado como parametro,
a porta eh a macro PORT_SERVER definida em servidor.h, retornando o socket 
conectado a este servidor
@PARAMETROS
    char *ip - endereço IP do server no formato string(X.X.X.X)
@RETORNO
    int - socket conectado ao servidor
*/
int creat_connect(char* ip){

    int sock_server = -1;

    int cod_error = 0;


    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0){
        error(ERROR_SOCK);
        exit(ERROR_SOCK_COD);
    }

    sockaddr_ip addrserver;
    /*montando a struct do endereço do socket*/
    addrserver.sin_family = AF_INET; /*utilizando TCP*/
    addrserver.sin_addr.s_addr = string_to_byte_ip_adress(ip);
    addrserver.sin_port = invert_endian_16B(PORT_SERVER);


    cod_error = connect(sock_server, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_error != 0)
    {
        error(ERROR_CONNECT);
        exit(ERROR_CONNECT_COD);
    }

    return sock_server;
}

/*Esta função chama a creat_connect para estabeler uma conexao,
e estabelece uma troca de mensagens com o servidor atraves de duas threads
@RETORNO
    int - sempre 0
*/
int client(){

    void *thread_ret1;
    void *thread_ret2;
    char ip[15] = {0};
    /*É passado como parameto 0 o socket e 1 o status da outra thread*/
    int parameter_thread[2];
    parameter_thread[1] = 1;

    signal(SIGINT, stop_client);

    printf("Digite o IP do servidor:\n");
    scanf("%s", ip);

    parameter_thread[0] = creat_connect(ip);

    sock_open_server = parameter_thread[0];

    pthread_t threads[2];

    printf(MSG_CONNECT);
    /*criando as threads, as funções send_menssage e receive_menssage estao definidas em menssage.c*/
    pthread_create (&threads[0], NULL, send_menssage, (void*)(parameter_thread));
    pthread_create (&threads[1], NULL, receive_menssage, (void*)(parameter_thread));

    pthread_join (threads[0], &thread_ret1);
    pthread_join (threads[1], &thread_ret2);

    shutdown(parameter_thread[0], 1);

    sock_open_server = -1;

    return 0;
}
