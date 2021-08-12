#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "client.h"
struct client_status infor;
int main()
{
    pthread_t pid;
    infor.root_type = 1;
    infor.pace_type = 1;
    infor.fin=0;
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
    pthread_create(&pid,NULL,cnnt,(void *)&infor);
    if (infor.pace_type ==1)
    {
        while (1)
        {
            sleep(1);
            printf("go\n");
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
        printf("%s\n",infor.buf);
        while (1)
        {
            sleep(1);
            printf("go\n");
        }
    }
    return 0;
}
