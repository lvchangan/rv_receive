/**
 * 每个TcpClient处理的
 *
 * 考虑到改模块为专有的，Client连接数不会太多，
 * 为了设计简便，每一个Client都有自己的线程来接收数据，不使用 epoll 模型
 *
 * 1. 如果是 Tcp Server accept创建的，则由 Tcp Server 来执行 delete 销毁
 * 2. 如果是上层创建的 Client，则由上层来销毁
 */

#ifndef SOCKET_TCPCLIENT_H
#define SOCKET_TCPCLIENT_H

#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <fcntl.h>
#include "native.h"
#include "RingBuffer.h"
#include "Event.h"
#include "UdpClient.h"
#include "SendPackageCache.h"
#include "decoder/avplayer.h"
class TcpNativeInfo;

class Tcp;

#include "UdpCombinePackage.h"

class TcpClient {
    friend class Tcp;

    /**
     * 定义RingBuffer的缓冲区大小
     */
    const int RINGBUFFER_SIZE = 4 * 1024 * 1024;
public:
    TcpClient(TcpNativeInfo *nativeInfo, Tcp *tcp, int fd, struct sockaddr_in *addr);

    TcpClient(TcpNativeInfo *nativeInfo);

    ~TcpClient();

    TcpNativeInfo *GetModule() {
        return this->nativeInfo;
    }

    int GetSocketFd() {
        return fd_socket;
    }

    bool IsValid() {
        return fd_socket > 0;
    }

    struct sockaddr_in GetSockAddr() {
        return addr;
    }

    bool IsClient(int addr) {
        return addr == (int) this->addr.sin_addr.s_addr;
    }

    UdpCombinePackage *GetUdpCombin() { return mUdpCombinePackage; }

    void Release(bool selfDis);

    void Put(uint8_t *data, size_t len);

    bool Connect(const char *ip, int port);

    int Send(int type, void *data, int len);

    int SendMediaData(int type, int offset, int len);

    void BindBufferVideo(void *video, int size) {
        this->bufferVideo = (uint8_t *) video;
        this->lenBufVideo = size;
    }

    void BindBufferAudio(void *audio, int size) {
        this->bufferAudio = (uint8_t *) audio;
        this->lenBufAudio = size;
    }

    void CreateBufferVideo(){
         if(bufferVideo != nullptr)
        {
            return;
        }
        uint32_t VideoBufferSize = 2 * 1024 * 1024;
        uint8_t* VideoBuffer = new uint8_t[VideoBufferSize];
        BindBufferVideo(VideoBuffer, VideoBufferSize);
    }

	void CreateBufferAudio(){
         if(bufferAudio != nullptr)
        {
            return;
        }
        uint32_t AudioBufferSize = 2 * 1024;
        uint8_t* AudioBuffer = new uint8_t[AudioBufferSize];
        BindBufferAudio(AudioBuffer, AudioBufferSize);
    }

     void ReleaseBufferVideo()
    {
        if(bufferVideo != nullptr)
        {
            delete[] bufferVideo;
            bufferVideo = nullptr;
            lenBufVideo = 0;
        }
        return;
    }

	void ReleaseBufferAudio()
    {
        if(bufferAudio != nullptr)
        {
            delete[] bufferAudio;
            bufferAudio = nullptr;
            lenBufAudio = 0;
        }
        return;
    }

    void SetMediaDecoderReady(bool ready) {
        this->mediaDecoderReady = ready;
    }

    uint32_t SetUdpSupport(uint32_t udpSupport);

    void StopCtrl();

    void DoRecvUdpMessage(SocketPackageData *packageData) { DoRecvMessage(packageData); };

    SendPackageCache &GetSendPackageCache(int type) {
        if (type == TYPE_MEDIA_VIDEODATA) {
            return sendVideoCache;
        } else if (type == TYPE_MEDIA_AUDIODATA) {
            return sendAudioCache;
        }
        return sendCmdCache;
    }

private:
    /**
     * 发送包的序号
     */
    uint32_t seq;
    TcpNativeInfo *nativeInfo;
    bool running;
    //const Tcp *tcp;
    int fd_socket;
    struct sockaddr_in addr;
    /**
     * 如果存在，则某些数据采用UDP协议发送
     */
    UdpClient *udpClient = nullptr;
    /**
     * 是否支持UDP
     * 同 UdpSupportMsg::supportType
     * 注意:
     * 1.作为Server，向Client发送，用来控制Client是否支持
     * 2.作为Client，存储从Server发过来的 TYPE_UDP_SUPPORT
     *
     * 上层都可以通过函数 SetUdpSupport() 来设置是否支持
     * 1.Server: 设置后将会发送消息 TYPE_UDP_SUPPORT 通知到Client
     * 2.Client: 设置后将会控制当前连接是否使用UDP发送
     */
    uint32_t udpSupportType = 0;
    /**
     * 与JAVA层绑定的Video数据缓存
     * 在上报到JAVA层时，先复制到这里
     */
    uint8_t *bufferVideo = nullptr;
    int lenBufVideo = 0;
    /**
     * 与JAVA层绑定的Audio数据缓存
     */
    uint8_t *bufferAudio = nullptr;
    int lenBufAudio = 0;
    /**
     * 接收的缓冲大小
     */
    //RingBuffer bufferRecv;
    /**
     * Recv线程的id
     */
    pthread_t tidRecv = 0;

    /**
     * 是否是本身主动断开的
     */
    bool selfDisconnect = false;
    /**
     * 写入锁
     */
    pthread_mutex_t mutex_write;
    /**
     * 循环发送心跳的Token
     * 循环时判断保存的token与这个不一致，表示需要终止循环
     */
    long hbToken = 0;
    /**
     * JAVA层的媒体解码是否准备OK
     * 仅仅为true时才会上报Video/Audio Data
     *
     * 注意：移植到手机端时需要注意，JAVA层也需要移植过来
     */
    bool mediaDecoderReady = false;
    /**
     * 超时未接收到数据的计数器
     * 没接收到数据计数器复位=0
     */
    int mRecvTimeoutCounter = 0;

    UdpCombinePackage *mUdpCombinePackage = nullptr;

    SendPackageCache sendVideoCache;
    SendPackageCache sendAudioCache;
    SendPackageCache sendCmdCache;
    /**
     * 是否正在投屏工作
     */
    bool working = false;
    /**
     * 是否上报 TYPE_NEW_CLIENT
     */
    bool newClientReported = false;
    /**
     * 心跳需要请求的消息，取值为 HBFLAG_CLIENTINFO, ...
     * 注意：仅仅 Server（TV）端有效
     */
    uint32_t mHeartbeatFlags = 0;

private:
    void DoRecvLoop();

    void DoRecvMessage(SocketPackageData *packageData);

    void DoHeartbeatLoop(long token);

    static void *threadRecv(void *p);

    static void *threadHeartbeat(void *);

private:
    /**
     * 有部分数据是需要通过线程发送的，避免在同一线程中数据量大，处理不过来造成网络堵塞
     */
    typedef struct ThreadSendData_s {
        //数据类型
        int32_t type;
        //实际有效的数据长度
        int32_t dataSize;
        //发送时的当前时间（微秒）
        int64_t time;
        //下面data实际的字节数
        uint32_t capacity;
        //数据内容
        uint8_t data[0];
    } ThreadSendData;

    class ThreadDataProcessor {
    public:
        TcpClient *pTcpClient;

        ThreadDataProcessor(TcpClient *p);

        void Create(int sizeCollections);

        void Destroy();

        ThreadSendData *GetData();

        void PutData(SocketPackageData *packageData);

    private:
        std::list<ThreadSendData *> threadDataList;
        /**
         * threadDataList 最大的元素个数。一旦超过这个元素则将 threadDataList 清空
         * 就是说，threadDataList 中总是保存最新的数据
         */
        int maxListCount = 15;
        /**
         * 线程发送数据的锁，主要是对 threadDataList 锁
         */
        pthread_mutex_t mutex_thread_data;
        Event event;
        pthread_t tidThreadSendData = 0;
    };

    /**
     * 接收到的音视频对象，是放到线程中上报到JAVA层的
     */
    ThreadDataProcessor mReceivedAudioProcessor;
    ThreadDataProcessor mReceivedVideoProcessor;

    static void *threadReportData(void *p);

    void DoThreadReportData(ThreadDataProcessor *pProcessor);

    /**
 * 将Sene()到Socket中的数据先缓存起来，在单独的线程中发送
 * 避免主线程中发送造成
 */
    class SendCacheManager {
    public:
        TcpClient *pTcpClient;

        void Create();

        void Destroy();

        SendCacheManager(TcpClient *p);

        ~SendCacheManager();

    public:
        int PutSendData(int type, void *data, int len);

        ThreadSendData *GetSendData();

        void ClearVideoData();

        void ClearAudioData();

    private:
        uint8_t *sendBuffer;        //MAXSIZE_SENDSOCKPACKAGE+SOCKPACKAGEHEAD_SIZE
        pthread_t tidWrite;
        pthread_mutex_t mutex;
        std::list<ThreadSendData *> cmdList;        //指令数据
        std::list<ThreadSendData *> videoList;      //视频H264数据
        std::list<ThreadSendData *> audioList;      //音频数据

        Event event;

        static void *threadSendData(void *p);

        void DoThreadSendData();
    };

    SendCacheManager sendCacheManager;

public:
	Tcp *tcp;
	void SendRequestQuality();
	void videodecoderinit();
	AVPlayer *avplayer;
};


#endif //SOCKET_TCPCLIENT_H
