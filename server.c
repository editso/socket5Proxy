#include <pthread.h>
#include "proxy.h"
#include <string.h>

void forward(struct sock_info *target,  struct sock_info *source){
    /**
     * 数据转发
     * target: 转发目标
     * source: 转发源
    */
   char buff[1024*1024] = {0};
   fd_set set;
   struct timeval time;
   time.tv_sec = 10;
   time.tv_usec = 0;
   int n = 0;
   int data_len = 0;

    FD_ZERO(&set);
    FD_SET(target->sockfd, &set);
    FD_SET(source->sockfd, &set);

   while ((n = select(
       (target->sockfd > source->sockfd?target->sockfd:source->sockfd) + 1, 
       &set, NULL, NULL, &time)) >= 0)
   { 
       if(n == 0){
           continue;
       }
       memset(buff,  0, sizeof(buff));
       if(FD_ISSET(target->sockfd, &set) && (data_len = read(target->sockfd, buff, sizeof(buff))) > 0){
            if(send(source->sockfd, buff, data_len, 0) == -1){
                perror("写入失败");
            }else{
                printf("写入成功\n%s\n", buff);
                printf("%s:%d\n", source->host, source->port);
            }
       }
       if (FD_ISSET(source->sockfd, &set) && (data_len = read(source->sockfd, buff, sizeof(buff))) > 0)
       {
           if(send(target->sockfd, buff, data_len, 0) == -1){
               perror("写入失败");
           }else{
               printf("%s:%d\n", target->host, target->port);
               printf("%s\n", buff);
           }
       }
       FD_ZERO(&set);
       FD_SET(target->sockfd, &set);
       FD_SET(source->sockfd, &set);
   }
   

}

void* start_handle(void *__sock_info__){
    struct sock_info* client_info = (struct sock_info*)__sock_info__;
    struct sock_info connect_info;
    printf("客户端:%s:%d", client_info->host, client_info->port);
    // socket5认证
    if(sock5_licenes(client_info, &connect_info) < 0){
        perror("sockt5 认证失败");
        close(client_info->sockfd);
        return (void*)0;
    }
    // 认证通过
    forward(client_info, &connect_info);

}

void handle(struct sock_info info){
    pthread_t pid;
    struct sock_info *__sock_info__ = (struct sock_info*)memcpy(malloc(sizeof(info)), &info, sizeof(info));
    if(pthread_create(&pid, NULL, start_handle, (void *)__sock_info__) != 0){
        perror("create thread error");
    }
}

void error(){
    puts("连接出错");
}

int main(int argc, char **argv){
    if(argc < 2){
        printf("请提供一个有效的端口号\n");
        exit(0);
    }

    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atol(argv[1]));
    addr.sin_family = AF_INET;
    struct sock_info info;
    if(create_server_socket(&addr, SOCK_STREAM, &info) < 0){
        perror("创建socket失败\n");
        exit(1);
    }

    if(listen(info.sockfd, 10) < 0)
        perror("监听失败");

    // printf("%s:%d\n", info.host, info.port);
    if(accept_client(&info, 100, handle, error) < 0){
        perror("listen client connect error");
    }
    
    perror("error");
    return 0;
}