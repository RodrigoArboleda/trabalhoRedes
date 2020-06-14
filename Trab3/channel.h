#ifndef CHANNEL_H
#define CHANNEL_H

#include "utilities.h"
#include "list.h"

typedef struct channel_{
    /*nome do canal*/
    char name[50];

    /*lista de clientes do canal*/
    sem_t sem_lista_clientes;
    LIST *lista_clientes;
    int num_cliente;
    /*numeros de cliente que ja conectaram a este canal,
    note que nao eh o numero conectados no momento, so incrementa*/
    int num_clientes_ja_conectados;
    /*Ponteiro para o administrador*/
    char admin_name[50];

    
}CHANNEL;

typedef struct no_ NO;

struct no_{
    CHANNEL*channel;
    NO* next;

};

typedef struct list_channel{

    int size;
    NO* ini;

}LIST_CHANNEL;

CHANNEL* create_channel(char*name,char*admin_name);

void delete_channel(CHANNEL*channel);

int channel_insert_client(CHANNEL*channel,CLIENT*cli);

CLIENT* channel_remove_client(CHANNEL*channel,char *cli_nickname);

LIST_CHANNEL* create_list_channel();

/*insere ordenado pelo nome do canal, 
nao se insere se ja existir um canal com este nome*/
int list_insert_channel(LIST_CHANNEL*l,CHANNEL*channel);

CHANNEL* list_remove_channel(LIST_CHANNEL* list_d, char* name);

int list_channel_delete(LIST_CHANNEL*list_d);

CHANNEL *list_get_channel_by_name(LIST_CHANNEL*l,char*name);

#endif