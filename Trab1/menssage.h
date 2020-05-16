#ifndef MENSSAGE
#define MENSSAGE
#include "utilities.h"

/*Esta função manda para o socket passado como parametros
todas as mensagens escritas na entrada padrão
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
void *send_menssage(void *sock);

/*Esta função escuta indefinidamente a um socket e escreve na saida padrao
o que ouvir do socket
@PARAMETROS
    - void *sock - endereço da variavel onde esta o socket
@RETORNO
    - void - sem retorno
*/
void *receive_menssage(void *sock);

#endif