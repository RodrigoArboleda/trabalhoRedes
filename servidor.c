#include "servidor.h"

int connect_server(){

    int sock_task = -1;
    int cod_error = 0;

    sock_task = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_task < 0){
        error(ERROR_SOCK);
        return ERROR_SOCK_COD;
    }

    sockaddr_ip addrserver;

    addrserver.sin_family = AF_INET;
    addrserver.sin_addr.s_addr = INADDR_ANY;
    addrserver.sin_port = htons(PORT_SERVER);

    sockaddr_ip addrclient;

    cod_error = bind(sock_task, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_error != 0)
    {
        error(ERROR_BIND);
        return ERROR_BIND_COD;
    }
    
    cod_error = listen(sock_task, MAX_CLIENT);
    if (cod_error != 0)
    {
        error(ERROR);
        return ERROR_COD;
    }

    int sock_client;
    unsigned int size_client_addr;

    sock_client = accept(sock_task, (struct sockaddr *)&addrclient, &size_client_addr);
    if(sock_client < 0){
        error(ERROR_CONNECT);
        return ERROR_CONNECT_COD;
    }

    shutdown(sock_task, 1);

    return sock_client;
}

int server(){

    void *thread_ret;
    int sock_client;

    printf("Aguardando conexao...\n");

    sock_client = connect_server();    

    pthread_t threads[2];

    printf("Conectado!\n");

    pthread_create (&threads[0], NULL, send_menssage, (void*)(&sock_client));
    pthread_create (&threads[1], NULL, receive_menssage, (void*)(&sock_client));

    pthread_join (threads[0], &thread_ret);
    pthread_join (threads[1], &thread_ret);

    return 0;
}
