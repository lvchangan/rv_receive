/**
 * 通过UDP发送数据到UdpServer
 * 为了兼容当前代码架构，在TcpClient中构建
 *
 * 1.发送数据时分包操作。并且标记每个包
 * 2.对发送的数据做缓存，接收到回应则从缓冲中移除
 * 3.如果未接收到回应，则重试发送N此后移除
 */
#ifndef _UDPCLIENT_H_
#define _UDPCLIENT_H_


#include <netinet/in.h>

class TcpClient;

class UdpClient {
public:
    UdpClient(TcpClient *tcpClient, struct in_addr sin_addr, int port, int sliceSize);

    ~UdpClient();

    int Send(int type, void *data, int dataSizes);

    void UpdateAddress(struct in_addr sin_addr, int port);

    void ReSend(int type, uint32_t seq, uint32_t *slicesMask);

    void ReSendRange(int type, uint32_t seqStart, uint32_t seqEnd, uint32_t flag);

private:
    /**
     * 每次发送的分片包大小
     */
    int slice_size;
    int fd_socket;
    struct sockaddr_in addr;
    pthread_mutex_t mutex_write;
    TcpClient *pTcpClient;
};


#endif //_UDPCLIENT_H_
