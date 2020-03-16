#ifndef __PROXY_H__
#define __PROXY_H__
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <string.h>
#include "protocol.h"
#include "server.h"


struct sock_info{
    /**
     * 主机
    */
    char host[64];

    /**
     * 端口号
    */
    uint16_t port;

    /**
     * socket文件描述符
    */
    int sockfd;

    /**
     * 网络类型
    */
    char atyp;

};

struct sock_domain{
    // 主机
    char domain[255];
    // 端口号
    char port[2];
    // 域名解析信息
    struct hostent net_host;
};

/**
 * 创建一个服务端的socket
 * 返回一个文件描述符号且大于0
*/
extern int create_server_socket(const struct sockaddr_in* addr, int type, struct sock_info* info);


/**
 * 将网络地址信息转换为主机信息
*/
extern void convert_ntos(const struct sockaddr_in *net_addr, struct sock_info* info);

/**
 * 域名解析
*/
extern int parser_domain(const char *domain, struct hostent* host);

/**
 * 数据转发
*/
extern int forward_data(struct sock_info* local, struct sock_info* remote);

/**
 * 监听socket连接
*/
extern int accept_client(const struct sock_info* info, int listen_count, 
    void (*handle)(struct sock_info),
    void (*error)());

/**
 * 连接目标主机
*/
extern int connect_target(const struct sockaddr_in* addr, struct sock_info* info);

/**
 * socket5认证
*/
extern int sock5_licenes(const struct sock_info* sock, struct sock_info* connect_info);

/**
 * sock5代理
*/
extern int sock5_proxy(const struct sock_info* sock, struct conn_addr* addr, struct sock_info* client);



// 获取域名信息与端口号信息
extern int get_sock_domain(const struct client_request_protocol* request, 
                           struct sock_domain* domain);


#endif