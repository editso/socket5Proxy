#include "server.h"
#include "proxy.h"
#include <pthread.h>
#include <sys/signal.h>

int sock_fd = -1;

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
       &set, NULL, NULL, &time)) > 0)
   { 
       
       printf("wait.....%d, local_fd:%d, remote_fd: %d, islocal:%d, isremote: %d\n", n, 
        target->sockfd,
        source->sockfd,
        FD_ISSET(target->sockfd, &set), 
        FD_ISSET(source->sockfd, &set));

       memset(buff,  0, sizeof(buff));
        
       if(FD_ISSET(target->sockfd, &set) && (data_len = read(target->sockfd, buff, sizeof(buff))) > 0){
            if(send(source->sockfd, buff, data_len, 0) == -1){
                break;
            }
            printf("写入成功\n%s\n", buff);
            printf("%s:%d\n", source->host, source->port);
       }else if (FD_ISSET(source->sockfd, &set) && (data_len = read(source->sockfd, buff, sizeof(buff))) > 0)
       {
           if(send(target->sockfd, buff, data_len, 0) == -1){
               break;
           }
            printf("%s:%d\n", target->host, target->port);
            printf("%s\n", buff);
       }else if(n > 0 && FD_ISSET(source->sockfd, &set) == source->sockfd && FD_ISSET(target->sockfd, &set) == target->sockfd){
            continue;
       }else{
           break;
       }
       bzero(buff, sizeof(buff));
       FD_ZERO(&set);
       FD_SET(target->sockfd, &set);
       FD_SET(source->sockfd, &set);
   }
   

}


void* run(void* __sock__){
    printf("开始处理:%ld\n", pthread_self());
    proxy* sock = (proxy*)__sock__;
    struct sock_info target;
    struct sock_info source;
    target.sockfd = sock->source_sock_fd;
    // 认证sockt5
    puts("sock5认证....");
    if(sock5_licenes(&target, &source) < 0){
        perror("认证失败");
        close(target.sockfd);
        free(sock);
        return (void*)0;
    }
    puts("认证通过......");
    forward(&target, &source);
    puts("处理完成");
    close(sock->remote_sock_fd);
    close(sock->source_sock_fd);
    free(sock);
}



int handler(proxy __sock__, struct sockaddr_in addr){
    pthread_t pid;
    struct sock_info info;
    convert_ntos(&addr, &info);
    printf("客户端连接>> %s:%d\n", info.host, info.port);
    proxy* sock = malloc(sizeof(proxy)); 
    memcpy(sock, &__sock__, sizeof(proxy));
    if(pthread_create(&pid, NULL, run, (void *)sock) != 0){
        perror("创建失败");
        close(sock->remote_sock_fd);
        close(sock->source_sock_fd);
    }
    return 0;
}


void signal_handler(int singno){
    printf("%d\n", singno);
    if(singno == SIGSEGV){
        puts("段错误");
    }else if(sock_fd != -1){
        close(sock_fd);
        printf("已关闭IO\n");
        exit(1);
    }
}


int main(int argc, char **argv){
    if(argc < 2){
        puts("请指定端口号");
        exit(0);
    }
    if((sock_fd = create_server_sock5(argv[1])) < 0){
        perror("创建服务失败");
        exit(0);
    }

    if(listen(sock_fd, 10) < 0){
        perror("监听失败");
        exit(0);
    }
        // 注册信号
    if(signal(SIGINT, signal_handler) == SIG_ERR){
        perror("信号捕获失败");
    }
    if(signal(SIGQUIT, signal_handler) == SIG_ERR){
        perror("信号捕获失败");
    }
    if(signal(SIGPIPE, SIG_IGN) == SIG_ERR){
        perror("信号捕获失败");
    }
    if(signal(SIGSEGV, signal_handler) == SIG_ERR){
        perror("信号捕获失败");
    }
    struct sockaddr_in addr;
    socklen_t sock_len = sizeof(addr);
    proxy sockfd = {-1,  -1};
    while (1)
    {
        puts("等待连接.....");
        if((sockfd.source_sock_fd = accept(sock_fd, (struct sockaddr*)&addr, &sock_len)) < 0){
            perror("客户端连接 失败");
            exit(0);
        }
        puts("开始认证");
        if(proxy_server_licenes(sockfd.source_sock_fd) < 0){
            puts("认证失败");
            close(sockfd.source_sock_fd);
            continue;
        }
        handler(sockfd,  addr);
    }
    return 0;
}