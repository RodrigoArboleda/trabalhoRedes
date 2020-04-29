#include "cliente.h"

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

    void *thread_ret;
    int sock_server;
    char ip[15] = {0};

    printf("Digite o IP do servidor:\n");
    scanf("%s", ip);

    sock_server = creat_connect(ip);

    pthread_t threads[2];

    printf(MSG_CONNECT);
    /*criando as threads, as funções send_menssage e receive_menssage estao definidas em menssage.c*/
    pthread_create (&threads[0], NULL, send_menssage, (void*)(&sock_server));
    pthread_create (&threads[1], NULL, receive_menssage, (void*)(&sock_server));

    pthread_join (threads[0], &thread_ret);
    pthread_join (threads[1], &thread_ret);

    return 0;
}
