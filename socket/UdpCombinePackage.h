/**
 * 由于UDP包的无序性，当一个包被分拆为多个时，可能会不同的顺序到达
 * 比如说有A、B两个包，被分拆为 A1,A2,A3, B1,B2,B3，
 * 虽然发射端是按照顺序发送的，但接收端可能是 A1,B2,A3,B1,A2,B3 这样的顺序
 * 因此需要在接收端正确的组合
 */
#ifndef _UDPCOMBINEPACKAGE_H_
#define _UDPCOMBINEPACKAGE_H_

#include "CombinePackage.h"
#include <list>

class TcpClient;

class UdpCombinePackage {
public:
    UdpCombinePackage(TcpClient *tcpClient);

    SocketPackageData *OnUdpSliceReceived(SocketPackageData *packageData);

private:
    class CombineCache {
    public:
        CombineCache(TcpClient *tcpClient, int type);

        ~CombineCache();

        SocketPackageData *OnUdpSliceReceived(SocketPackageData *packageData);

    private:
        void RequestSend(int type, uint32_t seq, uint32_t *slicesMask);

        void RequestRangeSend(int type, uint32_t seqStart, uint32_t seqEnd, uint32_t flag);

        int FindFullPackage();

    private:
        /**
         * 请求重发的次数的最大值
         * 总耗时约为 COUNT_RETRY*INTERVAL_RETRY
         */
        static const int COUNT_RETRY = 5;
        /**
         * TYPE_UDP_REQUEST 重发的时间间隔(微秒)
         */
        static const int INTERVAL_RETRY = 100 * 1000;
        /**
         * 数据包类型
         */
        const int dataType;
        TcpClient *tcpClient;
        /**
         * 接收的seq。如果上报的seq不是这个则表明包丢失，需要重新请求
         */
        uint32_t seqReport = 0;
        /**
         * 出错后重试的次数
         * 当发现包漏掉时，TV端发送请求，没发送一次+1，直到 COUNT_RETRY
         * 当上报到JAVA后（正确接收到）或主动抛弃都会置0
         */
        int32_t retryCount = 0;
        /**
         * 上次请求Client重发的时间戳
         */
        int64_t resendTimestamp = 0;
        /**
         * 使用list缓存所有接收到的数据包
         */
        std::list<CombinePackage *> packList;
    };

private:
    //PC端1920*1080视频数据一帧最大有1M，设置2M的缓存
    static const int VIDEO_BUFSIZE = ((1024 + 1024) * 1024);
    static const int AUDIO_BUFSIZE = (32 * 1024);
    static const int OTHER_BUFSIZE = (32 * 1024);

    CombineCache videoCache;

    TcpClient *tcpClient;
    CombinePackage combineAudio, combineOther;
};


#endif //_UDPCOMBINEPACKAGE_H_
