#include<pthread.h>
#include "client.h"
void thread_run(void*(*task)(void*arg),void *arg)
{
    pthread_t tid;
    struct client_status *infor=(struct client_status *)(void*)arg;
    pthread_create(&tid,NULL,cnnt,infor);
}
