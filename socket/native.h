//
// Created by hq on 2019/8/13.
//

#ifndef SOCKET_NATIVE_H
#define SOCKET_NATIVE_H

#include <string.h>
#include "log.h"
#include "Message.h"

/**
 * 每次Socket发送数据包的大小
 * 这个数值比较关键，必须要大于或等于发送端的，否则会出现数据丢失
 * 因为在分包读取时，使用这个缓冲区来处理分包、粘包的
 * 注意：整包数据可能会大于这个数值。实测发现，1920*1080下某帧会到1MB
 */
#define MAXSIZE_SOCKPACKAGE     (256*1024)
/**
 * SOCKET发送时最大包数据的大小
 */
#define MAXSIZE_SENDSOCKPACKAGE (64*1024)

int64_t timestamp();

#endif //SOCKET_NATIVE_H
