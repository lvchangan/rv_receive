//
// Created by hq on 2019/8/22.
//

#include <cstdint>
#include "TcpNativeInfo.h"

int64_t timestamp() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (int64_t) tv.tv_sec * 1000000 + tv.tv_usec;
}


TcpNativeInfo::TcpNativeInfo() {
    this->tcp = nullptr;
    this->client = nullptr;
}

/**
 * 上报到JAVA层
 *
 * @param client TcpClient
 * @param type 消息类型，取值为 TYPE_NEW_CLIENT, ...
 * @param msg 上报时的信息，根据 type 不同取值不同
 * @param data 以 byte[] 的方式上报数据到JAVA层。只有 msg==NULL时有效
 * @param off 音视频数据的偏移量。如果使用data则表示data的偏移。一般为0
 * @param len 音视频数据的有效长度。如果使用data则表示data的长度
 * @return
 *
 * 注意：该函数非线程安全，也就是说可能会在不同的线程中调用
 *
 * 如果是相同的JAVA线程上报，那么不能使用 AttachCurrentThread/DetachCurrentThread
 *
    int getEnv = jvm->GetEnv((void **) &env, JNI_VERSION_1_6);
    if (getEnv < 0) {
        jvm->AttachCurrentThread(&env, NULL);
        isAttached = true;
    }
    xxxx
    if (isAttached) {
        jvm->DetachCurrentThread();
    }
 */
int64_t
TcpNativeInfo::JniMessageReport(TcpClient *client, int type,
                                const char *msg, const void *data,
                                int64_t off, long len) {
    //ALOGI("JniMessageReport type=%d\n", type);
    this->fn(this->user, client, type, msg, data, off, len);
    return 0;
}
