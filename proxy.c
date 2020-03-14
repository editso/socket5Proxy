#include <string.h>
#include "proxy.h"

#define MAX_FD 1024

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
    char request_buffer[270];
    if(recv(sockfd, request_buffer, sizeof(request_buffer), 0) < 0)
        return -1;
    // 客户端请求信息
    struct client_request_protocol* request = (struct client_request_protocol*)request_buffer;
    if(request->ver != SOCK5)
        return -1;
    struct sockaddr_in addr;
    if(request->atyp == ATYP1){
        addr.sin_family = AF_INET;
        char* addr_p = &request->atyp + sizeof(request->atyp);
        memcpy(&addr.sin_addr, addr_p, 4);
        memcpy(&addr.sin_port, addr_p + 4,  2);
    }else if(request->atyp == ATYP2){
        struct sock_domain domain;
        if (get_sock_domain(request, &domain) != 0)
        {
            puts("解析域名失败");
            return -1;
        }
        addr.sin_family = AF_INET;
        memcpy(&addr.sin_addr,  domain.net_host.h_addr_list[0], sizeof(addr.sin_addr));
        struct sock_info s_info;
        memcpy(&addr.sin_port, &domain.port, 2);
    }else if(request->atyp == ATYP3){
        // ipv6
        return -5;
    }else{
        return -3;
    }
    if(connect_target(&addr, connect_info) < 0)
        return -1;
    struct server_response_protocol response = {SOCK5, REP0, RSV, ATYP1};
    bzero(&buffer, BUFF_LEN);
    memcpy(buffer, &response, sizeof(response));
    if(send(sockfd, buffer, sizeof(response), 0) < 0)
        return -1;
    return connect_info->sockfd;
}

extern int get_sock_domain(const struct client_request_protocol* request, 
                           struct sock_domain* domain){
    if(domain == NULL)
        return -1;
    // 获取域名长度
    unsigned int domain_len = ((unsigned int)*request->addr) +  1;
    int p_domain_len = strlen(domain->domain);
    for(int i= 1; i < domain_len; i++){
        domain->domain[i-1] = *(request->addr+i);
    }
    // 获取端口号 网络字节序
    memcpy(domain->port, &request->addr+domain_len, 2);
    // 解析域名
    if(parser_domain(domain->domain, &domain->net_host) != 0){
        return -2;
    }
    return 0;
}