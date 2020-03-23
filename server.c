/**
 * 服务端 Proxy
*/

#include "server.h"
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/epoll.h>




/**
 * 客户端连接到服务端
*/
extern int conn(struct conn_addr *conn_addr){
    if(conn_addr == NULL)
        return -1;
    printf("port:  %d\n", conn_addr->port);
    printf("connect>>> %s:%d\n", conn_addr->host, ntohs(conn_addr->port));
    int sockfd = -1;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = inet_addr(conn_addr->host);
    addr.sin_port = conn_addr->port;
    if(connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0)
        return -1;
    return sockfd;
}


/**
 * 服务端对与客户端的认证
*/
extern int server_licenes(int sockfd){
    char buff[20] = {0};
    int BUFF_LEN = sizeof(buff);
    
    printf("1........\n");
    if(recv(sockfd, buff, BUFF_LEN, 0) < 0){
        return -1; // 接收失败
    }
    struct request_protocol* request = (struct request_protocol*)buff;
    struct response_protocol response;
    if(request->method != METHOD){
        response.rep = REP1; // 失败
        // 协议错误
        bzero(buff, BUFF_LEN);
        memcpy(buff, &response, sizeof(response));
        if(send(sockfd, buff, BUFF_LEN, 0) < 0)
            return -1; // 回应客户端失败
        return  1;
    }
    printf("2........\n");
    bzero(buff, BUFF_LEN);
    response.rep = REP0;
    memcpy(buff, &response, sizeof(response));
    if(send(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1; // 回应客户端失败
    
    char req_buff[1024] = {0};
    if(recv(sockfd, req_buff, sizeof(struct request_data), 0) < 0)
        return -1; //读取失败
    printf("3........\n");
    struct request_data* req_data = (struct request_data*)req_buff;
    if(req_data->atyp != ATYP1){
        puts("协议错误");
        return -1;
    }
    // 连接远程地址
    printf("remote address: %s:%s\n", req_data->addr, req_data->port);
    struct conn_addr conn_addr;
    memcpy(conn_addr.host, req_data->addr, 32);
    memcpy(&conn_addr.port, req_data->port, sizeof(req_data->port));
    int conn_sock_fd = -1;
    if((conn_sock_fd = conn(&conn_addr)) < 0){
        puts("远程地址连接失败");
        return -1; // 远程地址连接失败 
    }
    puts("远程地址连接成功");
    struct response_data res_data;
    res_data.rep = REP0;
    bzero(buff, BUFF_LEN);
    memcpy(buff, &res_data, sizeof(res_data));
    if(send(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1;
    return conn_sock_fd;
}


/**
 * 客户端连接认证
*/
extern int client_licenes(int sockfd, struct request_data* data){
    char buff[20] = {0};
    int BUFF_LEN = sizeof(buff);
    struct request_protocol request = {METHOD};
    memcpy(buff, &request, BUFF_LEN);
    if(send(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1;
    bzero(buff, BUFF_LEN);
    if(recv(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1;
    struct response_protocol* response = (struct response_protocol*)buff;
    if(response->rep != REP0)
        return -1;
    char req_data[sizeof(struct request_data)] = {0};
    memcpy(req_data, data, sizeof(struct request_data));
    if(send(sockfd, req_data, sizeof(struct request_data), 0) < 0)
        return -1;
    bzero(buff, BUFF_LEN);
    if(recv(sockfd, buff, BUFF_LEN, 0) < 0){
        puts("接受服务端信息失败");
        return -1;
    }
    struct response_data* response_data = (struct response_data*)buff;
    if(response_data->rep != REP0){
        printf("认证失败\n=======");
        return -1;
    }
    printf(">..........,%d\n", sockfd);
    return sockfd;
}


/**
 * 代理认证
*/
extern int proxy_client_licenes(int sockfd){
    char buff[20] = {0};
    int BUFF_LEN = sizeof(buff);
    struct request_protocol request = {METHOD};
    puts("licenes.......1");
    memcpy(buff, &request, BUFF_LEN);
    if(send(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1;
    bzero(buff, BUFF_LEN);
    if(recv(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1;
    puts("licenes.......2");
    struct response_protocol* response = (struct response_protocol*)buff;
    if(response->rep != REP0)
        return -1;
    // 认证成功
    return 1;
}


/**
 * 服务端认证
*/
extern int proxy_server_licenes(int sockfd){
    char buff[20] = {0};
    int BUFF_LEN = sizeof(buff);
    
    printf("1........\n");
    if(recv(sockfd, buff, BUFF_LEN, 0) < 0){
        return -1; // 接收失败
    }
    struct request_protocol* request = (struct request_protocol*)buff;
    struct response_protocol response;
    if(request->method != METHOD){
        response.rep = REP1; // 失败
        // 协议错误
        bzero(buff, BUFF_LEN);
        memcpy(buff, &response, sizeof(response));
        if(send(sockfd, buff, BUFF_LEN, 0) < 0)
            return -1; // 回应客户端失败
        return  1;
    }
    printf("2........\n");
    bzero(buff, BUFF_LEN);
    response.rep = REP0;
    memcpy(buff, &response, sizeof(response));
    if(send(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1; // 回应客户端失败
    // 认证通过
    return sockfd;
}

/**
 *  创建一个socket
*/
extern int create_server_sock5(char *port){
    int sockfd = -1;
    if((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1; // 创建失败
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(atol(port)); 
    if(bind(sockfd, (struct sockaddr*)&addr, sizeof(struct sockaddr)) < 0)
        return -1;
    return sockfd;
}

/**
 * 服务端监听
*/
extern int server_listen(int sockfd, int count, int (*handler)(proxy, struct sockaddr_in)){
    if(listen(sockfd, count) < 0)
        return -1; // 监听失败

    proxy socks[1024] = {{-1, -1}};

    fd_set read_set;
    FD_ZERO(&read_set);
    FD_SET(sockfd, &read_set);

    int max_fd = sockfd + 1;
    int fds = -1;
    int accept_sock_fd = -1;
    int remote_sock_fd = -1;
    struct sockaddr_in addr;
    socklen_t sock_len = sizeof(struct sockaddr);
    while (1)
    {
        fds = select(max_fd, &read_set, NULL, NULL, NULL);
        if(fds == -1)
            break;
        if(fds == 0)
            continue;
        if(FD_ISSET(sockfd, &read_set) == sockfd){
            if(accept_sock_fd = accept(sockfd, (struct sockaddr*)&addr, &sock_len) < 0){
                perror("客户端连接失败");
                continue;
            }
            if(accept_sock_fd >= max_fd){
                max_fd = accept_sock_fd + 1;
            }
            puts("客户连入");
            if((remote_sock_fd = server_licenes(accept_sock_fd)) < 0){
                // 认证失败
                puts("认证不通过");
                close(accept_sock_fd);
                continue;
            }

            for(int i = 0; i < 1024; i++){
                proxy* sock = &socks[i];
                if(socks->source_sock_fd == -1){
                    socks->source_sock_fd = accept_sock_fd;
                    socks->remote_sock_fd = remote_sock_fd;
                }
            }

        }else{
            for(int i = 0; i < 1024; i++){
                proxy *sock = &socks[i];
                if(FD_ISSET(fds, &read_set) == sock->remote_sock_fd){
                    if(handler(*sock, addr) < 0){
                        for(int i = 0; i < 1024; i++){
                            if(sock->remote_sock_fd == socks[i].source_sock_fd){
                                proxy __sock__ = {-1, -1};
                                socks[i] = __sock__;
                                break;
                            }
                        }
                    }
                }
            }

        }

        FD_ZERO(&read_set);
        for(int i = 0; i < 1024; i++){
            proxy *sock = &socks[i];
            if(sock->remote_sock_fd == -1)
                continue;
            FD_SET(sock->source_sock_fd, &read_set);
        }

    }
    

}

/**
 * 连接到代理
*/
extern int proxy_connect(struct conn_addr *addr){
    int sockfd = -1;
    if((sockfd = conn(addr))  < 0)
        return -1; //连接失败
    if(proxy_client_licenes(sockfd) < 0)
        return -1; //认证失败
    return sockfd;
}


/**
 * 代理发送数据
*/
extern int proxy_send(int sockfd){
    /**
     * 发送数据包
    */
   
}


/**
 * 代理接收数据
*/
extern int proxy_recv(int sockfd){

}


/**
 * 数据转发
*/
extern int proxy_forward_data(int sockfd){
    int socks[1024] = {-1};
    int maxfd = sockfd;
    // 文件描述符集
    fd_set sock_set;
    FD_ZERO(&sock_set);
    FD_SET(sockfd, &sock_set);
    socks[0] = sockfd;
    struct timeval time; // 设置超时
    time.tv_sec = 5;
    time.tv_usec = 0;
    int sock_num = 0; // 准备好的数量
    while ((sock_num = select(maxfd + 1, &sock_set, NULL, NULL, &time)) >= 0)
    {   
        if(sock_num == 0)
            continue;
        if(FD_ISSET(sockfd, &sock_set) == sockfd){
            // 首次接收
            

        }


    }
    




}