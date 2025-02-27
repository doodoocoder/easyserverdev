/**
 * select函数示例，server端, select_server.cpp
 * 证明recv函数是从socket fd中move数据，详见 是从socket fd中move数据.png 
 */
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <iostream>
#include <string.h>
#include <sys/time.h>
#include <vector>
#include <errno.h>

//自定义代表无效fd的值
#define INVALID_FD -1

int main(int argc, char* argv[])
{
    //创建一个侦听socket
    int listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1)
    {
        std::cout << "create listen socket error." << std::endl;
        return -1;
    }

    //初始化服务器地址
    struct sockaddr_in bindaddr;
    bindaddr.sin_family = AF_INET;
    bindaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    bindaddr.sin_port = htons(3000);
    if (bind(listenfd, (struct sockaddr *)&bindaddr, sizeof(bindaddr)) == -1)
    {
        std::cout << "bind listen socket error." << std::endl;
        close(listenfd);
        return -1;
    }

    //启动侦听
    if (listen(listenfd, SOMAXCONN) == -1)
    {
        std::cout << "listen error." << std::endl;
        close(listenfd);
        return -1;
    }

    //存储客户端socket的数组
    std::vector<int> clientfds;
    int maxfd = listenfd;

    while (true)
    {
        fd_set readset;
        FD_ZERO(&readset);

        //将侦听socket加入到待检测的可读事件中去
        FD_SET(listenfd, &readset);

        //将客户端fd加入到待检测的可读事件中去
        int clientfdslength = clientfds.size();
        for (int i = 0; i < clientfdslength; ++i)
        {
            if (clientfds[i] != INVALID_FD)
            {
                FD_SET(clientfds[i], &readset);
            }
        }

        timeval tm;
        tm.tv_sec = 1;
        tm.tv_usec = 0;
        //暂且只检测可读事件，不检测可写和异常事件
        int ret = select(maxfd + 1, &readset, NULL, NULL, &tm);
        if (ret == -1)
        {
            //出错，退出程序。
            if (errno != EINTR)
                break;
        }
        else if (ret == 0)
        {
            //select 函数超时，下次继续
            continue;
        } else {
            //检测到某个socket有事件
            if (FD_ISSET(listenfd, &readset))
            {
                //侦听socket的可读事件，则表明有新的连接到来
                struct sockaddr_in clientaddr;
                socklen_t clientaddrlen = sizeof(clientaddr);
                //4. 接受客户端连接
                int clientfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clientaddrlen);
                if (clientfd == -1)
                {
                    //接受连接出错，退出程序
                    break;
                }

                //只接受连接，不调用recv收取任何数据
                std:: cout << "accept a client connection, fd: " << clientfd << std::endl;
                clientfds.push_back(clientfd);
                //记录一下最新的最大fd值，以便作为下一轮循环中select的第一个参数
                if (clientfd > maxfd)
                    maxfd = clientfd;
            }
            else
            {

                
                char recvbuf[50];
                int clientfdslength = clientfds.size();
                for (int i = 0; i < clientfdslength; ++i)
                {
                    if (clientfds[i] != -1 && FD_ISSET(clientfds[i], &readset))
                    {
                        memset(recvbuf, 0, sizeof(recvbuf));
                        //非侦听socket，则接收(move)数据
                        int length = recv(clientfds[i], recvbuf, 50, 0);
                        if(length == 50){//当一次性读取socket fd的长度为50bytes时，才打印
                            std::cout << "recv data from client, data: " << recvbuf <<" from client "<<clientfds[i] << std::endl;
                            //6. 将收到的数据原封不动地发给客户端
                            ret = send(clientfds[i], recvbuf, strlen(recvbuf), 0);
                            if (ret != strlen(recvbuf))
                                std::cout << "send data error." << std::endl;
                            std::cout << "send data to client successfully, data: " << recvbuf << " to "<< clientfds[i] <<std::endl;
                            clientfds[i] = INVALID_FD;
                            continue;
                        }
                        std::cout<<"fd: "<<clientfds[i]<<"length is: "<<length<<std::endl;
                        length = recv(clientfds[i], recvbuf, 50, 0);
                        std::cout<<"fd: "<<clientfds[i]<<"length is: "<<length<<std::endl;
                        length = recv(clientfds[i], recvbuf, 50, 0);
                        std::cout<<"fd: "<<clientfds[i]<<"length is: "<<length<<std::endl;


                        if (length <= 0 && errno != EINTR)
                        {
                            //收取数据出错了
                            std::cout << "recv data error, clientfd: " << clientfds[i] << std::endl;
                            close(clientfds[i]);
                            //不直接删除该元素，将该位置的元素置位-1
                            clientfds[i] = INVALID_FD;
                            continue;
                        }/*else{


                            std::cout << "recv data from client, data: " << recvbuf <<" from client "<<clientfds[i] << std::endl;
                            //6. 将收到的数据原封不动地发给客户端
                            ret = send(clientfds[i], recvbuf, strlen(recvbuf), 0);
                            if (ret != strlen(recvbuf))
                                std::cout << "send data error." << std::endl;
                            std::cout << "send data to client successfully, data: " << recvbuf << " to "<< clientfds[i] <<std::endl;
                            clientfds[i] = INVALID_FD;
                        }*/


                    }
                }

            }
        }
    }

    //关闭所有客户端socket
    int clientfdslength = clientfds.size();
    for (int i = 0; i < clientfdslength; ++i)
    {
        if (clientfds[i] != INVALID_FD)
        {
            close(clientfds[i]);
        }
    }

    //关闭侦听socket
    close(listenfd);

    return 0;
}
