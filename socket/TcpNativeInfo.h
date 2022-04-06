//
// Created by hq on 2019/8/22.
//

#ifndef SOCKET_TCPNATIVEINFO_H
#define SOCKET_TCPNATIVEINFO_H


#include <string.h>
#include "log.h"
#include "Message.h"
#include "TcpClient.h"

typedef int (*OnSocketReceived)(void *user, TcpClient *client, int type, const char *msg,
                             const void *data, int64_t off, long len);

class TcpNativeInfo {
public:
    TcpNativeInfo();

    int64_t JniMessageReport(TcpClient *client, int type, const char *msg,
                             const void *data, int64_t off, long len);
    void SetCallback(void *user, OnSocketReceived fn) {
        this->user = user;
        this->fn = fn;
    }
public:
    /**
     * JAVA对象的全局引用，不使用时必须要删除
     * NewGlobalRef()创建，DeleteGlobalRef()删除
     */
    void *cls;
    /**
     * 如果使用Server功能
     */
    Tcp *tcp;
    /**
     * 如果使用了Client功能（connect 服务器）
     */
    TcpClient *client;

    OnSocketReceived fn;
    void *user;
private:
    //JavaVM *jvm;
};


#endif //SOCKET_TCPNATIVEINFO_H
