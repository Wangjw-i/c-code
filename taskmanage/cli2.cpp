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
#define PORT 7800
struct client_status
{
    int root_type; //1可root，0不可root
    int pace_type; //1异步调用，0同步调用
    char data[128];
    int pid; //进程id
};
int main()
{
    int sock_fd;
    char data[20] = "test.sh";
    char lock[20] = "chmod 000 ";
    char buf[20] = "./";
    strncat(buf,data,strlen(data));
    strncat(lock,data,strlen(data));
    struct client_status infor;
    infor.pace_type = 0;
    infor.root_type = 1;
    infor.pid = getpid();
    memset(infor.data,0,sizeof(infor.data));
    strncpy(infor.data,data,strlen(data));

    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    memset(&clientaddr, 0, sizeof(clientaddr));

    sock_fd = socket(AF_INET, SOCK_STREAM, 0);

    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(PORT);
    clientaddr.sin_addr.s_addr = inet_addr("127.0.0.3");
    while (1)
    {
        int ret = connect(sock_fd, (struct sockaddr *)&clientaddr, len);
        if (ret >= 0)
        {
            printf("connect success!\n");
            if (send(sock_fd, &infor, sizeof(infor), 0) >= 0)
            {
                printf("Getting root...\n");
                while (1)
                {
                    sleep(1);
                    FILE *fp;
                    char buffer[1024] = {0};
                    fp = popen(buf, "r");
                    fgets(buffer, sizeof(buffer), fp);
                    printf("%s", buffer);
                    if(fp!=NULL)
                    {
                        pclose(fp);
                        break;
                    }
                }
                break;
            }
        }
    }
    popen(lock,"r");
    close(sock_fd);
    return 0;
}