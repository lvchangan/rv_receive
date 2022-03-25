//
// Created by hq on 2021/2/7.
//

#include "UdpCombinePackage.h"
#include "TcpClient.h"

UdpCombinePackage::UdpCombinePackage(TcpClient *tcpClient) : videoCache(tcpClient,
                                                                        TYPE_MEDIA_VIDEODATA) {
    this->tcpClient = tcpClient;
    combineAudio.AllocBuffer(AUDIO_BUFSIZE);
    combineOther.AllocBuffer(OTHER_BUFSIZE);
}

#define time_after(a, b) ((int32_t)(b) - (int32_t)(a) < 0)
//#define time_after(a, b) (a>b)

/**
 * 接收到一个UDP包（可能是分包）后的调用
 * @param packageData 接收到的UDP包数据
 * @return null表示没有组合为一个包，!=null表示返回需要上报的整个包
 */
SocketPackageData *UdpCombinePackage::OnUdpSliceReceived(SocketPackageData *packageData) {
    if (packageData->type == TYPE_MEDIA_VIDEODATA) {
//        ALOGI("HQ UDP Receive seq=%d, slices=%d, cur=%d\n", packageData->seq, packageData->slices,
//              packageData->curSlice);
        return videoCache.OnUdpSliceReceived(packageData);
    } else if (packageData->type == TYPE_MEDIA_AUDIODATA) {
        if (combineAudio.GetSeq() != packageData->seq) {
            //判断序号，如果以前的序号则直接丢弃。声音少量的丢弃没问题
            if (time_after(packageData->seq, combineAudio.GetSeq())) {
                combineAudio.InitPackage(packageData, UDP_PACKENGTH);
            }
        } else {
            combineAudio.PutPackage(packageData);
        }
        if (combineAudio.IsDone()) {
            tcpClient->DoRecvUdpMessage(combineAudio.GetPackageData());
            combineAudio.ResetData();
        }
    } else {
        //目前其他命令总是不分包，直接上报
        if (packageData->slices == 1) {
			if(packageData->data[0]== '$' && packageData->data[1] == 16&&packageData->data[2] == 1&&packageData->data[3] == 0&&packageData->data[4]==0&&packageData->data[5]==0)
			printf("receive Udp gb\n");
            tcpClient->DoRecvUdpMessage(packageData);
        }
    }
    return nullptr;
}


UdpCombinePackage::CombineCache::CombineCache(TcpClient *tcpClient, int type) : dataType(type) {
    this->tcpClient = tcpClient;
}

UdpCombinePackage::CombineCache::~CombineCache() {
    while (!packList.empty()) {
        CombinePackage *pCombinePackage = packList.front();
        packList.pop_front();
        delete pCombinePackage;
    }
}

SocketPackageData *
UdpCombinePackage::CombineCache::OnUdpSliceReceived(SocketPackageData *packageData) {
    CombinePackage *pCombinePackage = nullptr;
    for (auto it = packList.begin(); it != packList.end(); it++) {
        if ((*it)->GetSeq() == packageData->seq) {
            pCombinePackage = *it;
            break;
        }
    }
    if (pCombinePackage) {
        //找到了，表示有接收到过的
        pCombinePackage->PutPackage(packageData);
    } else {
        while (packList.size() >= 15) {
            pCombinePackage = packList.front();
            //ALOGI("HQ UDP drop seq=%d", pCombinePackage->GetSeq());
            packList.pop_front();
            delete pCombinePackage;
        }
        //如果接收到的seq小于列表中的第一项，则表明是udp包已经处理了的（请求重发导致接收到重复包）忽略
        if (!packList.empty()) {
            const uint32_t after = this->seqReport;
            const uint32_t back = packageData->seq;
            if (time_after(after, back)) {
                ALOGI("HQ UDP Recv before seq = %d, cur=%d, ignore ...", back, after);
                return nullptr;
            }
        }
        //没找到，创建一个新的，并且需要排序
        pCombinePackage = new CombinePackage();
        pCombinePackage->AllocBuffer(packageData->slices * UDP_PACKENGTH);
        pCombinePackage->InitPackage(packageData, UDP_PACKENGTH);
        //插入到列表中，根据seq排序
        auto it = packList.begin();
        for (; it != packList.end(); it++) {
            const uint32_t itSeq = (*it)->GetSeq();
            const uint32_t seq = packageData->seq;
            if (time_after(itSeq, seq)) {
                break;
            }
        }
        packList.insert(it, pCombinePackage);
    }
    int64_t curTimestamp = timestamp();
    if (this->retryCount < COUNT_RETRY) {
        //1.判断是否有丢包
        if (this->seqReport != packList.front()->GetSeq()) {
            if (curTimestamp - resendTimestamp >= INTERVAL_RETRY) {
                resendTimestamp = curTimestamp;
                this->retryCount++;
                ALOGI("HQ UDP Request full pack start=%d, end=%d", this->seqReport,
                      packList.front()->GetSeq());
                RequestRangeSend(this->dataType, this->seqReport, packList.front()->GetSeq(), 0);
                //RequestSend(this->dataType, this->seqReport, 0x0fffffff);
            }
            return nullptr;
        }
        //判断在接收完整的包之前是否有未接受完整的
        if (FindFullPackage() > 0) {
            if (curTimestamp - resendTimestamp >= INTERVAL_RETRY) {
                resendTimestamp = curTimestamp;
                this->retryCount++;
                pCombinePackage = packList.front();
                uint32_t slicesMask[8];
                pCombinePackage->GetRequestSliceMask(slicesMask);
                RequestSend(pCombinePackage->GetDataType(), pCombinePackage->GetSeq(),
                            slicesMask);
                ALOGI("HQ UDP Resend seq=%d, slices=%d, received=%d",
                      pCombinePackage->GetSeq(),
                      pCombinePackage->GetPackageData()->slices,
                      pCombinePackage->GetReceivedPackages());
                return nullptr;
            }
        }
    } else {
        //请求超时，抛弃未接收完整的包
        while (!packList.empty()) {
            pCombinePackage = packList.front();
            this->seqReport = packList.front()->GetSeq() + 1;
            if (pCombinePackage->IsDone()) break;
            packList.pop_front();
            delete pCombinePackage;
        }
        this->retryCount = 0;
    }
    //将所有已经OK的发送出去（按顺序发送）
    while (!packList.empty()) {
        pCombinePackage = packList.front();
        if (!pCombinePackage->IsDone()) {
            break;
        }
        //ALOGI("HQ UDP Report seq=%d", pCombinePackage->GetPackageData()->seq);
        this->seqReport = pCombinePackage->GetPackageData()->seq + 1;
        this->retryCount = 0;
        packageData = pCombinePackage->GetPackageData();
        tcpClient->DoRecvUdpMessage(pCombinePackage->GetPackageData());
        packList.pop_front();
        delete pCombinePackage;
    }
    return nullptr;
}

/**
 * 请求重新发送UDP包
 * @param type  包类型，取值为 TYPE_MEDIA_VIDEODATA
 * @param seq   需要重发的包的序号
 * @param sliceMask  需要重发的sliceMask(丢失的)
 */
void UdpCombinePackage::CombineCache::RequestSend(int type, uint32_t seq, uint32_t *slicesMask) {
    UdpPackageReply udpResend;
    udpResend.version = sizeof(udpResend);
    udpResend.seq = seq;
    udpResend.type = type;
    //注意：是未接受到（需要重发）的掩码
    if (slicesMask) {
        memcpy(udpResend.slicesMask, slicesMask, sizeof(udpResend.slicesMask));
    } else {
        for (int i = 0; i < sizeof(udpResend.slicesMask) / sizeof(udpResend.slicesMask[0]); i++) {
            udpResend.slicesMask[i] = 0xffffffff;
        }
    }
    //ALOGI("HQ UDP Resend seq=%d, mask=%x, slices=%d", udpResend.seq,
    //      udpResend.sliceMask, (*it)->GetPackageData()->slices);
    tcpClient->Send(TYPE_UDP_REQUEST, &udpResend, sizeof(udpResend));
}

/**
 * 请求发送一定范围内的UDP包
 * @param type  包类型，取值为 TYPE_MEDIA_VIDEODATA
 * @param seqStart 开始的包
 * @param seqEnd  结束的包(重发时不包括)
 * @param flag 重发的标志。参考：UdpPackageReply2::flag
 */
void UdpCombinePackage::CombineCache::RequestRangeSend(int type, uint32_t seqStart, uint32_t seqEnd,
                                                       uint32_t flag) {
    UdpPackageReply2 request;
    request.version = sizeof(request);
    request.type = type;
    request.seqStart = seqStart;
    request.seqEnd = seqEnd;
    request.flag = flag;
    tcpClient->Send(TYPE_UDP_REQUEST2, &request, sizeof(request));
}

/**
 * 在list中查找接收到完整包
 * @return 找到第一个完整包的序号。-1表示未找到
 */
int UdpCombinePackage::CombineCache::FindFullPackage() {
    int index = 0;
    for (auto it = packList.begin(); it != packList.end(); it++) {
        if ((*it)->IsDone()) {
            return index;
        }
        index++;
    }
    return -1;
}