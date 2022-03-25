//
// Created by hq on 2021/2/1.
//

#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include "log.h"
#include "Tcp.h"
#include "UdpServer.h"
#include "Message.h"

void UdpServer::StartServer(Tcp *tcp, int port) {
    this->tcp = tcp;
    this->listenPort = port;
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    //创建网络通信对象
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(listenPort);
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    //绑定socket对象与通信链接
    int ret = bind(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    ALOGI("HQ Udp StartServer port %d, bind result: %d\n", port, ret);
    if (0 > ret) {
        ALOGI("HQ Udp bind port %d error\n", port);
        return;

    }
    fd_socket = sockfd;
    pthread_t tidListen;
    pthread_create(&tidListen, nullptr, threadListen, this);
}

bool UdpServer::StopServer() {
    if (fd_socket > 0) {
        fd_socket = 0;
    }
    return true;
}

void *UdpServer::threadListen(void *p) {
    UdpServer *udp = (UdpServer *) p;
    pthread_detach(pthread_self());
    udp->doStartListen();
    return nullptr;
}

void UdpServer::doStartListen() {
    int sockFd = fd_socket;
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    uint8_t *buffer = new uint8_t[UDP_MAXLENGTH];
    CombinePackage *pCombineProcess;
    SocketPackageData *packageData = (SocketPackageData *) buffer;
    ALOGI("HQ UDP listen fd=%d port=%d\n", fd_socket, listenPort);
    while (fd_socket > 0) {
        ssize_t recv = recvfrom(fd_socket, buffer, UDP_MAXLENGTH, 0, (struct sockaddr *) &cli,
                                &len);
        //ALOGI("AAA UDP Recv. seq=%d, type=%x, size=%d, ", packageData->seq, packageData->type,packageData->dataSize);
        //UDP通信的有界性，此处可以确保接收到的一整个分包(发送端确保不会超出BUFSISE)
		if(buffer[0]== '$' && buffer[1] == 16&&buffer[2] == 1&&buffer[3] == 0&&buffer[4]==0&&buffer[5]==0)
			printf("receive Udp1 gb\n");
        tcp->OnUdpSliceReceived(packageData, cli.sin_addr);
    }
    ALOGI("HQ UDP Server exit ....\n");
    close(sockFd);
    delete[] buffer;
}
