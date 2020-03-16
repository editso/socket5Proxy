#ifndef __SERVER_H__
#define __SERVER_H__

#include <sys/socket.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

/**
 * 连接方法
*/
#define METHOD 0x00 // 无

/**
 * 状态消息
*/
#define REP0 0x00 // 成功
#define REP1 0x01 // 失败

/**
 * 连接地址类型
*/
#define ATYP1 0x01 // ipv4
#define ATYP2 0x04 // ipv6
#define ATYP3 0x03 // 域名


/**
 * 客户端连接协议 简单
*/
struct request_protocol{
    char method; // 请求方式
};

/**
 * 请求方式
*/
struct response_protocol{
    /**
     * 服务端回应
     * 0x00 成功
     * 0x01 失败
    */
    char rep; 
};

struct request_data{
    char user[20]; // 请求携带的帐号
    char passwd[255]; // 密码
    char atyp; // 地址类型 ipv4 ipv6 域名
    char addr[32]; //地址
    char port[4]; // 端口
};


struct response_data{
    char rep; // 回应消息 0x00成功 0x01失败 
};

/**
 * 服务端连接地址
*/
struct conn_addr{
    /**
     * 主机地址
    */
    char host[64];

    /**
     * 端口号
    */
    uint16_t port;
};


typedef struct{
    int remote_sock_fd;
    int source_sock_fd;
}proxy;


/**
 * 客户端连接到服务端
*/
extern int conn(struct conn_addr *addr);


/**
 * 连接到代理
*/
extern int proxy_connect(struct conn_addr *addr);


/**
 * 连接认证
*/
extern int server_licenes(int sockfd);

/**
 * 客户端练级认证
*/
extern int client_licenes(int sockfd, struct request_data* data);


/**
 *  创建一个socket
*/
extern int create_server_sock5(char *port);

/**
 * 服务端监听
*/
extern int server_listen(int sockfd, int count, int (*handler)(proxy, struct sockaddr_in));

/**
 * 客户端认证
*/
extern int proxy_client_licenes(int sockfd);

/**
 * 服务端认证
*/
extern int proxy_server_licenes(int sockfd);

/**
 * 代理发送数据
*/
extern int proxy_send(int sockfd);

/**
 * 代理接收数据
*/
extern int proxy_recv(int sockfd);


/**
 * 数据转发
*/
extern int proxy_forward_data(int sockfd);


#endif