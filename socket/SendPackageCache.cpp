#include "SendPackageCache.h"
#include "TcpClient.h"

SendPackageCache::SendPackageCache() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_mutex_init(&mutexSend, nullptr);
    pthread_create(&pidThreadSend, nullptr, threadSend, this);
}


SendPackageCache::~SendPackageCache() {
    running = false;
    eventSend.Signal();
    pthread_join(this->pidThreadSend, nullptr);
    FreeAll();
    pthread_mutex_destroy(&mutexSend);
    pthread_mutex_destroy(&mutex);
}

void SendPackageCache::FreeAll() {
    pthread_mutex_lock(&mutex);
    while (!packList.empty()) {
        //需要销毁掉
        SocketPackageData *pack = packList.front();
        packList.pop_front();
        free(pack);
    }
    pthread_mutex_unlock(&mutex);
}

//************************************
// 函数: AddPackageData
// 将待发送的数据缓存起来
// 参数: int type
// 参数: void * data
// 参数: int len
// 参数: 分包的大小。注意：TCP和UDP是不同的
// 返回: SocketPackageData *
//************************************
SocketPackageData *SendPackageCache::AddPackageData(int type, void *data, int len, int sliceSize) {
    SocketPackageData *packageData = (SocketPackageData *) malloc(sizeof(SocketPackageData) + len);
    packageData->magic = SOCKMAGIC_HEAD;
    packageData->dataSize = len;
    if (data && len > 0) {
        memcpy(packageData->data, data, len);
    }
    packageData->seq = seq++;
    packageData->type = (uint16_t) type;
    packageData->time = timestamp();
    const int slices = (len <= 0 ? 1 : (len + sliceSize - 1) / sliceSize);
    packageData->slices = (uint8_t) slices;
    packageData->curSlice = 0;
    pthread_mutex_lock(&mutex);
    while (packList.size() >= MAX_SIZE) {
        //需要销毁掉
        SocketPackageData *pack = packList.front();
        packList.pop_front();
        free(pack);
    }
    packList.push_back(packageData);
    pthread_mutex_unlock(&mutex);
    return packageData;
}

//************************************
// 函数: Send
// 将数据发送出去。一般用于接收到TV端的TYPE_UDP_REQUEST后发送指定的包和分片
// 参数: uint32_t seq 需要发送的包序号
// 参数: uint32_t sliceMask 需要重发的slice掩码
//************************************
int SendPackageCache::Send(int socketFd, struct sockaddr_in *pAddr, int sliceSize, uint32_t seq,
                           uint32_t *slicesMask) {
    SendData data = {0};
    data.socketFd = socketFd;
    if(pAddr) {
        memcpy(&data.addr, pAddr, sizeof(*pAddr));
    }
    data.sliceSize = sliceSize;
    data.seq = seq;
    if(slicesMask) {
        memcpy(data.slicesMask, slicesMask, sizeof(data.slicesMask));
    } else {
        //表示发送所有的分片
        memset(data.slicesMask, 0xff, sizeof(data.slicesMask));
    }
    pthread_mutex_lock(&mutexSend);
    sendList.push_back(data);
    pthread_mutex_unlock(&mutexSend);
    eventSend.Signal();
    return 0;
}

int SendPackageCache::Send(int socketFd, struct sockaddr_in *pAddr, int sliceSize,
                           SocketPackageData *pack, uint32_t *slicesMask) {
    return Send(socketFd, pAddr, sliceSize, pack->seq, slicesMask);
}

/**
 * 重新发送一定范围内的UDP包
 * @param socketFd 网络套接字
 * @param pAddr  如果是UDP则为UDP的地址
 * @param sliceSize 分片大小
 * @param seqStart 起始seq
 * @param seqEnd 结束seq
 * @param flag 标志位
 */
void SendPackageCache::SendRange(int socketFd, struct sockaddr_in *pAddr, int sliceSize,
                                 uint32_t seqStart, uint32_t seqEnd, uint32_t flag) {
    SendData data = {0};
    data.socketFd = socketFd;
    if(pAddr) {
        memcpy(&data.addr, pAddr, sizeof(*pAddr));
    }
    data.sliceSize = sliceSize;
    //表示发送所有的分片
    memset(data.slicesMask, 0xff, sizeof(data.slicesMask));
    
    if (!packList.empty()) {
        if (flag & UDPRANGE_START) {
            seqStart = packList.front()->seq;
        }
        if (flag & UDPRANGE_END) {
            seqEnd = packList.back()->seq + 1;
        }
        for (auto it = packList.begin(); it != packList.end(); it++) {
            const uint32_t itSeq = (*it)->seq;
            if (itSeq >= seqStart && itSeq < seqEnd) {
                SocketPackageData *packageData = *it;
                data.seq = packageData->seq;
                pthread_mutex_lock(&mutexSend);
                sendList.push_back(data);
                pthread_mutex_unlock(&mutexSend);
            }
        }
    }
}

void *SendPackageCache::threadSend(void *p)
                                    {
    SendPackageCache *pTHIS = (SendPackageCache*)p;
    uint8_t *buffer = new uint8_t[2*1024*1024];
    SocketPackageData *packageData = (SocketPackageData *) buffer;
    while(pTHIS->running) {
        bool matched = false;
        SendData data;
        if(pTHIS->sendList.empty()) {
            pTHIS->eventSend.Wait(1000);
        }
        if(!pTHIS->running) break;
        if(pTHIS->sendList.empty()) continue;
        pthread_mutex_lock(&pTHIS->mutexSend);
        data = pTHIS->sendList.front();
        pTHIS->sendList.pop_front();
        pthread_mutex_unlock(&pTHIS->mutexSend);
        //获取到实际的需要发送的数据
        pthread_mutex_lock(&pTHIS->mutex);
        for (auto it = pTHIS->packList.begin(); it != pTHIS->packList.end(); it++) {
            if ((*it)->seq == data.seq) {
                //需要发送的数据在缓存列表中，发送出去
                SocketPackageData *pack = (SocketPackageData*)(*it);
                memcpy(packageData, pack, SOCKPACKAGE_SIZE(pack));
                matched = true;
                break;
            }
        }
        pthread_mutex_unlock(&pTHIS->mutex);
        if(matched) {
            pTHIS->Send_l(data.socketFd, &data.addr, data.sliceSize, packageData, data.slicesMask);
        }
    }
    delete []buffer;
    return 0;
}
//************************************
// 函数: Send
// 将数据通过完了发送出去。TCP和UDP都通过该函数发送
// 参数: int socketFd 网络套接字
// 参数: struct sockaddr_in * pAddr UDP时有效。==null为TCP
// 参数: int sliceSize 分片大小。TCP=MAXSIZE_SOCKPACKAGE, UDP是TV端返回的
// 参数: SocketPackageData * pack 需要发送的数据包
// 参数: uint32_t* slicesMask 需要发送的slice掩码。null表示全部发送
// 返回: int 发送了的字节数
//************************************
int debug_ignore = 0;

int SendPackageCache::Send_l(int socketFd, struct sockaddr_in *pAddr, int sliceSize,
                             SocketPackageData *pack, uint32_t *slicesMask) {
    const int slices = pack->slices;
    ssize_t wr = 0;
#if 0
    if (pack->type == TYPE_MEDIA_VIDEODATA) {
        if (sliceMask == 0xffffffff) {
            if ((debug_ignore % 100) == 0) {
                ++debug_ignore;
                ALOGI("HQ Ignore seq = %d\n", pack->seq);
                pthread_mutex_unlock(&mutex);
                return 0;
            }
            ++debug_ignore;
        }
        if (sliceMask == 0x0fffffff) {
            ALOGI("HQ Resend seq=%d", pack->seq);
        }
    }
#endif
    //仅仅只有一个包，不需要再次分包了
    if (slices <= 1) {
        if (pAddr) {
            wr = sendto(socketFd, (const char *) pack, SOCKPACKAGE_SIZE(pack), 0,
                        (const struct sockaddr *) pAddr, sizeof(struct sockaddr_in));
        } else {
            wr = send(socketFd, (char *) pack, SOCKPACKAGE_SIZE(pack), 0);
        }
    } else {
        SocketPackageData *packageData = (SocketPackageData *) sendBuffer;
        memcpy(packageData, pack, sizeof(SocketPackageData));
        uint8_t *data = pack->data;
        int len = pack->dataSize;
        for (int s = 0; s < slices; s++) {
            if (CombinePackage::IsSliceMask(s, slicesMask)) {
                const int leave = len >= sliceSize ? sliceSize : len;
                packageData->curSlice = (uint8_t) s;
                packageData->dataSize = (uint32_t) leave;
                if (data && len > 0) {
                    memcpy(packageData->data, data, (size_t) leave);
                }
                ssize_t sd;
                if (pAddr) {
                    sd = sendto(socketFd, (const char *) packageData, SOCKPACKAGE_SIZE(packageData),
                                0,
                                (const struct sockaddr *) pAddr, sizeof(struct sockaddr_in));
                } else {
                    sd = send(socketFd, (char *) packageData, SOCKPACKAGE_SIZE(packageData), 0);
                }
                if (sd < 0) {
                    break;
                }
                wr += sd;
            }
            data += sliceSize;
            len -= sliceSize;
        }
    }
    return wr;
}

//************************************
// 函数: SendDirect
// 直接发送数据，而不是先保存到缓存列表中
// 参数: int socketFd
// 参数: struct sockaddr_in * pAddr
// 参数: int sliceSize
// 参数: int type
// 参数: void * data
// 参数: int len
// 返回: int
//************************************
int SendPackageCache::SendDirect(int socketFd, struct sockaddr_in *pAddr, int sliceSize, int type,
                                 void *data, int len) {
    pthread_mutex_lock(&mutex);
    int wr = 0;
    SocketPackageData *packageData = (SocketPackageData *) sendBuffer;
    packageData->magic = SOCKMAGIC_HEAD;
    packageData->dataSize = len;
    packageData->seq = seq++;
    packageData->type = (uint16_t) type;
    packageData->time = timestamp();
    const int slices = (len <= 0 ? 1 : (len + sliceSize - 1) / sliceSize);
    packageData->slices = slices;
    uint8_t *d = (uint8_t *) data;
    for (int s = 0; s < slices; s++) {
        const int leave = len >= sliceSize ? sliceSize : len;
        packageData->curSlice = (uint8_t) s;
        packageData->dataSize = (uint32_t) leave;
        if (data && len > 0) {
            memcpy(packageData->data, d, (size_t) leave);
        }
        ssize_t sd;
        if (pAddr) {
            sd = sendto(socketFd, (const char *) packageData, SOCKPACKAGE_SIZE(packageData), 0,
                        (struct sockaddr *) pAddr, sizeof(struct sockaddr_in));
        } else {
            sd = send(socketFd, (char *) packageData, SOCKPACKAGE_SIZE(packageData), 0);
        }
        if (sd < 0) {
            break;
        }
        wr += sd;
        d += sliceSize;
        len -= sliceSize;
    }
    pthread_mutex_unlock(&mutex);
    return wr;
}