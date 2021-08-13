#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"
struct client_status infor;
int main()
{
    int count = 0;
    pthread_t tid;
    infor.root_type = 1;
    infor.pace_type = 0;
    infor.pid=getpid();
    infor.fin=0;
    infor.thread_select=1;
    printf("my pid is %d,i select the thread_routine%d\n",infor.pid,infor.thread_select);
    memset(infor.filename, 0, sizeof(infor.filename));
    memset(infor.filepath, 0, sizeof(infor.filepath));
    memset(infor.needcommand, 0, sizeof(infor.needcommand));
    memset(infor.buf,0,sizeof(infor.buf));
    char filename[128] = "test.sh";
    char pathname[128] = "/home/wangjw/hello/test.sh";
    char needcommand[128] = "./test.sh";
    strncpy(infor.filename, filename, strlen(filename));
    strncpy(infor.filepath, pathname, strlen(pathname));
    strncpy(infor.needcommand, needcommand, strlen(needcommand));
    pthread_create(&tid,NULL,cnnt,(void *)&infor);
    if (infor.pace_type ==1)
    {
        while (1)
        {
            printf("%d\n",count);
            count ++;
            sleep(1);
            if(infor.fin==1)
            {
                printf("the value had returned\n");
                infor.fin=0;
            }
        }
    }
    else
    {
        while(infor.fin==0)
        {
            
        }
        infor.fin=0;
        printf("the value had returned\n");
        while (1)
        {
            printf("%d\n",count);
            count++;
            if (count == 21)
                break;
        }
    }
    return 0;
}
