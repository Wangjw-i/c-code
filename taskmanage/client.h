
#ifndef __HEAD_H__
#define __HEAD_H__
#include <stdio.h>
#define PORT 7800
struct client_status
{
    int root_type; //1可root，0不可root
    int pace_type; //1异步调用，0同步调用
    char filename[128];
    char filepath[128];
    char buf[1024];
    char needcommand[128];
    int pid; //进程id
    int fin;
};
void * cnnt(void* arg);
#endif
