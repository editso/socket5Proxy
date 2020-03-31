#include <stdlib.h>
#include "proxy.h"
#include <unistd.h>
#include <sys/signal.h>
#include <sys/epoll.h>
#include <pthread.h>
#include <string.h>
#include <stdio.h>

struct proxy_info local;
struct proxy_info proxy;
int listen_fd = -1;

void signal_handler(int sig){
    if(listen_fd != -1){
        if(close(listen_fd) < 0){
            printf("socket关闭失败:%d\n", errno);
        }else{
            puts("已关闭socket");
            exit(0);
        }
    }
}

void* run(void* p_info){
    struct proxy_info* info = (struct proxy_info*)p_info;
    int proxy_fd = -1;
    if((proxy_fd = connect_proxy(&proxy)) < 0){
        perror("连接远程地址失败");
        printf("address: %s:%d\n", proxy.addr, proxy.port);
    }else{
        proxy_forward_data(info->fd, proxy_fd);
    }
    close(info->fd);
    close(proxy_fd);
    free(info);
}


int main(int argc, char **argv){
    if(argc < 4){
        printf("%s: 缺少必须的参数:[bind:[port]] [proxy:[host] [port]]\n", argv[0]);
        exit(0);
    }
    strncpy(proxy.addr, argv[2], 32);
    proxy.port = atol(argv[3]);
    int signals[] = {SIGSEGV, SIGTTOU, SIGQUIT, SIGINT, SIGTERM, SIGABRT, SIGIO, SIGPIPE, SIGURG};
    if(signal_register(signals, sizeof(signals) / sizeof(int), signal_handler) < 0){
        perror("信号注册失败");
        exit(0);
    }
    int opt = 1;
    if((listen_fd = create_reuse_tcp(argv[1], (void*)&opt,  sizeof(opt), 100, &local)) < 0){
        perror("创建失败");
        exit(0);
    }
    struct sockaddr_in conn_addr;
    int conn_fd = -1;
    socklen_t len = sizeof(struct sockaddr);
    pthread_t pid;
    struct proxy_info* p_conn;
    struct proxy_info conn_info;
    while (1)
    {
        printf("等待连接%s:%d#%d\n", local.addr, local.port, local.fd);
        if((conn_fd = accept(listen_fd, (struct sockaddr*)&conn_addr, &len)) < 0){
            perror("通讯失败");
        }
        proxy_ntos(&conn_addr, &conn_info);
        conn_info.fd = conn_fd;
        p_conn = (struct proxy_info*)memcpy(malloc(sizeof(struct proxy_info)), &conn_info, sizeof(conn_info));
        if(pthread_create(&pid, NULL, run, (void*)p_conn) != 0){
            perror("创建线程失败");
            close(conn_fd);
            free(p_conn);
        }
    }
    return 0;
}