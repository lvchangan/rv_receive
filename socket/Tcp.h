/**
 * 实现TCP功能的封装类
 *
 * @author huangqiang
 */
#ifndef SOCKET_TCP_H
#define SOCKET_TCP_H


#include "TcpClient.h"
#include "UdpServer.h"

#define MAX_CLIENTS 12

class TcpNativeInfo;

class Tcp {

public:
    Tcp(TcpNativeInfo *module);

    ~Tcp();

    void StartServer(int port);

    bool StopServer();

    void setModule(TcpNativeInfo *module) {
        this->nativeInfo = module;
    }

    /**
     * 设置默认Socket连接支持UDP方式
     * @param udpSupport 按位与，取值为 UDPSUPPORT_VIDEODATA, ...
     */
    void SetDefaultUdpSupport(uint32_t udpSupport) {
        this->udpDefaultSupport = udpSupport;
    }

    uint32_t GetDefaultUdpSupport() const { return this->udpDefaultSupport; }

    int GetListenPort() { return this->listenPort; }

    int GetUdpListenPort() const { return this->listenPort + 1; }

    bool CheckTcpClient(TcpClient *client) const;

    void OnUdpSliceReceived(SocketPackageData *packageData, struct in_addr addr);

private:
    TcpNativeInfo *nativeInfo;

    int listenPort;
    /**
     * 是否正在监听
     */
    bool listening;
    /**
     * 监听的fd
     */
    int socketFd;
    /**
     * 最多允许12个客户端同时连接
     */
    TcpClient *clients[MAX_CLIENTS];
    /**
     * UdpServer
     */
    UdpServer udpServer;
    /**
     * UDP默认支持类型
     * 当新建一个TCP连接后会发送到Client中
     */
    uint32_t udpDefaultSupport = UDPSUPPORT_VIDEODATA;
private:
    static void *threadListen(void *);

    int doStartListen();

    void FreeClient(int addr);
public:
	int Clientnum;	//连接的客户端数目
	int ClientId;	//连接的客户端ID
	int ClientPlayernum;	//投屏的客户端数量
	std::list<int> clientpalyerList; //投屏管理链表
	
	//int TVScreenwidth;
	//int TVScreenheight;
	//unsigned char *YUVSplicingBuffer;
	//int YUVSplicingBufferSize;	
};


#endif //SOCKET_TCP_H
