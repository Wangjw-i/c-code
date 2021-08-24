#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <sys/msg.h>
#include <time.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/ipc.h>
#include <sys/types.h>
#define BUF_SIZE 1024
#define PORT 7800
#define THREADNUM 1
#define KEY 5000
typedef struct condition
{
    pthread_mutex_t pmutex; //锁变量
    pthread_cond_t pcond;   //条件变量
} condition_t;

typedef struct task
{
    void *(*run)(void *arg); //回调函数
    void *arg;
    struct task *next; //指针域
} task_t;              //任务链表

typedef struct client
{
    int pace_type; //1异步调用，0同步调用
    char buf[1024];
    char needcommand[128];
    int pid; //进程id
    int fin;
    int k;
    int thread_select;
}client_status;

//定义线程池属性
typedef struct threadpool
{
    condition_t ready;
    task_t *first;
    task_t *last;
    int listen_sock;
    int app_sock[10];
    int counter1; //线程数量
    int counter2;
    int idle1;
    int idle2;
    int max_threads; //最大线程数量
    int quit;        //线程运行状态
    client_status infor;
} threadpool_t;

static int sock_num = 0;//记录连接的sock数量

int condition_init(condition_t *cond);//线程锁初始化
int condition_lock(condition_t *cond);//线程锁
int condition_wait(condition_t *cond);
int condition_timewait(condition_t *cond, const struct timespec *abstime);
int condition_unlock(condition_t *cond);
int condition_signal(condition_t *cond);
int condition_broadcast(condition_t *cond);
int condition_destory(condition_t *cond);
void giveroot(client_status infor,int sock_fd);
void *thread_routine1(void *arg);
void *thread_routine2(void *arg);
void threadpool_init(threadpool_t *pool, int threads);//初始化
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg);//加任务
void threadpool_destory(threadpool_t *pool);//销毁线程池
void *mytask(void *arg);

int main()
{
    threadpool_t pool;
    threadpool_init(&pool, THREADNUM);
    int sock_fd = 0;
    int id2 = 0;
    char buf[20];
    char continu[10] = {0};
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    // socklen_t len = sizeof(clientaddr);
    memset(&clientaddr, 0, sizeof(clientaddr));

    socklen_t len = sizeof(serveraddr);
    memset(&serveraddr, 0, sizeof(serveraddr));
    pool.listen_sock = socket(AF_INET, SOCK_STREAM, 0);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(pool.listen_sock, (struct sockaddr *)&serveraddr, len);

    listen(pool.listen_sock, 6); //开始监听
    while (1)
    {
        pool.app_sock[sock_num] = accept(pool.listen_sock, (struct sockaddr *)&clientaddr, &len);
        if (pool.app_sock[sock_num] >= 0)
        {
            if(sock_num>0)
                printf("\n");
            printf("accpet success!app_sock=%d,k=%d\n", pool.app_sock[sock_num], sock_num);
            sock_num++;
            printf("app_sock = %d,k=%d\n", pool.app_sock[sock_num - 1], sock_num - 1);
            if (pool.app_sock[sock_num - 1] >= 0)
            {
                printf("client online!\n");
                recv(pool.app_sock[sock_num - 1], &pool.infor, sizeof(client_status), 0);
                printf("pid :%d\n", pool.infor.pid);
            }
        }
        int *arg = (int *)malloc(sizeof(int));
        *arg = sock_num;
        threadpool_add_task(&pool, mytask, arg);

        //创建与thread_routine进行通信的消息队列
        memset(continu, 0, sizeof(continu));
        id2 = msgget(7000, 0666 | IPC_CREAT);
        if (id2 == -1)
            perror("msgget");

        //利用消息队列返回线程结束信息
        for (;;)
        {
            msgrcv(id2, continu, sizeof(continu), 0, 0);
            if (strcmp(continu, "ok") == 0)
            {
                msgctl(id2, IPC_RMID, 0);
                break;
            }
        }
    }
    threadpool_destory(&pool);
    return 0;
}
//线程锁初始化
int condition_init(condition_t *cond)
{
    int status;
    if ((status = pthread_mutex_init(&cond->pmutex, NULL))) //返回0代表初始化成功
        return status;
    if ((status = pthread_cond_init(&cond->pcond, NULL)))
        return status;
    return 0;
}
//线程锁
int condition_lock(condition_t *cond)
{
    return pthread_mutex_lock(&cond->pmutex);
}
//线程解锁
int condition_unlock(condition_t *cond)
{
    return pthread_mutex_unlock(&cond->pmutex);
}
int condition_wait(condition_t *cond)
{
    return pthread_cond_wait(&cond->pcond, &cond->pmutex);
}
int condition_timewait(condition_t *cond, const struct timespec *abstime)
{
    return pthread_cond_timedwait(&cond->pcond, &cond->pmutex, abstime);
}
int condition_signal(condition_t *cond)
{
    return pthread_cond_signal(&cond->pcond);
}
int condition_broadcast(condition_t *cond)
{
    return pthread_cond_broadcast(&cond->pcond);
}
int condition_destory(condition_t *cond)
{
    int status;
    if ((status = pthread_mutex_destroy(&cond->pmutex)))
        return status;
    if ((status = pthread_cond_destroy(&cond->pcond)))
        return status;
    return 0;
}
void giveroot(client_status infor,int sock_fd)
{
    printf("giving root.....\n");
    FILE *fp;
    int i=0;
    char buffer[1024] = {0};
    fp = popen(infor.needcommand, "r");
    if (fp == NULL)
    {
        printf("give root popen error!\n");
    }
    while((buffer[i]=fgetc(fp))!=EOF)
    printf("%c", buffer[i++]);
    if (send(sock_fd, buffer, strlen(buffer)-1, 0) < 0)
    {
        printf("giveroot send error!\n");
    }

    pclose(fp);
}
//1号线程
void *thread_routine1(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    struct timespec abstime;
    struct sockaddr_in clientaddr;
    char okbuf[10] = "ok"; //用于发送结束信号
    int id1 = 0;
    int timeout;
    int listen_sock = pool->listen_sock;
    printf("this is thread_routine1\n");
    printf("thread 0x%0x is starting\n", (int)pthread_self());
    while (1)
    {
        timeout = 0;
        condition_lock(&pool->ready);
        pool->idle1++;
        //等待队列有任务到来或者线程池销毁的通知
        while (pool->first == NULL && !pool->quit)
        {
            printf("thread 0x%0x is waiting\n", (int)pthread_self());
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += 2;
            int status = condition_timewait(&pool->ready, &abstime);
            if (status == ETIMEDOUT)
            {
                printf("thread 0x%0x is wait timed out\n", (int)pthread_self());
                timeout = 1;
                break;
            }
            
        }
        //等到到条件，处于工作状态
        pool->idle1--;
        if (timeout == 0)
        {
            if (pool->app_sock[sock_num - 1] >= 0)
            {
                giveroot(pool->infor, (int)pool->app_sock[sock_num - 1]);
                id1 = msgget(7000, 0666 | IPC_EXCL);
                if (id1 == -1)
                    perror("msgget");
                int ret = msgsnd(id1, okbuf, strlen(okbuf), 0);
                if (ret < 0)
                    perror("msgsnd");
            }
        }
        if (pool->first != NULL)
        {
            task_t *t = pool->first;
            pool->first = t->next;
            //需要先解锁，以便添加新任务。其他消费者线程能够进入等待任务。
            condition_unlock(&pool->ready);
            mytask(t->arg);
            free(t);
            condition_lock(&pool->ready);
        }
        //等待线程池销毁的通知
        if (pool->quit && pool->first == NULL)
        {
            pool->counter1--;
            if (pool->counter1 == 0)
            {
                condition_signal(&pool->ready);
            }
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }

        if (timeout && pool->first == NULL)
        {
            pool->counter1--;
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }
        condition_unlock(&pool->ready);
    }
    close(pool->app_sock[sock_num - 1]);
    printf("thread 0x%0x is exiting\n", (int)pthread_self());
    return NULL;
}
//2号线程
void *thread_routine2(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    struct timespec abstime;
    struct sockaddr_in clientaddr;
    char okbuf[10] = "ok"; //用于发送结束信号
    int id1 = 0;
    int timeout;
    int listen_sock = pool->listen_sock;
    printf("this is thread_routine2\n");
    printf("thread 0x%0x is starting\n", (int)pthread_self());
    while (1)
    {
        timeout = 0;
        condition_lock(&pool->ready);
        pool->idle2++;
        //等待队列有任务到来或者线程池销毁的通知
        while (pool->first == NULL && !pool->quit)
        {
            printf("thread 0x%0x is waiting\n", (int)pthread_self());
            clock_gettime(CLOCK_REALTIME, &abstime);
            abstime.tv_sec += 2;
            int status = condition_timewait(&pool->ready, &abstime);
            if (status == ETIMEDOUT)
            {
                printf("thread 0x%0x is wait timed out\n", (int)pthread_self());
                timeout = 1;
                break;
            }
            
        }
        //等到到条件，处于工作状态
        pool->idle2--;
        if (timeout == 0)
        {
            if (pool->app_sock[sock_num - 1] >= 0)
            {
                giveroot(pool->infor, (int)pool->app_sock[sock_num - 1]);
                id1 = msgget(7000, 0666 | IPC_EXCL);
                if (id1 == -1)
                    perror("msgget");
                int ret = msgsnd(id1, okbuf, strlen(okbuf), 0);
                if (ret < 0)
                    perror("msgsnd");
            }
        }
        if (pool->first != NULL)
        {
            task_t *t = pool->first;
            pool->first = t->next;
            //需要先解锁，以便添加新任务。其他消费者线程能够进入等待任务。
            condition_unlock(&pool->ready);
            t->run(t->arg);
            free(t);
            condition_lock(&pool->ready);
        }
        //等待线程池销毁的通知
        if (pool->quit && pool->first == NULL)
        {
            pool->counter2--;
            if (pool->counter2 == 0)
            {
                condition_signal(&pool->ready);
            }
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }

        if (timeout && pool->first == NULL)
        {
            pool->counter2--;
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }
        condition_unlock(&pool->ready);
    }
    close(pool->app_sock[sock_num - 1]);
    printf("thread 0x%0x is exiting\n", (int)pthread_self());
    return NULL;
}
//初始化
void threadpool_init(threadpool_t *pool, int threads)
{
    condition_init(&pool->ready);
    pool->first = NULL;
    pool->last = NULL;
    pool->listen_sock = 0;
    pool->counter1 = 0;
    pool->counter2=0;
    pool->idle1 = 0;
    pool->idle2 = 0;
    pool->max_threads = threads;
    pool->quit = 0;
    memset(pool->infor.buf,0,sizeof(pool->infor.buf));
    memset(pool->infor.needcommand,0,sizeof(pool->infor.needcommand));
}
//加任务
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg)
{
    //nwetask接受参数的传递run回调函数和arg任务数量
    task_t *newstask = (task_t *)malloc(sizeof(task_t));
    newstask->run = run;
    newstask->arg = arg;
    newstask->next = NULL;

    //加锁进行处理
    condition_lock(&pool->ready);
    //将任务添加到队列中。
    if (pool->first == NULL)
    {
        pool->first = newstask;
    }
    else
        pool->last->next = newstask;
    pool->last = newstask;
    switch (pool->infor.thread_select)
    {
    case 1:
        //如果有等待线程，则唤醒其中一个
        if (pool->idle1 > 0)
        {
            condition_signal(&pool->ready);
        }
        else if (pool->counter1 < pool->max_threads)
        {
            pthread_t tid;
            //printf("tid%d=%lu\n", pool->counter, tid);

            pthread_create(&tid, NULL, thread_routine1, pool);
            pool->counter1++;
        }
        break;

    default:
        //如果有等待线程，则唤醒其中一个
        if (pool->idle2 > 0)
        {
            condition_signal(&pool->ready);
        }
        else if (pool->counter2 < pool->max_threads)
        {
            pthread_t tid;
            //printf("tid%d=%lu\n", pool->counter, tid);

            pthread_create(&tid, NULL, thread_routine2, pool);
            pool->counter2++;
        }
        break;
    }

    condition_unlock(&pool->ready);
}
//销毁线程池
void threadpool_destory(threadpool_t *pool)
{

    if (pool->quit)
    {
        return;
    }
    condition_lock(&pool->ready);
    pool->quit = 1;
    if (pool->counter1 > 0||pool->counter2 > 0)
    {
        if (pool->idle1 > 0||pool->idle2 > 0 )
            condition_broadcast(&pool->ready);

        while (pool->counter1 > 0||pool->counter2 > 0)
        {
            condition_wait(&pool->ready);
        }
    }
    condition_unlock(&pool->ready);
    condition_destory(&pool->ready);
}

void *mytask(void *arg)
{
    printf("thread 0x%0x is working on task %d\n",(int)pthread_self(), *(int *)arg);
    sleep(1);
    free(arg);
    return NULL;
}