#include "servidor.h"

/*
A váriavel globas a seguir armazena o socket que deve ser fechado caso o programa
seja encerrado de forma assincrona
*/
int sock_open_client = -1;

void stop_server(int sig){
    printf("encerrando conexão...\n");
    if (sock_open_client != -1){
        shutdown(sock_open_client, 1);
    }
    sock_open_client = -1;
    exit(1);
}

/*
Esta funcção conecta o servidor a um client que tentar se conectar a ele,
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

    sockaddr_ip addrserver;
    /*montando a struct do endereço do socket*/
    addrserver.sin_family = AF_INET;/*utilizando TCP*/
    addrserver.sin_addr.s_addr = INADDR_ANY;
    addrserver.sin_port = invert_endian_16B(PORT_SERVER);

    sockaddr_ip addrclient;
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

    int sock_client;
    unsigned int size_client_addr;
    /*Esperando conecção de um cliente*/
    sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
    if(sock_client < 0){
        error(ERROR_CONNECT);
        exit(ERROR_CONNECT_COD);
    }

    shutdown(sock_task, 1);

    return sock_client;
}

/*Esta função chama a connect_server para estabeler uma conexao com um cliente,
e estabelece uma troca de mensagens com o cliente atraves de duas threads
@RETORNO
    int - sempre 0*/
int server(){

    void *thread_ret;
    int sock_client;

    signal(SIGINT, stop_server);

    printf("Aguardando conexao...\n");

    sock_client = connect_server();    

    sock_open_client= sock_client;

    pthread_t threads[2];

    printf(MSG_CONNECT);
    /*criando as threads, as funções send_menssage e receive_menssage estao definidas em menssage.c*/
    pthread_create (&threads[0], NULL, send_menssage, (void*)(&sock_client));
    pthread_create (&threads[1], NULL, receive_menssage, (void*)(&sock_client));

    pthread_join (threads[0], &thread_ret);
    pthread_join (threads[1], &thread_ret);

    shutdown(sock_client, 1);

    sock_open_client = -1;

    return 0;
}
