#ifndef LISTA
#define LISTA

#include "utilities.h"

typedef struct list_element_ LIST_ELEMENT;

struct list_element_
{
    LIST_ELEMENT* next;
    CLIENT* client_ele;
    
};

typedef struct l{
    int size;
    LIST_ELEMENT* listfirst;
}LIST;


/*
Cria uma lista para armazenar os cliente do servidor
@RETORNO
    LIST* - Retorna o endereco da lista
*/
LIST* creat_list();

/*
Insere um elemento na lista
@PARAMETROS
    LIST* - list_d - o endereco da lista
    CLIENT* - client_e - o endereco do cliente
@RETORNO
    int -   0  - elemento inserido na lista com sucesso
            -5 - lista nao existe
            -6 - elemento nao existe
*/
int insert_list(LIST* list_d, CLIENT* client_e);

/*
Remove um elemento da lista e rotorna seu endereco
@PARAMETRO
    LIST* list_d - endereco da lista
    char* nickname - nome do cliente a ser removido da lista
@RETORNO
    CLIENT* - endereco do cliente
*/
CLIENT* remove_list(LIST* list_d, char* nickname);

/*
Deleta a lista completamente
@PARAMETRO
    LIST* lisd_d - endereco da lista
@RETORNO
    int -   0 - lista deletada
            -6 - lista nao existe
*/
int delete_list(LIST* list_d);


#endif