#include "list.h"

/*
Cria uma lista para armazenar os cliente do servidor
@RETORNO
    LIST* - Retorna o endereco da lista
*/
LIST* creat_list(){
    
    LIST* list_temp = NULL;
    list_temp = (LIST*)malloc(sizeof(LIST));

    if (list_temp == NULL)
    {
        return NULL;
    }
    

    list_temp->listfirst = NULL;
    list_temp->size = 0;

    return list_temp;
}

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
int insert_list(LIST* list_d, CLIENT* client_e){
    
    if (list_d == NULL)
    {
        return -5;
    }
    

    LIST_ELEMENT* next_element = NULL;

    LIST_ELEMENT* now_element = NULL;
    
    now_element = list_d->listfirst;
    
    /*verifica se a lista esta vazia*/
    if (now_element == NULL)
    {
        LIST_ELEMENT* new_list_element = NULL;
        new_list_element = (LIST_ELEMENT*)malloc(sizeof(LIST_ELEMENT));

        if (new_list_element == NULL)
        {
            return -6;
        }
        

        new_list_element->next = NULL;
        new_list_element->client_ele = client_e;

        list_d->listfirst = new_list_element;
        list_d->size = 1;
    }
    
    /*caso a lista nao esteja vazia*/
    else
    {
        next_element = now_element->next;

        while (next_element != NULL)
        {
            now_element = next_element;
            next_element = now_element->next;
        }

        LIST_ELEMENT* new_list_element = NULL;
        new_list_element = (LIST_ELEMENT*)malloc(sizeof(LIST_ELEMENT));

        if (new_list_element == NULL)
        {
            return -6;
        }

        new_list_element->next = NULL;
        new_list_element->client_ele = client_e;

        now_element->next = new_list_element;

        list_d->size = list_d->size + 1;
        
    }
    
    return 0;
}

/*
Remove um elemento da lista e rotorna seu endereco
@PARAMETRO
    LIST* list_d - endereco da lista
    char* nickname - nome do cliente a ser removido da lista
@RETORNO
    CLIENT* - endereco do cliente
*/
CLIENT* remove_list(LIST* list_d, char* nickname){

    if (list_d == NULL)
    {
        return NULL;
    }

    if (list_d->listfirst == NULL)
    {
        return NULL;
    }

    LIST_ELEMENT* next_element = NULL;
    LIST_ELEMENT* now_element = NULL;
    LIST_ELEMENT* previous_element = NULL;


    CLIENT* client_ret = NULL;

    now_element = list_d->listfirst;

    while (now_element != NULL)
    {
        next_element = now_element->next;
        
        if(strcmp(now_element->client_ele->nickname, nickname) == 0){
            client_ret = now_element->client_ele;
            free(now_element);
            /*caso seja o primeiro elemento a se removido*/
            if (previous_element == NULL)
            {
                list_d->size = list_d->size -1;
                list_d->listfirst = next_element;
            }

            else
            {
                previous_element->next = next_element;
                list_d->size = list_d->size - 1;
            }

        }
        previous_element = now_element;
        now_element = next_element;
    }
    
    return client_ret;
    
}

/*
Deleta a lista completamente
@PARAMETRO
    LIST* lisd_d - endereco da lista
@RETORNO
    int -   0 - lista deletada
            -6 - lista nao existe
*/
int delete_list(LIST* list_d){
    
    if(list_d == NULL){
        return -6;
    }

    if (list_d->listfirst == NULL)
    {
        free(list_d);
        return 0;
    }
    
    LIST_ELEMENT* next_element = NULL;

    LIST_ELEMENT* now_element = NULL;

    now_element = list_d->listfirst;

    while (now_element != NULL)
    {
        next_element = now_element->next;
        free(now_element);
        now_element = next_element;
    }

    free(list_d);

    return 0;

}

CLIENT* list_get_client_by_name(LIST* l, char*cli_nickname){

    if(l == NULL || cli_nickname == NULL)
        return NULL;
    
    LIST_ELEMENT*no = l->listfirst;

    while(no != NULL){
        if(strcmp(no->client_ele->nickname,cli_nickname) == 0)
            return no->client_ele;
        
        no->next;
    }

    return NULL;
}