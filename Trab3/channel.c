#include "channel.h"

CHANNEL* create_channel(char*name,char*admin_name){
    CHANNEL* channel = (CHANNEL*)malloc(sizeof(CHANNEL));
    sem_init(&(channel->sem_lista_clientes),0,1);
    channel->lista_clientes = creat_list();
    strcpy(channel->admin_name, admin_name);
    channel->num_cliente = 0;
    strcpy(channel->name,name);

    channel->num_clientes_ja_conectados = 0;
    return channel;
}

void delete_channel(CHANNEL*channel){

    delete_list(channel->lista_clientes);
    sem_destroy(&(channel->sem_lista_clientes));
    free(channel);
    channel = NULL;
}
  
int channel_insert_client(CHANNEL*channel,CLIENT*cli){
    if(channel == NULL || cli == NULL)
        return -1;
    
    sem_wait( &(channel->sem_lista_clientes) );
    sem_wait( &(cli->sem_nickname) );
    CLIENT *client_in_list = list_get_client_by_name(channel->lista_clientes, cli->nickname);

    if(client_in_list == NULL){
        insert_list(channel->lista_clientes, cli);
        channel->num_cliente++;
        channel->num_clientes_ja_conectados++;
    }   
    /*incrementando o numero de usuarios conectados*/
    sem_post( &(cli->sem_nickname) );
    sem_post( &(channel->sem_lista_clientes) );

    if(client_in_list == NULL){
        return 0;
    }
    return -2;
}

CLIENT* channel_remove_client(CHANNEL*channel,char *cli_nickname){
    if(channel == NULL || cli_nickname == NULL)
        return NULL;

    sem_wait( &(channel->sem_lista_clientes) );
    CLIENT * cli = remove_list(channel->lista_clientes, cli_nickname);
    /*Abaixando o número de clientes conectados*/
    if(cli != NULL){
        channel->num_cliente--;
        cli->channel = NULL;
    }   
        
    sem_post( &(channel->sem_lista_clientes) );
    return cli;
}



LIST_CHANNEL* create_list_channel(){

    LIST_CHANNEL* list_temp  = (LIST_CHANNEL*)malloc(sizeof(LIST_CHANNEL));

    if (list_temp == NULL)
    {
        return NULL;
    }
    

    list_temp->ini = NULL;
    list_temp->size = 0;

    return list_temp;
}

/*insere ordenado pelo nome do canal, 
nao se insere se ja existir um canal com este nome*/
int list_insert_channel(LIST_CHANNEL*l,CHANNEL*channel){

    if(channel == NULL)
        return -1;
    
    /*casos onde se insere no inicio da lista*/

    if(l->size == 0 || strcmp(channel->name,l->ini->channel->name) < 0){

        NO*new = (NO*)malloc(sizeof(NO));
        new->channel = channel;
        new->next = l->ini;
        l->ini = new;
        l->size = l->size + 1;
        return 0;
    }

    NO* ant = l->ini;
    NO* no = ant->next;

    while(no != NULL){
        int cmp = strcmp(channel->name,no->channel->name);

        if(cmp == 0){
            /*Ja existe um canal com este nome*/
            return -2;
        }

        if(cmp < 0){
            NO*new = (NO*)malloc(sizeof(NO));
            new->channel = channel;
            new->next = no;
            ant->next = new;
            l->size = l->size + 1;
            return 0;
        }

        ant = no;
        no = no->next;        
    }

    /*se chegou ate aqui, significa que tem que inserir no final da lista*/
    NO*new = (NO*)malloc(sizeof(NO));
    new->channel = channel;
    new->next = NULL;
    ant->next = new;
    l->size = l->size + 1;
    return 0;
}

CHANNEL* list_remove_channel(LIST_CHANNEL* list_d, char* name){

    if (list_d == NULL)
    {
        return NULL;
    }

    if (list_d->ini == NULL)
    {
        return NULL;
    }

    NO* next_element = NULL;
    NO* now_element = NULL;
    NO* previous_element = NULL;


    CHANNEL* client_ret = NULL;

    now_element = list_d->ini;

    while (now_element != NULL)
    {
        next_element = now_element->next;
        
        if(strcmp(now_element->channel->name, name) == 0){
            client_ret = now_element->channel;
            free(now_element);
            /*caso seja o primeiro elemento a se removido*/
            if (previous_element == NULL)
            {
                list_d->size = list_d->size -1;
                list_d->ini = next_element;
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

int list_channel_delete(LIST_CHANNEL*list_d){
    if(list_d == NULL){
        return -6;
    }

    if (list_d->ini == NULL)
    {
        free(list_d);
        return 0;
    }
    
    NO* next_element = NULL;

    NO* now_element = NULL;

    now_element = list_d->ini;

    while (now_element != NULL)
    {
        next_element = now_element->next;
        free(now_element);
        now_element = next_element;
    }

    free(list_d);

    return 0;
}

CHANNEL *list_get_channel_by_name(LIST_CHANNEL*l,char*name){

    if(l == NULL || name == NULL)
        return NULL;
    
    NO*no = l->ini;

    while(no != NULL){
        if(strcmp(no->channel->name,name) == 0)
            return no->channel;
        
        no->next;
    }

    return NULL;

}