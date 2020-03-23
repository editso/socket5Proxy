#ifndef __MULTIWAY_H__
#define __MULTIWAY_H__
#include <sys/epoll.h>

/**
 * 多路复用
*/

typedef struct{
    int lfd; // 本地fd
    int tfd;  // 目标fd
    int flag; // 标志
}proxy_data;


/**
 * 添加一个事件
*/
extern proxy_data* add_event(epoll_data_t* ev, proxy_data* data);


/**
 * del
*/
extern int proxy_del(int efd, int dfd, struct epoll_event* event);


/**
 * 添加
*/
extern int proxy_add(int efd, int afd, int event, proxy_data* data);


extern proxy_data* proxy_get(struct epoll_event* event);

/**
 * 设置阻塞
*/
extern int setnobolck(int fd);


#endif
