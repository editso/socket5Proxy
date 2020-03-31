#include "proxy.h"
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <string.h>

int listen_fd = -1;

void signal_handler(int sig){
    printf("signal: %d\n", sig);
    if(listen_fd != -1){
        if(close(listen_fd) < 0){
            perror("关闭失败");
        }else{
            puts("已关闭");
        }
        exit(0);
    }
}

void* run(void* p_info){
    struct proxy_info* info = (struct proxy_info*)p_info;
    printf("连接%s:%d#%d\n", info->addr, info->port, info->fd);
    int remote_fd;
    // 连接认证
    if(proxy_server_licenes(info->fd) < 0){
        puts("认证不通过");
    }else if((remote_fd = proxy_sock5_licenes(info->fd)) < 0){
        puts("sock5 认证失败");
    }else{
        // 转发数据
        proxy_forward_data(info->fd, remote_fd);
    }
    printf("会话结束: %ld\n", pthread_self());
    close(info->fd);
    close(remote_fd);
    free(p_info);
}

int main(int argc, char *argv[]){
    if(argc < 2){
        printf("%s:请指定一个端口号\n", argv[0]);
        exit(0);
    }
    int signals[] = {SIGSEGV, SIGTTOU, SIGQUIT, SIGINT, SIGTERM, SIGABRT, SIGIO, SIGPIPE, SIGURG};
    if(signal_register(signals, sizeof(signals) / sizeof(int), signal_handler) < 0){
        perror("信号注册失败");
        exit(0);
    }
    struct proxy_info s_info;
    int opt = 1;
    if((listen_fd = create_reuse_tcp(argv[1], (void*)&opt, sizeof(opt), 100,&s_info)) < 0){
        perror("创建失败");
        exit(0);
    }

    struct sockaddr_in addr;
    socklen_t socklen = sizeof(addr);
    int conn_fd;
    pthread_t pid;
    struct proxy_info info;
    struct proxy_info* p_info;
    while (1)
    {
        printf("等待连接: %s:%d#%d\n", s_info.addr, s_info.port, s_info.fd);
        if((conn_fd = accept(listen_fd, (struct sockaddr*)&addr, &socklen)) < 0){
            perror("accept error");
            continue;
        }
        proxy_ntos(&addr, &info);
        info.fd = conn_fd;
        p_info = (struct proxy_info*)memcpy(malloc(sizeof(struct proxy_info)), &info, sizeof(info));
        if(pthread_create(&pid, NULL, run, (void*)p_info) != 0){
            perror("创建线程失败");
            free(p_info);
        }

    }
    
}