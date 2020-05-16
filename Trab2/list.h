#ifndef LIST
#define LIST

#include "utilities.h"

typedef struct list_ LIST;
typedef struct list_element_ LIST_ELEMENT;

LIST* creat_list();

int insert_list(LIST* list_d, CLIENT* client_e);

CLIENT* remove_list(LIST* list_d, char* nickname);

int delete_list(LIST* list_d);


#endif