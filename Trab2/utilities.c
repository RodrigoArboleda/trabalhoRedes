#include "utilities.h"

/*
Esta função printa uma mensagem de erro passada como parametro
@PARAMETROS
    - char* mens - mensagem a ser imprimida na saida padrao
@RETORNO
    - void - sem retorno
*/
void error(char* mens){
    printf("%s\n", mens);
    return;
}

/*
Esta função recebe um endereço IP no formato string e o convert para o seu formato em 32bits
@PARAMETROS
    - char *ip_string - endereço IP no formato string
@RETORNO
    - int - endereço IP no formato 32bits
*/
int string_to_byte_ip_adress(char *ip_string){
    int bytes[4];
    char*pch;
    int i = 0;
    /*Separando os valores dos bytes entre os pontos.*/
    pch = strtok(ip_string,".");
    
    while(pch != NULL){
        bytes[i] = atoi(pch);
        i++;
        pch = strtok(NULL,".");
    }
    /*Forma que o ip 32bits é montada*/
    int ip = bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24);
    return ip;
}

/*
Esta função inverte um valor 16bits de little endian para big endian e vice-versa
@PARAMETROS
    - uint16_t x - valor 16 bits de entrada.
@RETORNO
    - uint16_t - valor com a endian mudada.
*/
uint16_t invert_endian_16B(uint16_t x){
    uint16_t new_x;
    new_x = ((x<<8) & 0xff00) | ((x>>8) & 0x00ff);
    return new_x;
}