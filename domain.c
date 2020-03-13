// #include "proxy.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "proxy.h"

int main(int argc, char **argv){
    if(argc < 2){
        puts("请输入域名");
        exit(0);
    }
    struct hostent host;
    printf("domain: %s\n", argv[1]);
    if(parser_domain(argv[1] ,&host) != 0){
        puts("解析失败");
        exit(0);
    }
    char ip[32] = {0};
   
    inet_ntop(AF_INET, host.h_addr_list[0], ip, 32);
    printf("%s\n", ip);
    
}