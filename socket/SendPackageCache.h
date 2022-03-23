#ifndef SOCKET_SENDPACKAGECACHE_H
#define SOCKET_SENDPACKAGECACHE_H

#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <pthread.h>
#include "native.h"
#include "Event.h"

class TcpClient;

/**
 * UDP发送时，TV端可能会丢包
 * 因此需要在发送时做一个缓冲，当TV端发现丢包时请求重新发送
 *
 * 目前策略为：
 * Client发送数据时，将数据缓冲起来，并且根据seq排序。
 * 1.当缓冲区满，则自动移除seq最小的
 * 2.当接收到TV端请求重发包时，则重新发送数据包(分包)
 */
class SendPackageCache {
public:
    SendPackageCache();

    ~SendPackageCache();

public:
    void FreeAll();

    SocketPackageData *AddPackageData(int type, void *data, int len, int sliceSize);

    int
    Send(int socketFd, struct sockaddr_in *pAddr, int sliceSize, uint32_t seq,
         uint32_t *slicesMask);

    int Send(int socketFd, struct sockaddr_in *pAddr, int sliceSize, SocketPackageData *pack,
             uint32_t *slicesMask);

    int SendDirect(int socketFd, struct sockaddr_in *pAddr, int sliceSize, int type, void *data,
                   int len);

    void SendRange(int socketFd, struct sockaddr_in *pAddr, int sliceSize, uint32_t seqStart,
                   uint32_t seqEnd, uint32_t flag);

private:
    int Send_l(int socketFd, struct sockaddr_in *pAddr, int sliceSize, SocketPackageData *pack,
               uint32_t *slicesMask);

    /*
     * 最大缓存多少个数据包
     */
    static const int MAX_SIZE = 30;

    pthread_mutex_t mutex;
    /*
     * 发送的数据包的序号
     */
    uint32_t seq = 0;
    /*
     * 已经缓存了的数据包
     */
    std::list<SocketPackageData *> packList;
    /*
     * 每次发送的缓冲区
     */
    uint8_t sendBuffer[MAXSIZE_SOCKPACKAGE + SOCKPACKAGEHEAD_SIZE];

    //使用线程来发送。当有数据需要发送时，先放入到待发送列表中，然后使用线程发送
    bool running = true;
    pthread_mutex_t mutexSend;
    pthread_t pidThreadSend;
    Event eventSend;
    static void *threadSend(void *);
    struct SendData {
        //需要发送的socketFd
        int socketFd;
        //发送的地址。如果地址为0表示为TCP发送
        struct sockaddr_in addr;
        //分片大小
        int sliceSize;
        //需要发送的包的序号。数据在 packList 中保存
        uint32_t seq;
        //发送改包的哪些分片
        uint32_t slicesMask[8];
    };
    std::list<SendData> sendList;
};

#endif  //SOCKET_SENDPACKAGECACHE_H
