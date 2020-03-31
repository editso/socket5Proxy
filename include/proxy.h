#ifndef __PROXY_H__
#define __PROXY_H__

#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <arpa/inet.h>
#include <unistd.h>

#define SOCKADDR(addr) ((struct sockaddr*)addr)


#define SOCK5 0x05 // socket5
#define SOCK4 0x04 // socket4

/**
 * 认证方式
*/
#define METHOD0 0x00 // 无需认证
#define METHOD1 0x01 // 通过安全服务程序
#define METHOD2 0x02 // 用户名/密码
#define METHOD3 0x03 // IANA 分配
#define METHOD4 0x08 // 私人方法保留
#define METHOD5 0xFF // 没有可接受的方法


/**
 * 响应状态
*/
#define REP0 0x00 // 成功
#define REP1 0x01 // 失败
#define REP2 0x02 // 不允许连接
#define REP3 0x03 // 网络无法访问
#define REP4 0x04 // 主机无法访问
#define REP5 0x05 // 连接失败
#define REP6 0x06 // 超时
#define REP7 0x07  // 命令不被支持
#define REP8 0x08 // 地址类型不受支持

/**
 * 保留字
*/
#define RSV 0x00


/**
 * 地址类型
*/

#define ATYP1 0x01 // ipv4
#define ATYP2 0x03 // 域名
#define ATYP3 0x04 // ipv6


/**
 * 客户端连接到服务端时发送的消息
*/
struct client_licenes{
    char ver; /**协议版本 SOCK5,SOCK4*/
    char nmethods; /** 客户端支持的认证方式长度*/
    char methods[255]; /** 客户端支持的认证方式*/
};

/**
 * 服务端回应客户端
*/
struct server_licenes{
    char ver; /** 协议版本 SOCK5,SOCK4*/
    char method; /** 认证方式 METHOD0 ~ METHOD5*/
};

/**
 * 客户端连接请求
*/
struct client_request_licenes{
    char ver; /** 协议版本 SOCK5,SOCK4*/
    char cmd; /**连接方式 */
    char rsv; /**保留字 */
    char atyp; /**地址类型*/
    char addr[1]; /** 如果atyp是域名那么地一个字节就是域名的长度*/
    char port[2]; /**连接的端口号*/
};

struct server_response_licenes{
    char ver; /** 协议版本 SOCK5,SOCK4*/
    char rep; /**状态消息 REP0 ~ REP8*/
    char rsv; /**保留字必须为 0x00: RSV*/
    char atyp; /**地址类型*/
    char addr[4]; /*地址*/
    char port[2]; /**端口*/
};

/**
 * 代理信息
*/
struct proxy_info{
    int fd; /** 文件描述符*/
    char addr[32]; /** 地址*/
    uint16_t port; /**端口号 */ 
};


struct domain_info{
    char domain[255]; // 域名
    char h[4]; // 网络字节序
    char n[32]; // 主机字节序
};

/**
 * 域名解析信息
*/
struct proxy_dns_info{
    struct domain_info domain;
    struct hostent host;
};

/**
 * sock5认证
*/
extern int proxy_sock5_licenes(int fd);

/**
 * 代理客户端认证
*/
extern int proxy_client_licenes(int fd);

/**
 * 代理服务端认证
*/
extern int proxy_server_licenes(int fd);


/**
 *  代理连接
*/
extern int proxy_connect(const struct sockaddr_in* addr);


/**
 * 创建一个socket 
*/
extern int create_socket(const struct sockaddr_in* addr, int listen_num);

/**
 * 创建一个可复用端口的tcp连接
*/
extern  int create_reuse_tcp(char *port, void *val, socklen_t len, int listen_num, struct proxy_info* p_info);

/**
 * 将一个网络自己序转换为主机字节序
*/
extern int proxy_ntos(const struct sockaddr_in* addr, struct proxy_info* info);

/**
 * 域名解析
*/
extern int proxy_dns(char *domain, struct proxy_dns_info* dns);

/**
 * 数据转发
*/
extern int forward_data(int dest_fd, int src_fd, char *buff, int size);

/**
 * 数据转发
*/
extern int proxy_forward_data(int s_fd, int t_fd);

/**
 *  信号注册
*/
extern int signal_register(int* sigs, int size, void (*callback)());

/**
 * 创建tcp
*/
extern int create_tcp(char* port, int listen_num, struct proxy_info* p_info);

/**
 * 连接到代理
*/
extern int connect_proxy(const struct proxy_info* proxy);


extern int reuse_port(const int *fd, const void* val, socklen_t* len);
#endif