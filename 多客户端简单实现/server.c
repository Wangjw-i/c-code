#include <stdio.h>

#include <stdlib.h>

#include <errno.h>

#include <arpa/inet.h>

#include <unistd.h>

#include <string.h>

#include <sys/types.h>

#include <netinet/in.h>

#include <sys/socket.h>

#include <sys/wait.h>

#define BUFFER_SIZE 100

//服务器读写方法

void listen_data(int fd, int id)

{

    char buffer[BUFFER_SIZE];

    while (1) //一直处于监听客户端信息状态,知道客户端断开或客户端发出退出信息

    {

        memset(buffer, 0, BUFFER_SIZE);

        int len = recv(fd, buffer, BUFFER_SIZE, 0);

        if (len > 0) //如果接受到数据，则判断是否为退出信息

        {

            if (strcmp(buffer, "exit") == 0)

            {

                printf("id %d exited.\n", id);

                break;
            }

            printf("ID_%d:%s\n", id, buffer); //输出第N 个用户，输出的信息
        }

        else //接收数据小于0，或等于0 则退出

        {

            printf("clinet %d close!\n", id);

            break;
        }

        //如果服务端需要发送信息，此处添加发送信息

        //memset(buffer,0,BUFFER_SIZE);

        //scanf("%s",buffer);

        //send(fd,buffer,BUFFER_SIZE,0);
    }

    close(fd); //关闭此客户端的连接的socket
}

int main()

{

    int sockfd, new_fd;

    struct sockaddr_in my_addr; //本地连接socked

    struct sockaddr_in their_addr; //客户端连接socked

    int numbytes;

    socklen_t sin_size;

    char buffer[BUFFER_SIZE];

    static int i = 0; //记录连接客户端数目，可以使用数组，结构体等数据类型记录客户端信息（IP，端口等）

    //记录本地信息

    my_addr.sin_family = AF_INET; //IPV4

    my_addr.sin_port = htons(9999); //绑定端口9999，并将其转化为网络字节

    my_addr.sin_addr.s_addr = INADDR_ANY; //指定接收IP为任意（如果指定IP，则只接收指定IP）

    bzero(&(my_addr.sin_zero), 0); //最后位置赋0，补位置

    //设置socked

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)

    {

        perror("socket error!\n");

        exit(1);
    }

    //绑定socked

    if ((bind(sockfd, (struct sockaddr *)&my_addr, sizeof(my_addr))) < 0)

    {

        perror("bind error!\n");

        exit(1);
    }

    //开启监听,可连接客户端最大为10个

    if (listen(sockfd, 10) == -1)

    {

        perror("listen error!\n");

        exit(1);
    }

    //服务端一直运行，等待客户端连接

    while (1)

    {

        sin_size = sizeof(struct sockaddr_in);

        //等待客户端连接，连接后their_addr接收客户端的IP等信息，没有客户端连接，则一直等待

        if ((new_fd = accept(sockfd, (struct sockaddr *)(&their_addr), &sin_size)) < 0)

        {

            perror("accept error!\n");

            exit(1);
        }
        printf("1\n");

        //连接成功后，连接客户端数+1

        i++;

        //开启进程运行客户端

        pid_t childid;

        childid = fork(); //fork()函数有两个返回值，0为子进程，-1为错。子进程运行客户端

        if (childid == 0)

        {

            close(sockfd); //子进程中不再需要sockfd去监听，这里释放它，只需要new_fd即可

            listen_data(new_fd, i);

            exit(0);
        }

        //父进程继续执行while，在accept（）等待客户端。父进程的socked此时还在运行，没有关闭

        //此处没有设置父进程退出的代码，因为假设服务器一直运行，如果需要服务器自动退出，可设置服务器

        //等待连接的时间，如果一定时间没有客户端连接，可以退出等待，结束
    }

    //所有客户端

    close(sockfd);

    printf("server-------closed.\n");

    return 0;
}

