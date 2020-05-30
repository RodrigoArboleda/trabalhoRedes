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


LIST* creat_list();

int insert_list(LIST* list_d, CLIENT* client_e);

CLIENT* remove_list(LIST* list_d, char* nickname);

int delete_list(LIST* list_d);


#endif