#ifndef SERVIDOR2
#define SERVIDOR2
#include "utilities.h"

/*Esta função faz com que a execucação de quem o chamou espere por x*10 nanosegundos
@PARAMETROS
    - int x - tempo que vai ficar parado
*/
void wait(int x);

/*Esta prepara o socket do servidor, associando o endereco e o deixando pronto para os
clientes se conectarem
@RETORNO
    - int - socket aberto para coneccao.
*/
int prepare_server_socket();

/*Esta função faz o processo de desconectar um cliente do servidor
@PARAMETROS
    - int x - tempo que vai ficar parado
*/
void *disconnect_cli(void *cli_);

/*Esta função deve ser iniciada por uma thread, ela vai receber as mensagens que um client enviou e guardado
em char*message da struct RECEIVE_MESSAGE_STRUCT.
Esta funcao sempre eh chamada com exec_n_segundos, pois precisa para de tentar de ler apos um tempo.
Se da timeout na função por causa do recv, pois ele eh bloqueante, e se colocar como nao bloqueante, 
teria que lidar com a perca de sincronismo. 
@PARAMETROS
    - void * param - na verdade eh um RECEIVE_MESSAGE_STRUCT*recv_msg, se passa na struct onde 
sera guardado a mensagem, de que CLIENT vai receber a mensagem.
*/
void *receive_message_aux(void *param);

/*Esta função deve ser iniciada por uma thread, esta funcao tem como objetivo enviar a um 
cliente, uma mensagem que outro client enviou.
@PARAMETROS
    - void * param - na verdade eh um SEND_MESSAGE_STRUCT*param, na struct eh definido 
o nickname de quem enviou, a mensagem, e o CLIENT*cli para quem vai enviar.
*/
void *send_menssage_thread(void *param);

/*Esta funcao manda a mensagem de um cliente para os outros, 
utiliza a send_message_thread para enviar para cada um
@PARAMETROS
    CLIENT* - o client que esta enviando a mensagem
    char *message - a mensagem que ele enviou
*/
void send_message(CLIENT*sender, char*message);

/*Esta funcao vai ser chamada pela thread_listen de cada client,
Esta funcao fica escutando o client para ver se ele enviou alguma mensagem.
@PARAMETROS
    - void *param- na verdade eh um CLIENT*param - o client que vamos escutar
*/
void *receive_message(void *param);

/*Cria uma struct CLIENT
@PARAMETROS
    int sock - socket do client
    struct sockaddr_in addr - endereco do socket do client
    unsigned int size_addr - tamanho do endereco
    char*  nickname - nickname do cliente.
@RETORNO
    CLIENT * - cliente criado.
*/
CLIENT *create_client(int sock,struct sockaddr_in addr,unsigned int size_addr, char*nickname);

/*Esta funcao eh a thread que fica conectando o servidor com os clientes que estao tentando
se conectar.
@PARAMETROS
    void * param - na verdade eh um int*param, eh um ponteiro para o socket aberto do servidor.
*/
void* conecta_cliente(void *param);


/*Esta funcao desliga o servidor, necessario cancelar que novos clientes se conectem.
@ARAMETROS
    int sock_task - socket aberto do servidor.
*/
void shutdown_server(int sock_task);


#endif