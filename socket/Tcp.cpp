//
// Created by hq on 2019/8/21.
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
#include <sys/epoll.h>
#include "Tcp.h"
#include "log.h"


Tcp::Tcp(TcpNativeInfo *module) {
    nativeInfo = module;
    socketFd = 0;
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i] = nullptr;
    }
	ClientPlayernum = 0;
}

Tcp::~Tcp() {
    StopServer();
    for (int i = 0; i < MAX_CLIENTS; i++) {
        delete clients[i];
    }
    nativeInfo = nullptr;
}

/**
 * 监听一个端口
 * @param port 需要监听的端口号
 *
 * 如果已经监听，且端口号不同，则返回false
 */
void Tcp::StartServer(int port) {
    listenPort = port;
    pthread_t tidListen;
	ALOGI("LCA start StartServer\n");
    pthread_create(&tidListen, nullptr, threadListen, this);
    udpServer.StartServer(this, port + 1);
}

bool Tcp::StopServer() {
    listening = false;
    if (socketFd > 0) {
        shutdown(socketFd, SHUT_RDWR);
        socketFd = 0;
        udpServer.StopServer();
    }
    return true;
}

void *Tcp::threadListen(void *p) {
    Tcp *tcp = (Tcp *) p;
    pthread_detach(pthread_self());
    tcp->doStartListen();
    return nullptr;
}

int Tcp::doStartListen() {
    int fd_socket = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    LOGI("AAA doStartListen fd_socket=%d, errno=%d\n", fd_socket, errno);
    if (fd_socket < 0) {
        return 0 - __LINE__;
    }
    int value;
    struct sockaddr_in addrServer;
    struct sockaddr_in addrClient;
    addrServer.sin_family = AF_INET;
    addrServer.sin_port = htons(listenPort);
    /* INADDR_ANY表示不管是哪个网卡接收到数据，只要目的端口是SERV_PORT，就会被该应用程序接收到 */
    addrServer.sin_addr.s_addr = htonl(INADDR_ANY);  //自动获取IP地址
    //设置地址重用
    value = 1;
    setsockopt(fd_socket, SOL_SOCKET, SO_REUSEADDR, &value, sizeof(value));
    value = 1;
    setsockopt(fd_socket, SOL_SOCKET, SO_REUSEPORT, &value, sizeof(value));
    /* 绑定socket */
    if (bind(fd_socket, (struct sockaddr *) &addrServer, sizeof(addrServer)) <
        0) {
        LOGE("AAA bind error: errno=%d", errno);
        close(fd_socket);
        return (__LINE__);
    }
    value = listen(fd_socket, 10);
    if (value < 0) {
        close(fd_socket);
        return -__LINE__;
    }
    socketFd = fd_socket;
    this->listening = true;
    while (this->listening) {
        socklen_t len = sizeof(addrClient);
		ALOGI("LCA CCC\n");
        int fd = accept(fd_socket, (struct sockaddr *) &addrClient, &len);
        if (!this->listening) {
            break;
        }
        ALOGI("Client Connected, addr=%x, port=%d\n", addrClient.sin_addr.s_addr,addrClient.sin_port);
        //清空无效的Client
        FreeClient((int) addrClient.sin_addr.s_addr);
        //找到一个空闲的，保存起来
        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] == nullptr) {
				ClientId = i;
                TcpClient *client = new TcpClient(nativeInfo, this, fd, &addrClient);
                clients[i] = client;
                break;
            }
        }
    }
	
    return fd_socket;
}

/**
 * 释放 Client
 * 如果是 Tcp Server模式，创建的Client
 * 当Client断开连接后，在TcpClinet中设置无效，交由 Tcp 来释放资源
 *
 * @param addr 需要强制清空的客户端IP地址。
 * 如果手机Client端再次连接，会以相同的IP不同的端口方式，此时应该把上次连接的断开，否则TV端会显示出2个
 */
void Tcp::FreeClient(int addr) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        TcpClient *client = clients[i];
        if (client == nullptr) continue;
        if (!client->IsValid()) {
            delete client;
            clients[i] = nullptr;
        } else if (client->IsClient(addr)) {
            //这个客户端需要主动断开
            delete client;
            clients[i] = nullptr;
        }
    }
}

/**
 * 检查TcpClient*指针是否在列表中
 */
bool Tcp::CheckTcpClient(TcpClient *client) const {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (client == clients[i]) {
            return true;
        }
    }
    return false;
}

/**
 * UDP接收到一个包的分片
 * 接收到后需要通知到Client表示该包已经接收到了
 * @param packageData 包的一个分片
 * @param addr Udp接收到的发送方地址
 */
void Tcp::OnUdpSliceReceived(SocketPackageData *packageData, struct in_addr addr) {

    for (int i = 0; i < MAX_CLIENTS; i++) {
        TcpClient *client = clients[i];
        if (client && client->IsClient(addr.s_addr) && client->IsValid()) {
            client->GetUdpCombin()->OnUdpSliceReceived(packageData);
            break;
        }
    }
}
