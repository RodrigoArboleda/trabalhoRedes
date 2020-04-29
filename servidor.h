#ifndef SERVIDOR
#define SERVIDOR
#include "utilities.h"
#include "menssage.h"

/*Esta função chama a connect_server para estabeler uma conexao com um cliente,
e estabelece uma troca de mensagens com o cliente atraves de duas threads
@RETORNO
    int - sempre 0*/
int server();

#endif