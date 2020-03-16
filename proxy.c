#include <string.h>
#include "proxy.h"

#define FORWARD_BUFF_LEN 1024


extern int create_server_socket(const struct sockaddr_in* addr, int type, struct sock_info* info){
    // socket fd > 0 or -1
    int sockfd = 0;
    // 创建socket 连接
    if((sockfd = socket(addr->sin_family, type, 0)) < 0 ){
        return -1;
    }
    // 绑定
    if(bind(sockfd, (struct sockaddr*)addr, sizeof(struct sockaddr)) < 0){
        return -1;
    }
    convert_ntos(addr, info); // 转换为主机字节序
    info->sockfd = sockfd;
    return sockfd;
}

extern void convert_ntos(const struct sockaddr_in* addr, struct sock_info* info){
    // 获取主机字节序
    inet_ntop(AF_INET, &addr->sin_addr, info->host, 64);
    // 获取地址类型
    memcpy(&info->atyp, &addr->sin_family, sizeof(&addr->sin_family));
    // 获取端口号
    info->port = ntohs(addr->sin_port);
}

extern int parser_domain(const char *domain, struct hostent* host){
    struct hostent* __host__ = gethostbyname(domain);
    if(__host__ == NULL)
        return -1;
    memcpy(host, __host__, sizeof(struct hostent));
    return 0;
}

extern int accept_client(const struct sock_info* info, int listen_count,
    void (*handler)(struct sock_info),
    void (*error)()){
    if(info == NULL)
        return -1;
    if(listen(info->sockfd, listen_count) < 0)
        return -1;

    // 最大的fd
    int sockfd = -1;
    struct sock_info c_info; // 连接信息
    struct sockaddr_in c_addr; // 连接信息
    socklen_t socklen = sizeof(c_addr);
    while (1)
    {   
        if((sockfd=accept(info->sockfd, (struct sockaddr *)&c_addr, &socklen)) < 0){
            error();
        }
        convert_ntos(&c_addr, &c_info);
        c_info.sockfd  = sockfd;
        // 通知程序处理
        handler(c_info);
    }
    
}

extern int connect_target(const struct sockaddr_in* addr, struct sock_info* info){
    int sockfd = -1;
    if((sockfd = socket(addr->sin_family, SOCK_STREAM, 0)) < 0){
        return -1;
    }
    printf("连接.......");
    if(connect(sockfd, (struct sockaddr*)addr, sizeof(struct sockaddr)) == -1){
        return -1;
    }
    convert_ntos(addr, info);
    info->sockfd = sockfd;
    return sockfd;
}


extern int sock5_licenes(const struct sock_info* sock, struct sock_info* connect_info){
    // 客户端文件描述符
    int sockfd = sock->sockfd;

    // 缓冲区
    char buffer[20] = {0};

    // 缓冲区长度
    int BUFF_LEN = sizeof(buffer);


    //  接收客户端消息
    if(recv(sockfd, buffer, BUFF_LEN, 0) < 0)
        return -1;
    puts("licenes........1");
    // 客户端连接协议
    struct client_protocol* client = (struct client_protocol*)buffer;
    
    if(client->ver != SOCK5)
        return -2; //  如果不是sock5
    
    // 服务端回应消息
    struct server_protocol server = {SOCK5, METHOD1};
    
    // 重置缓冲
    // bzero(&buffer, BUFF_LEN); 
    // 拷贝到缓冲区
    char sbuf[2] = {0};
    memcpy(sbuf, &server, sizeof(server));
    // 发送给客户端
    if(send(sockfd, sbuf, sizeof(server), 0) < 0)
        return -1;
    puts("licenes........2");
    char request_buffer[270];
    if(recv(sockfd, request_buffer, sizeof(request_buffer), 0) < 0)
        return -1;
    // 客户端请求信息
    puts("licenes........3");
    struct client_request_protocol* request = (struct client_request_protocol*)request_buffer;
    if(request->ver != SOCK5)
        return -1;
    struct sockaddr_in addr;
    if(request->atyp == ATYP1){
        puts("ipv4..........");
        addr.sin_family = AF_INET;
        char* addr_p = &request->atyp + sizeof(request->atyp);
        memcpy(&addr.sin_addr, addr_p, 4);
        memcpy(&addr.sin_port, addr_p + 4,  2);
    }else if(request->atyp == ATYP2 || request->atyp == ATYP3){
        puts("domain..........");
        struct sock_domain domain;
        if (get_sock_domain(request, &domain) != 0)
        {
            printf("域名解析失败: %s:%d\n", domain.domain, ntohs(atol(domain.port)));
            return -1;
        }
        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr,  domain.net_host.h_addr_list[0], sizeof(addr.sin_addr));
        memcpy(&addr.sin_port, &domain.port, 2);
    }else if(request->atyp == ATYP3){
        puts("ipv6..........");
        // ipv6
        return -5;
    }else{
        return -3;
    }
    if(connect_target(&addr, connect_info) < 0){
        puts("目标地址连接失败");
        return -1;
    }
    puts("licenes........4");
    struct server_response_protocol response = {SOCK5, REP0, RSV, ATYP1};
    bzero(&buffer, BUFF_LEN);
    memcpy(buffer, &response, sizeof(response));
    if(send(sockfd, buffer, sizeof(response), 0) < 0)
        return -1;
    return connect_info->sockfd;
}

/**
 * sock5代理
*/
extern int sock5_proxy(const struct sock_info* sock,struct conn_addr* addr, struct sock_info* client){
    int sockfd = sock->sockfd;
    // 缓冲区
    char buff[10] = {0};
    int BUFF_LEN = sizeof(buff);

    if(recv(sockfd, buff, BUFF_LEN, 0) < 0)
        return -1; //  客户端读取失败
    puts("1.........");
    struct client_protocol* client_P = (struct client_protocol*)buff;
    if(client_P->ver != SOCK5)
        return -2; // 协议版本错误
    

    struct server_protocol server_p = {SOCK5, METHOD1};
    // 重置缓冲
    // bzero(&buffer, BUFF_LEN); 
    // 拷贝到缓冲区
    char sbuf[2] = {0};
    memcpy(sbuf, &server_p, sizeof(server_p));
    // 发送给客户端
    if(send(sockfd, sbuf, sizeof(server_p), 0) < 0)
        return -1;
    puts("2..........");
    char req_buff[270] = {0};
    if(recv(sockfd, req_buff, sizeof(req_buff), 0) < 0 )
        return -1; // 接收客户端请求信息失败
    puts("3..........");
    // 客户端请求信息
    struct client_request_protocol* request = (struct client_request_protocol*)req_buff;
    if(request->ver != SOCK5)
        return -1; //  协议版本错误

    struct request_data req_data;
    req_data.atyp = ATYP1;
    if(request->atyp == ATYP1){
        puts("ipv4....");
        inet_ntop(AF_INET, request->addr, req_data.addr,  4);
        char port[2];
        memcpy(port, &request->addr+4, 2);
        int __port__  = ntohs(atol(port));
        memcpy(req_data.port, (void *)&__port__, 2);
        printf("connect: %s:%s\n", req_data.addr, req_data.port);
    }else if(request->atyp == ATYP3){
        puts("domain  parser....");
        struct sock_domain domain;
        if(get_sock_domain(request, &domain) != 0){
            printf("域名解析失败: %s:%s", domain.domain, domain.port);
            return -3; // 域名解析失败
        }
        puts("domain.....");
        inet_ntop(AF_INET, domain.net_host.h_addr_list[0], req_data.addr,  32);
        uint16_t port;
        printf("address>>> %s\n", domain.port);
        memcpy(req_data.port, domain.port, 2);
        memcpy(&port, req_data.port, 2);
        printf("connect: %s:%d\n", req_data.addr, ntohs(port));
    }else{
        return -1; //协议错误 
    }
    puts("Proxy连接......");
    int conn_sockfd = -1;
    if((conn_sockfd = conn(addr)) < 0)
        return -4; // 连接失败
    puts("Proxy认证......");
    if(client_licenes(conn_sockfd, &req_data) < 0)
        return -1; // 认证失败
    
    client->atyp = AF_INET;
    memcpy(client->host, addr->host, sizeof(addr->host));
    memcpy(&client->port, &addr->port, sizeof(addr->port));
    client->sockfd = conn_sockfd;
    puts("client回应.......");
    struct server_response_protocol response = {SOCK5, REP0, RSV, ATYP1};
    bzero(&buff, BUFF_LEN);
    memcpy(buff, &response, sizeof(response));
    if(send(sockfd, buff, sizeof(response), 0) < 0)
        return -1;
    printf("连接成功.....");
    return conn_sockfd;
}


extern int get_sock_domain(const struct client_request_protocol* request, 
                           struct sock_domain* domain){
    if(domain == NULL || request == NULL)
        return -1;
    // 获取域名长度
    unsigned int domain_len = ((unsigned int)*request->addr) + 1;
    for(int i=1; i < domain_len; i++){
        domain->domain[i-1] = *(request->addr+i);
    }
    puts("解析.....");
    // 解析域名
    if(parser_domain(domain->domain, &domain->net_host) != 0){
        puts("解析失败");
        return -2;
    }
    // 获取端口号 网络字节序
    memcpy(domain->port, &request->addr+domain_len, 2);
    puts("解析成功");
    return 0;
}

/**
 * 转发数据程序
*/
extern int __FORWARD_DATA__(int dest_fd, int src_fd, char *buff, int size){
    printf("发送数据\n");
    bzero(buff, sizeof(buff)); // 清空缓冲区
    int data_len = 0;
    if((data_len = recv(src_fd, buff, size, 0)) < 0)
        return -1; // 读取遇到错误
    // 开始转发数据
    if(send(dest_fd, buff, data_len, 0) < 0)
        return -1; // 转发遇到错误
}

/**
 * 数据转发
*/
extern int forward_data(struct sock_info* local, struct sock_info* remote){
    if(local == NULL || remote == NULL)
        return -1;
    // 文件描述符集
    fd_set read_fd_set;
    char buff[FORWARD_BUFF_LEN];

    int local_fd = local->sockfd;
    int remote_fd = remote->sockfd;
    // 获取文件描述符范围
    int max_fd = (local_fd > remote_fd ? local_fd : remote_fd) + 1;

    struct timeval time;
    time.tv_sec = 5;
    time.tv_usec = 0;

    // 将文件描述符置0
    FD_ZERO(&read_fd_set);
    FD_SET(local_fd, &read_fd_set);
    FD_SET(remote_fd, &read_fd_set);
    int fds = -1;
    printf("local: %d, remote: %d, max: %d\n", local_fd, remote_fd, max_fd);

    while ((fds = select(max_fd, &read_fd_set, NULL, NULL, &time)) >= 0)
    {
        // printf("maxfd:%d\n", max_fd);
        if(fds == 0)
            continue;
        if(FD_ISSET(local_fd, &read_fd_set) == local_fd){
            printf("转发数据\n");
            if (__FORWARD_DATA__(remote_fd, local_fd, buff, FORWARD_BUFF_LEN) < 0)
            {
                printf("数据转发失败\n");
                break;
            }
        }else if(FD_ISSET(remote_fd, &read_fd_set) == remote_fd){
            printf("转发数据\n");
            if (__FORWARD_DATA__(local_fd, remote_fd, buff, FORWARD_BUFF_LEN) < 0)
            {
                printf("数据转发失败\n");
                break;
            }
        }
        // 重置描述符集
        FD_ZERO(&read_fd_set);
        FD_SET(local_fd, &read_fd_set);
        FD_SET(remote_fd, &read_fd_set);
    }
    return 0;
}

