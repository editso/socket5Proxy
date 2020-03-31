#include "multiway.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/fcntl.h>
#include <sys/select.h>

/**
 * 添加一个事件
*/
extern proxy_data* add_event(epoll_data_t* ev, proxy_data* data){
    proxy_data *data_ptr = (proxy_data *)malloc(sizeof(proxy_data));
    ev->ptr = memcpy(data_ptr, data, sizeof(proxy_data));
    return data_ptr;
}

/**
 * del
*/
extern int proxy_del(int efd, int dfd, struct epoll_event* event){
    if(event == NULL)
        return -1;
    if(epoll_ctl(efd,  EPOLL_CTL_DEL, dfd, event) < 0)
        return -1;
    close(dfd);
    return 0;
}

/**
 * 添加
*/
extern int proxy_add(int efd, int afd, int event, proxy_data* data){
    if(data == NULL)
        return -1;
    struct epoll_event ev;
    memset(&ev, 0, sizeof(ev));
    ev.events = event;
    add_event(&ev.data, data);
    if(epoll_ctl(efd, EPOLL_CTL_ADD, afd, &ev) < 0)
        return -1;
    return afd;
}

extern proxy_data* proxy_get(struct epoll_event* event){
    return (proxy_data*)event->data.ptr;
}


/**
 * 设置非阻塞
*/
extern int setnobolck(int fd){
    int flag;
    if((flag = fcntl(fd, F_GETFL, 0)) < 0)
        return -1;
    if(fcntl(fd, F_SETFL, flag | O_NONBLOCK) < 0)
        return -1;
    return 0;
}

