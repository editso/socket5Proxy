#include <pthread.h>
#include "proxy.h"
#include <string.h>
#include <signal.h>
#include  <errno.h>
#include <server.h>

int sock_fd = -1;


typedef struct {
    struct sock_info sock;
    struct conn_addr addr;
}sock_arg;

// 代理地址
struct conn_addr proxy_addr;

void forward(struct sock_info *target,  struct sock_info *source){
    /**
     * 数据转发
     * target: 转发目标
     * source: 转发源
    */
    puts("开始转发数据............");
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


void* start_handle(void *__arg__){
    sock_arg *arg = (sock_arg *)__arg__;
    struct sock_info* client_info = (struct sock_info*)&arg->sock;
    struct sock_info connect_info;
    printf("客户端:%s:%d\n", client_info->host, client_info->port);
    // socket5认证
    if(sock5_licenes(client_info, &connect_info) < 0){
        close(client_info->sockfd);
        perror("sockt5 认证失败");
        return (void*)0;
    }
    printf("认证通过, 开始转发数据\n");
    // 认证通过
    // if(forward_data(client_info, &connect_info) < 0)
    //     perror("数据转发失败");
    forward(client_info, &connect_info);
    printf("数据转发完成线程结束: %ld\n", pthread_self());
    close(client_info->sockfd);
    close(connect_info.sockfd);
    return (void*)0;
}


/**
 * 开始代理
*/
void* start_proxy(void *__arg__){
    sock_arg *arg = (sock_arg *)__arg__;
    struct sock_info* client_info = (struct sock_info*)&arg->sock;
    struct sock_info connect_info;
    printf("客户端连接: %s:%d#%d\n", client_info->host, client_info->port, client_info->sockfd);

    // if(sock5_proxy(client_info, &arg->addr, &connect_info) < 0){
    //     perror("认证失败");
    //     close(client_info->sockfd);
    //     if(connect_info.sockfd != -1)
    //         close(connect_info.sockfd);
    // }

    puts("连接到代理......");
    if((connect_info.sockfd = proxy_connect(&proxy_addr)) < 0){
        perror("连接到代理失败.....");
        close(client_info->sockfd);
    }
    puts("代理连接成功......");    
    forward(client_info, &connect_info);
    puts("会话结束");
    close(client_info->sockfd);
    close(connect_info.sockfd);
}

void handler(struct sock_info info){
    pthread_t pid;
    sock_arg arg;
    memcpy(&arg.sock,  (struct sock_info*)memcpy(malloc(sizeof(info)), &info, sizeof(info)), sizeof(struct sock_info));
    memcpy(&arg.addr, &proxy_addr, sizeof(struct conn_addr));
    if(pthread_create(&pid, NULL, start_proxy, (void *)&arg) != 0){
        perror("create thread error");
    }
}


void error(){
    puts("连接出错");
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
    if(argc < 4){
        printf("请提供一个有效的端口号 %s [port] [host] [port]\n", argv[0]);
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
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(atol(argv[1]));
    addr.sin_family = AF_INET;
    struct sock_info info;
    if(create_server_socket(&addr, SOCK_STREAM, &info) < 0){
        perror("创建socket失败\n");
        exit(1);
    }
    memcpy(&sock_fd, &info.sockfd, sizeof(int));
    strncpy(proxy_addr.host, argv[2], 32);
    proxy_addr.port = htons(atol(argv[3]));
    // 代理信息
    printf("Proxy: %s:%d\n", proxy_addr.host, htons(proxy_addr.port));
    if(accept_client(&info, 100, handler, error) < 0){
        perror("listen client connect error");
        close(info.sockfd);
    }
    return 0;
}