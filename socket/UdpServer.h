/**
 * 仅仅作为Server端使用，用来监听UDP端口
 * 为了兼容以前的代码架构，在 Tcp 中初始化
 */

#ifndef _UDPSERVER_H_
#define _UDPSERVER_H_

#include <list>
#include "CombinePackage.h"

class Tcp;

class UdpServer {
public:
    void StartServer(Tcp *tcp, int port);

    bool StopServer();

private:
    Tcp *tcp;
    int fd_socket = 0;
    int listenPort;
    /**
     * 保存已经缓存所有不得组合好了的包
     * 因为UDP顺序随机，因此需要使用缓存
     */
    std::list<CombinePackage *> mPackages;
private:
    void doStartListen();

    static void *threadListen(void *p);

	void AnswerRadioBroadcast(struct sockaddr_in cli);
};


#endif //_UDPSERVER_H_
