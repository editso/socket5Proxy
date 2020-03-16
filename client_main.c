#include "server.h"


int main(int argc, char **argv){
    if(argc < 3){
        puts("错误的参数host:port");
        exit(0);
    }

    struct conn_addr addr;
    struct request_data data;
    strncpy(addr.host, argv[1], sizeof(argv[1]) + 1);
    strncpy(addr.port, argv[2], sizeof(argv[2]));
    memcpy(data.addr, "101.132.143.80", sizeof("101.132.143.80"));
    memcpy(data.port, "80", sizeof("80"));
    data.atyp = ATYP1;
    int sock_fd = -1;
    if((sock_fd = conn(&addr)) < 0 ){
        perror("连接失败");
        exit(0);
    }
    puts("连接成功, 开始认证....");

    if(client_licenes(sock_fd, &data) <  0){
        perror("认证失败");
        exit(0);
    }
    puts("认证成功");
}