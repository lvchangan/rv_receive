//
// Created by hq on 2021/2/1.
//

#include <endian.h>
#include <sys/socket.h>
#include <pthread.h>
#include "UdpClient.h"
#include "UdpServer.h"
#include "Message.h"
#include "native.h"
#include "TcpClient.h"

UdpClient::UdpClient(TcpClient *tcpClient, struct in_addr sin_addr, int port, int sliceSize) {
    this->pTcpClient = tcpClient;
    this->slice_size = sliceSize;
    memset(&addr, 0x00, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = sin_addr.s_addr;
    fd_socket = socket(AF_INET, SOCK_DGRAM, 0);
    int ret = 0;
    int len = sizeof(ret);
    ret = MAXSIZE_SOCKPACKAGE;
    len = setsockopt(fd_socket,SOL_SOCKET,SO_SNDBUF,&ret,len);
    pthread_mutex_init(&mutex_write, nullptr);
}

UdpClient::~UdpClient() {
    pthread_mutex_lock(&mutex_write);
    if (fd_socket > 0) {
        close(fd_socket);
        fd_socket = 0;
    }
    pthread_mutex_unlock(&mutex_write);
    pthread_mutex_destroy(&mutex_write);
}

void UdpClient::UpdateAddress(struct in_addr sin_addr, int port) {

}

/**
 * 将数据通过UDP发送出去
 * 如果超出UDP包大小，则会自动分包
 * @param type 数据类型
 * @param data 数据内容
 * @param dataSize 数据大小
 * @return <=0表示失败
 */
int UdpClient::Send(int type, void *data, int dataSize) {
    if (fd_socket <= 0) return 0;
    SendPackageCache &sendCache = pTcpClient->GetSendPackageCache(type);
    SocketPackageData *packageData = sendCache.AddPackageData(type, data, dataSize, slice_size);
    return sendCache.Send(fd_socket, &addr, slice_size, packageData, nullptr);
}


void UdpClient::ReSend(int type, uint32_t seq, uint32_t *slicesMask) {
    SendPackageCache &sendCache = pTcpClient->GetSendPackageCache(type);
    sendCache.Send(fd_socket, &addr, slice_size, seq, slicesMask);
}

void UdpClient::ReSendRange(int type, uint32_t seqStart, uint32_t seqEnd, uint32_t flag) {
    SendPackageCache &sendCache = pTcpClient->GetSendPackageCache(type);
    sendCache.SendRange(fd_socket, &addr, slice_size, seqStart, seqEnd, flag);
}