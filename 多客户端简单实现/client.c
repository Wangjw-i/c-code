#include <stdio.h>

#include <stdlib.h>

#include <errno.h>

#include <string.h>

#include <sys/types.h>

#include <netinet/in.h>

#include <sys/socket.h>

#include <sys/wait.h>

#include <unistd.h>

#include <arpa/inet.h>

int main()

{

    int sockfd, numbytes;

    char buff[100];

    struct sockaddr_in their_addr;

    int i = 0;

    their_addr.sin_family = AF_INET;

    their_addr.sin_port = htons(9999);

    their_addr.sin_addr.s_addr = inet_addr("127.0.0.1"); //本次设置的是本地连接

    bzero(&(their_addr.sin_zero), 0);

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)

    {

        perror("socket error!\n");

        exit(1);
    }

    // 使用connect连接服务器，their_addr获取服务器信息

    if (connect(sockfd, (struct sockaddr *)&their_addr, sizeof(struct sockaddr)) == -1)

    {

        perror("connect error!\n");

        exit(1);
    }

    while (1)

    {

        //连接成功后

        memset(buff, 0, sizeof(buff));

        printf("clinet----:");

        scanf("%s", buff);

        //客户端开始写入数据，*****此处buff需要和服务器中接收

        if (send(sockfd, buff, sizeof(buff), 0) == -1)

        {

            perror("send error \n");

            exit(1);
        }
    }

    close(sockfd);

    return 0;
}
