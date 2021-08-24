
#ifndef __HEAD_H__
#define __HEAD_H__
#include <stdio.h>
#include <pthread.h> 
#define PORT 7800
struct client_status
{
    int pace_type; //1异步调用，0同步调用
    char buf[1024];
    char needcommand[128];
    int pid; //进程id
    int k;//记录任务id
    int fin;
    int thread_select;
};
int task_id;
void * cnnt(void* arg);//客户端接口
void * returntask(void *arg);//回调函数
void clientthread_run(void*(*task)(void*arg),void *arg); //启动客户端线程函数
#endif
