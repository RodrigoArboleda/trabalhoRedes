#include "cliente.h"

int creat_connect(char* ip){

    int sock_server = -1;

    int cod_error = 0;


    sock_server = socket(AF_INET, SOCK_STREAM, 0);
    if (sock_server < 0){
        error(ERROR_SOCK);
        return ERROR_SOCK_COD;
    }

    sockaddr_ip addrserver;

    addrserver.sin_family = AF_INET;
    addrserver.sin_addr.s_addr = inet_addr(ip);
    addrserver.sin_port = htons(PORT_SERVER);


    cod_error = connect(sock_server, (struct sockaddr *)&addrserver, sizeof(sockaddr_ip));

    if (cod_error != 0)
    {
        error(ERROR_CONNECT);
        return ERROR_CONNECT_COD;
    }

    return sock_server;
}

int client(){

    void *thread_ret;
    int sock_server;
    char ip[15] = {0};

    printf("Digite o IP do servidor:\n");
    scanf("%s", ip);

    sock_server = creat_connect(ip);

    pthread_t threads[2];

    printf("Conectado!\n");

    pthread_create (&threads[0], NULL, send_menssage, (void*)(&sock_server));
    pthread_create (&threads[1], NULL, receive_menssage, (void*)(&sock_server));

    pthread_join (threads[0], &thread_ret);
    pthread_join (threads[1], &thread_ret);

    return 0;
}
