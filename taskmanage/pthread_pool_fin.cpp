#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/unistd.h>
#include <sys/socket.h>
#include <pthread.h>
#include <time.h>
#include <assert.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#define BUF_SIZE 1024
#define PORT 7800
#define THREADNUM 3

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

//定义线程池属性
typedef struct threadpool
{
    condition_t ready;
    task_t *first;
    task_t *last;
    int listen_sock;
    int app_sock[10];
    int pid;
    int counter; //线程数量
    int idle;
    int max_threads; //最大线程数量
    int quit;        //线程运行状态
    int i;
} threadpool_t;

static int i=0;

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

void *thread_routine(void *arg)
{
    threadpool_t *pool = (threadpool_t *)arg;
    struct timespec abstime;
    struct sockaddr_in clientaddr;
    char buf[BUF_SIZE] = {0};
    int timeout;
    int k=i;
    int listen_sock = pool->listen_sock;

    printf("thread 0x%0x is starting\n", (int)pthread_self());
    while (1)
    {
        timeout = 0;
        condition_lock(&pool->ready);
        pool->idle++;
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
        pool->idle--;
        printf("app_sock = %d,k=%d\n", pool->app_sock[k],k);
        if (pool->app_sock[k] >= 0)
        {
            printf("client online!\n");
            recv(pool->app_sock[k], buf, sizeof(buf), 0);
            printf("%s\n", buf);
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
            pool->counter--;
            if (pool->counter == 0)
            {
                condition_signal(&pool->ready);
            }
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }

        if (timeout && pool->first == NULL)
        {
            pool->counter--;
            condition_unlock(&pool->ready);
            //跳出循环之前要记得解锁
            break;
        }
        condition_unlock(&pool->ready);
    }
    close(pool->app_sock[k]);
    printf("thread 0x%0x is exiting\n", (int)pthread_self());
    printf("............................\n");
    return NULL;
}

//初始化
void threadpool_init(threadpool_t *pool, int threads)
{
    condition_init(&pool->ready);
    pool->first = NULL;
    pool->last = NULL;
    pool->listen_sock = 0;
    pool->counter = 0;
    pool->idle = 0;
    pool->max_threads = threads;
    pool->quit = 0;
}

//加任务
void threadpool_add_task(threadpool_t *pool, void *(*run)(void *arg), void *arg)
{
    task_t *newstask = (task_t *)malloc(sizeof(task_t));
    newstask->run = run;
    newstask->arg = arg;
    newstask->next = NULL;

    condition_lock(&pool->ready);
    //将任务添加到对列中
    if (pool->first == NULL)
    {
        pool->first = newstask;
    }
    else
        pool->last->next = newstask;
    pool->last = newstask;
    //如果有等待线程，则唤醒其中一个
    if (pool->idle > 0)
    {
        condition_signal(&pool->ready);
    }
    else if (pool->counter < pool->max_threads)
    {
        pthread_t tid;
        printf("tid%d=%lu\n",pool->counter,tid);
        pthread_create(&tid, NULL, thread_routine, pool);
        pool->counter++;
        i++;
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
    if (pool->counter > 0)
    {
        if (pool->idle > 0)
            condition_broadcast(&pool->ready);

        while (pool->counter > 0)
        {
            condition_wait(&pool->ready);
        }
    }
    condition_unlock(&pool->ready);
    condition_destory(&pool->ready);
}

void *mytask(void *arg)
{
    printf("thread 0x%0x is working on task %d\n", (int)pthread_self(), *(int *)arg);
    printf("............................\n");
    sleep(1);
    free(arg);
    return NULL;
}

int main()
{
    threadpool_t pool;
    threadpool_init(&pool, THREADNUM);
    int sock_fd = 0;
    char buf[20];
    struct sockaddr_in serveraddr;
    struct sockaddr_in clientaddr;
    // socklen_t len = sizeof(clientaddr);
    memset(&clientaddr, 0, sizeof(clientaddr));

    socklen_t len = sizeof(serveraddr);
    memset(&serveraddr, 0, sizeof(serveraddr));
    pool.listen_sock  = socket(AF_INET, SOCK_STREAM, 0);

    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(PORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);

    int ret = bind(pool.listen_sock, (struct sockaddr *)&serveraddr, len);

    listen(pool.listen_sock ,5);
    int k=0;
    while(1)
    {
        pool.app_sock[k]= accept(pool.listen_sock, (struct sockaddr *)&clientaddr, &len);
        if(pool.app_sock[k]>=0)
        {
            printf("accpet success!app_sock=%d,k=%d\n",pool.app_sock[k],k);
            k++;
        }
        int *arg = (int *)malloc(sizeof(int));
        *arg = k;
        threadpool_add_task(&pool, mytask, arg);
        if(k==THREADNUM-1)
        {
            break;
        }
    }
    sleep(15); //为了等待其他线程结束 当然也可以通过pthread_join来做

    threadpool_destory(&pool);
    return 0;
}
