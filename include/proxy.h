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

/**
 * 参考: 
 *  https://www.ietf.org/rfc/rfc1928.txt
 *  https://blog.csdn.net/lmory233/article/details/80219581
*/


/**
 * 协议版本
*/
#define SOCK5 0x05  // socket5
#define SOCK4 0x04 // socket4

/**
 * 认证方式
*/
#define METHOD1 0x00 // 无需认证
#define METHOD2 0x01 // 通用安全服务应用程序
#define METHOD3 0x02 // 用户名/密码
#define METHOD4 0x03 // to X'7F' IANA ASSIGNED
#define METHOD8 0x80 // to X'FE' RESERVED FOR PRIVATE METHODS
#define METHODF 0xFF // 没有可接受的方法

/**
 * 连接类型
*/
#define CMD1 0x01 // CONNECT
#define CMD2 0x02 // BIND
#define CMD3 0x03 // UDP

/**
 * 地址类型
*/
#define ATYP1 0x01 // IPV4
#define ATYP2 0x03 // DOMAINNAME(域名)
#define ATYP3 0x04 // IPV6

/**
 * 服务端应答客户端
*/
#define REP0 0x00 // 成功
#define REP1 0x01 // 一般性失败
#define REP2 0x02 // 规则不允许转发
#define REP3 0x03 // 网络不可达
#define REP4 0x04 // 主机不可达
#define REP5 0x05 // 连接拒绝
#define REP6 0x06 // TTL超时
#define REP7 0x07 // 不支持请求包含CMD
#define REP8 0x08 // 不支持请求中的atyp
#define REP9 0x09 // 未定义

/**
 * 保留位
*/
#define RSV 0x00 // 必须为0x00

struct client_protocol{
    /**
     * 客户端连接到服务端发送的版本标识
    */


    /**
     * 协议版本
     * socket5: 0x05
     * socket4: 0x04
    */
    char ver;

    /**
     * 客户端支持的验证方式长度
    */
    char nmethods;

    /**
     * 客户端支持的认证方式
     * 0x00: 无需认证
     * 0x01:  通用安全服务应用程序
     * 0x02: 用户名/密码
     * 0x03: to X'7F' IANA ASSIGNED 
     * 0x80: to X'FE' RESERVED FOR PRIVATE METHODS
     * 0xFF: 没有可接受的方法 
    */
    char methods[255];

};

struct server_protocol{
    /**
     * 服务器从客户端报文中选择一种方式
    */

    /**
     * 协议版本
     * socket5: 0x05
     * socket4: 0x04
    */
    char ver;

    /**
     * 0x00: 无需认证
     * 0x01:  通用安全服务应用程序
     * 0x02: 用户名/密码
     * 0x03: to X'7F' IANA ASSIGNED 
     * 0x80: to X'FE' RESERVED FOR PRIVATE METHODS
     * 0xFF: 没有可接受的方法 
    */
    char method;
};

struct client_request_protocol{
    /**
     * 客户端向服务器发送连接目的服务器的请求报文
    */

    /**
     * 协议版本
     * socket5: 0x05
     * socket4: 0x04
    */
    char ver;

    /**
     * 连接类型
     * CONNECT: 0x01
     * BIND: 0x02
     * UDP: 0x03
    */
    char cmd;

    /**
     * 保留位
    */
    char rsv;

    /**
     * 地址类型
     * IPV4: 0x01
     * DOMAINNAME(域名): 0x03
     * IPV6: 0x04
    */
    char atyp;

    /**
     * 地址 与 端口号
     * 地址字段中的第一字节是以字节为单位的该域名的长度，没有结尾的NUL字
     * 
    */
    char addr[1];
};

struct server_response_protocol{
    /**
     * 服务端回应客户端的连接
    */

    /**
     * 协议版本
     * socket5: 0x05
     * socket4: 0x04
    */
   char ver;

   /**
    * 应答结果
    * 0x00: 成功
    * 0x01: 一般性失败
    * 0x02: 规则不允许转发
    * 0x03: 网络不可达
    * 0x04: 主机不可达
    * 0x05: 连接拒绝
    * 0x06: TTL超时
    * 0x07: 不支持请求包含CMD
    * 0x08: 不支持请求中的atyp
    * 0x09-0xFF: 未定义
   */
   char rep;

   /**
    * 保留位
    * 必须设置为: 0x00
   */
   char rsv;

    /**
     * 地址类型
     * IPV4: 0x01
     * DOMAINNAME(域名): 0x03
     * IPV6: 0x04
    */
   char atyp;

   /**
    * 地址
   */
   char addr[4];

    /**
     * 端口
    */
   char port[2];

};

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


// 获取域名信息与端口号信息
extern int get_sock_domain(const struct client_request_protocol* request, 
                           struct sock_domain* domain);

#endif