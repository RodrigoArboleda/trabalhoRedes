#include "cliente.h"
#include "servidor.h"

int main(){

    int menu = 0;

    while (menu != 1 && menu != 2)
    {
        printf("1 - Conectar a um computador.\n2 - esperar conexão.\n");
        scanf("%d", &menu);
        
        if (menu == 1)
        {
            client();
        }

        else if (menu == 2)
        {
            server();
        }
        
        else
        {
            printf("Opcão inválida.");
        }
    }
    

    return 0;
}