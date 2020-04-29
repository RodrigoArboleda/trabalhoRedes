#include "utilities.h"

void error(char* mens){
    printf("%s\n", mens);
    return;
}

int string_to_byte_ip_adress(char *ip_string){
    int bytes[4];
    char*pch;
    int i = 0;
    pch = strtok(ip_string,".");
    
    while(pch != NULL){
        bytes[i] = atoi(pch);
        i++;
        pch = strtok(NULL,".");
    }

    int ip = bytes[0] | (bytes[1]<<8) | (bytes[2]<<16) | (bytes[3]<<24);
    return ip;
}

uint16_t invert_endian_16B(uint16_t x){
    uint16_t new_x;
    new_x = ((x<<8) & 0xff00) | ((x>>8) & 0x00ff);
    return new_x;
}