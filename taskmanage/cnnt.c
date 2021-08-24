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
#include "client.h"
void *cnnt(void *arg)
{
    int sock_fd;
    struct client_status *infor=(struct client_status *)(void*)arg;
    struct sockaddr_in clientaddr;
    socklen_t len = sizeof(clientaddr);
    memset(&clientaddr, 0, sizeof(clientaddr));
    memset(infor->buf,0,sizeof(infor->buf));
    sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    clientaddr.sin_family = AF_INET;
    clientaddr.sin_port = htons(PORT);
    clientaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    while (1)
    {
        int ret = connect(sock_fd, (struct sockaddr *)&clientaddr, len);
        // if(ret<0)
        // {
        //     perror("connect");
        //     continu;
        // }
        if(ret >=0)
        {
            printf("connect success!\n");
            send(sock_fd,infor, sizeof(struct client_status), 0);
            while (1)
            {
                if (recv(sock_fd, infor->buf, sizeof(infor->buf), 0) > 0)
                    break;
            }
            break;
        }
    }
    returntask(infor);
    close(sock_fd);
    return NULL;
}
