#include "proxy.h"
#include "proxy_errno.h"
#include "string.h"
#include <sys/select.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/fcntl.h>
#include <stdio.h>

extern int __address_parser__(const struct client_request_licenes* request, struct sockaddr_in* addr){
    if(request == NULL)
        return PROXY_NULL;
    if(request->atyp == ATYP1){
        memcpy(&addr->sin_addr, request->addr, 4);
        memcpy(&addr->sin_port, request->port, 2);
    }else if (request->atyp == ATYP2)
    {
        unsigned int domain_len = (unsigned int)*request->addr;
        char domain[255] = {0};
        strncpy(domain, request->addr+1, domain_len);
        struct proxy_dns_info dns;
        if(proxy_dns(domain, &dns) != 0)
            return -1; //域名解析失败
        memcpy(&addr->sin_addr, dns.domain.n, 4);
        memcpy(&addr->sin_port, &request->addr+1+domain_len, 2);
    }else
    {
        puts("address .......");
        return -1;
    }
    return 0;
}

/**
 * sock5认证
*/
extern int proxy_sock5_licenes(int fd){
    char buff[10] = {0};
    if(recv(fd, buff, sizeof(buff), 0) < 0)
        return -1;
    puts("sock5 licenes..........1");
    struct client_licenes* client = (struct client_licenes*)buff;
    if(client->ver != SOCK5)
        return  PROXY_LICENES;
    struct server_licenes s_licenes = {SOCK5, METHOD0};
    char rep_buff[2] = {0};
    memcpy(rep_buff, &s_licenes, 2);
    if(send(fd, rep_buff, sizeof(rep_buff), 0) < 0)
        return -1;
    puts("sock5 licenes..........2");
    char req_buff[270] = {0};
    if(recv(fd, req_buff, sizeof(req_buff), 0) < 0)
        return -1;
    puts("sock5 licenes..........3");
    struct client_request_licenes* request = (struct client_request_licenes*)req_buff;
    if(request->ver != SOCK5)
        return -1;
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    if(__address_parser__(request, &addr) != 0)
        return -1; // 地址解析失败
    struct proxy_info info;
    proxy_ntos(&addr, &info);
    printf("connect: %s:%d\n", info.addr, info.port);
    int conn_fd = -1;
    if((conn_fd = proxy_connect(&addr)) < 0)
        return -1; //连接失败
    puts("sock5 connect.........4");
    struct server_response_licenes response = {SOCK5, REP0, RSV, ATYP1};
    bzero(&buff, sizeof(buff));
    memcpy(buff,  &response, sizeof(buff));
    if(send(fd, buff, sizeof(response), 0) < 0)
        return -1;
    puts("sock5 licenes ok");
    return conn_fd;
}

/**
 * 代理客户端认证
*/
extern int proxy_client_licenes(int fd){
    return 0;
}

/**
 * 代理服务端认证
*/
extern int proxy_server_licenes(int fd){
    return  0;
}

/**
 *  代理连接
*/
extern int proxy_connect(const struct sockaddr_in* addr){
    if(addr == NULL)
        return PROXY_NULL;
    int fd = -1;
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    if(connect(fd, SOCKADDR(addr), sizeof(struct sockaddr)) < 0)
        return -1; 
    return fd;
}   

/**
 * 创建一个socket 
*/
extern int create_socket(const struct sockaddr_in* addr, int listen_num){
    if(addr == NULL)
        return PROXY_NULL;
    int fd = -1;
    if((fd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
        return -1;
    if(bind(fd, (struct sockaddr*)addr, sizeof(struct sockaddr)) < 0)
        return -1;
    if(listen(fd,  listen_num) < 0)
        return -1;
    return fd;
}

/**
 * 将一个网络自己序转换为主机字节序
*/
extern int proxy_ntos(const struct sockaddr_in* addr, struct proxy_info* info){
    if(addr ==NULL || info == NULL)
        return PROXY_NULL;
    inet_ntop(addr->sin_family, &addr->sin_addr, info->addr, 32);
    info->port = ntohs(addr->sin_port);
}

/**
 * 域名解析
*/
extern int proxy_dns(char *domain, struct proxy_dns_info* dns){
    if(domain == NULL)
        return PROXY_NULL;
    printf("dns: %s\n", domain);
    struct hostent* host = gethostbyname(domain);
    if(host == NULL)
        return PROXY_DNS;
    memcpy(&dns->host, host, sizeof(struct hostent));
    if(dns->host.h_addr_list ==  NULL)
        return PROXY_DNS; //解析失败
    memcpy(&dns->domain.h, domain, sizeof(domain));
    memcpy(&dns->domain.n, dns->host.h_addr_list[0], sizeof(dns->host.h_addr_list[0]));
    return 0; // 解析成功
}


/**
 * 转发数据程序
*/
extern int forward_data(int dest_fd, int src_fd, char *buff, int size){
    // bzero(buff, sizeof(buff)); // 清空缓冲区
    int data_len = 0;
    memset(buff, 0, size);
    int done;

    while (1)
    {
        data_len = recv(dest_fd, buff, size, 0);
        printf("len :%d\n", data_len);
        if(data_len == -1){
            if(errno == EAGAIN)
                break;
            return -1;
        }
        if(data_len == 0)
            return -1;
        if(send(src_fd, buff, data_len, 0) == -1){
            perror("转发遇到错误");
            return -1; // 转发遇到错误
        }
    }

}

/**
 * 设置阻塞
*/
extern int setnobolck(int fd);

/**
 * 数据转发
*/
extern  int proxy_forward_data(int s_fd, int t_fd){
    fd_set read_set;
    struct timeval time;
    time.tv_sec = 10;
    time.tv_usec = 0;
    char buff[1024 * 1024] = {0};
    int buff_size = sizeof(buff);
    int max_fd  = s_fd > t_fd?s_fd : t_fd;
    int ret = 0;
    int data_len = -1;
    if(setnobolck(s_fd) != 0 || setnobolck(t_fd) != 0)
        return -1;
    while (1)
    {
        FD_ZERO(&read_set);
        FD_SET(s_fd, &read_set);
        FD_SET(t_fd, &read_set);
        ret = select(max_fd + 1, &read_set, NULL, NULL, NULL);
        printf("fd1:%d, fd2: %d, ret: %d\n", s_fd, t_fd,ret);
        if(ret  == -1)
            break;
        if(ret == 0)
            break;
        if(FD_ISSET(s_fd, &read_set)){
            if(forward_data(s_fd, t_fd, buff, buff_size) < 0){
                // perror("转发失败");
                return -1;
            }
        }else if(FD_ISSET(t_fd, &read_set)){
            if(forward_data(t_fd, s_fd, buff, buff_size) < 0){
                // perror("转发失败");
                return -1;
            }
        }else{
            // printf("....................\n");
        }
        
    }
    printf("线程结束\n");

}

/**
 *  信号注册
*/
extern int signal_register(int* sigs,  int size, void (*callback)()){
    for(int i = 0; i < size; i++){
        if(signal(sigs[i], callback) == SIG_ERR){
            return -1;
        }
    }
    return 0;
}


/**
 * 创建tcp
*/
extern int create_tcp(char* port, int listen_num, struct proxy_info* p_info){
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family= AF_INET;
    addr.sin_port = htons(atol(port));
    if(p_info != NULL){
        proxy_ntos(&addr, p_info);
    }
    p_info->fd = create_socket(&addr, listen_num);
    return p_info->fd;
}

/**
 * 连接到代理
*/
extern int connect_proxy(const struct proxy_info* proxy){
    struct sockaddr_in addr;
    addr.sin_addr.s_addr = inet_addr(proxy->addr);
    addr.sin_family = AF_INET;
    addr.sin_port = ntohs(proxy->port);
    int conn_fd;
    if((conn_fd = proxy_connect(&addr)) < 0)
        return -1;
    if(proxy_client_licenes(conn_fd) < 0)
        return -1;
    return conn_fd;
}