//
// Created by hq on 2019/8/21.
//
#include "Tcp.h"
#include "TcpClient.h"
#include "log.h"
#include <arpa/inet.h>
#include "TcpNativeInfo.h"
#include "Locker.h"

/**
 * Tcp Server检测到一个Client连接时，创建 TcpClient 实例
 */
TcpClient::TcpClient(TcpNativeInfo *nativeInfo, Tcp *tcp, int fd, struct sockaddr_in *addr)
        : seq(0), tcp(tcp), mReceivedAudioProcessor(this), mReceivedVideoProcessor(this),
          sendCacheManager(this) {
    this->nativeInfo = nativeInfo;
    this->running = true;
    this->fd_socket = fd;
    //设置接收到Client的Socket的发送超时
    struct timeval timeout = {3, 0};
    setsockopt(fd_socket, SOL_SOCKET, SO_SNDTIMEO, (char *) &timeout, sizeof(struct timeval));
    pthread_mutex_init(&mutex_write, NULL);
    memcpy(&this->addr, addr, sizeof(this->addr));
    //const char *ip = inet_ntoa(this->addr.sin_addr);
    mHeartbeatFlags = HBFLAG_CLIENTINFO;
    pthread_t tid;
    pthread_create(&tid, nullptr, threadRecv, this);
    //创建子线程，专门用于上报数据
    mReceivedAudioProcessor.Create(this->lenBufAudio);
    mReceivedVideoProcessor.Create(this->lenBufVideo);
    sendCacheManager.Create();
    mUdpCombinePackage = new UdpCombinePackage(this);
    CreateBufferVideo();
	CreateBufferAudio();
	mediadecoderinit();
}

/**
 * 如果是创建一个新的Client（主动连接Server），请使用此构造函数初始化
 */
TcpClient::TcpClient(TcpNativeInfo *nativeInfo)
        : seq(0), tcp(nullptr), mReceivedAudioProcessor(this), mReceivedVideoProcessor(this),
          sendCacheManager(this) {
    this->nativeInfo = nativeInfo;
    this->running = false;
    fd_socket = 0;
    pthread_mutex_init(&mutex_write, NULL);
    CreateBufferVideo();
	CreateBufferAudio();
}

TcpClient::~TcpClient() {
    Release(false);
    ReleaseBufferVideo();
	ReleaseBufferAudio();
    if (mUdpCombinePackage) {
        delete mUdpCombinePackage;
        mUdpCombinePackage = nullptr;
    }
    nativeInfo = nullptr;
    pthread_mutex_destroy(&mutex_write);
}

void TcpClient::Release(bool selfDis) {
    //if (this->fd_socket <= 0) return;
    this->addr.sin_addr.s_addr = 0;
    pthread_mutex_lock(&mutex_write);
    int fd = fd_socket;
    fd_socket = 0;
    this->running = false;
    if (fd > 0) {
        this->selfDisconnect = selfDis;
        shutdown(fd, SHUT_RDWR);
        if (tidRecv > 0) {
            pthread_join(tidRecv, nullptr);
            tidRecv = 0;
        }
    }
    if (udpClient) {
        delete udpClient;
        udpClient = nullptr;
    }
	if(avplayer)
	{
		delete avplayer;
	}
	if(aacdec)
	{
		delete aacdec;
	}
	if(pcmframe)
	{
		free(pcmframe);
	}
    mReceivedAudioProcessor.Destroy();
    mReceivedVideoProcessor.Destroy();
    sendCacheManager.Destroy();
    pthread_mutex_unlock(&mutex_write);
}

/**
 * 接收到数据后，各个Client自己处理数据
 * @param data
 * @param len
 */
void TcpClient::Put(uint8_t *UNUSED(data), size_t UNUSED(len)) {
    //TODO 接收到数据的处理。
    //注意：TCP有粘包的情况，需要处理
}

/**
 * 连接到目标服务器
 * 必须要满足下面条件
 * 1. 必须是使用第二种构造方式创建的(tcp==null)
 * 2. 当前必须处于未连接状态
 * @return  false为不成功
 */
bool TcpClient::Connect(const char *ip, int port) {
    if (tcp != nullptr) {
        return false;
    }
    if (fd_socket > 0) {
        return false;
    }
    mRecvTimeoutCounter = 0;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    inet_pton(AF_INET, ip, &addr.sin_addr);
    addr.sin_port = htons(port);
    //开始执行连接
    int sockfd = socket(AF_INET, SOCK_STREAM | SOCK_CLOEXEC, 0);
    if (sockfd <= 0) {
        return false;
    }
    //在局域网内连接时间应该比较短，避免默认的75秒连接超时
    fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) | O_NONBLOCK);
    int ret = connect(sockfd, (struct sockaddr *) &addr, sizeof(addr));
    if (ret < 0) {
        if (errno != EINPROGRESS) {
            close(sockfd);
            return false;
        }
        //局域网内，设置5秒超时
        struct timeval tm = {5, 0};
        fd_set wset, rset;
        FD_ZERO(&wset);
        FD_ZERO(&rset);
        FD_SET(sockfd, &wset);
        FD_SET(sockfd, &rset);
        int res = select(sockfd + 1, &rset, &wset, NULL, &tm);
        if (res <= 0) {
            //==0 is timeout; <0 is error
            close(sockfd);
            return false;
        } else {
            if (FD_ISSET(sockfd, &wset)) {
                printf("connect succeed.\n");
                fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL, 0) & ~O_NONBLOCK);
            } else {
                close(sockfd);
                return false;
            }
        }

    }
    this->fd_socket = sockfd;
    this->running = true;
    //设置Socket的发送缓冲区大小
    int len = sizeof(ret);
    if (0 == getsockopt(fd_socket, SOL_SOCKET, SO_RCVBUF, (void *) &ret, (socklen_t *) &len)) {
        if (ret < MAXSIZE_SOCKPACKAGE) {
            ret = MAXSIZE_SOCKPACKAGE;
            len = setsockopt(fd_socket, SOL_SOCKET, SO_SNDBUF, (void *) &ret, sizeof(ret));
        }
    }

    pthread_t tid;
    pthread_create(&tid, nullptr, threadRecv, this);
    //开启心跳包发送子线程
    pthread_create(&tid, nullptr, threadHeartbeat, this);
    //创建子线程，专门用于上报数据
    mReceivedAudioProcessor.Create(128 * 1024);
    mReceivedVideoProcessor.Create(1024 * 1024);
    sendCacheManager.Create();
    return true;
}

#if 0
static void DebugPack(SocketPackageData *packageData) {
    ALOGI("AAA headsize=%d, magic=%x, seq=%d, type=%d, slices=%d, curSlice=%d, time=%lld, dataSize=%d",
          SOCKPACKAGEHEAD_SIZE,
          packageData->magic, packageData->seq, packageData->type, packageData->slices,
          packageData->curSlice, packageData->time, packageData->dataSize);
}
#endif

/**
 * 发送数据
 * @param type 发送的数据类型，取值为 TYPE_NEW_CLIENT, ...
 * @param data 需要发送的数据
 * @param len  长度
 * @return  加入到缓冲区的长度。<0表示无效，比如说已经小虎销毁
 */
int TcpClient::Send(int type, void *data, int len) {
    if (!running || fd_socket <= 0) {
        return -1;
    }
    //如果支持UDP，则音视频数据通过UDP发送
    if (udpClient && udpSupportType) {
        int useUdp = 0;
        if (type == TYPE_MEDIA_VIDEODATA) {
            useUdp = udpSupportType & UDPSUPPORT_VIDEODATA;
        } else if (type == TYPE_MEDIA_AUDIODATA) {
            useUdp = udpSupportType & UDPSUPPORT_AUDIODATA;
        } else {
            useUdp = udpSupportType & UDPSUPPORT_OTHERDATA;
        }
        if (useUdp) {
            return udpClient->Send(type, data, len);
        }
    }
    return sendCacheManager.PutSendData(type, data, len);
}

/**
 * 仅仅发送媒体数据。
 * 数据从 bufferVideo/bufferAudio 中获取
 *
 * @param type 取值为 TYPE_MEDIA_VIDEODATA 或 TYPE_MEDIA_AUDIODATA
 * @param offset 偏移量
 * @param len  数据长度
 * @return  实际发送的数据的长度。<0表示错误
 */
int TcpClient::SendMediaData(int type, int offset, int len) {
    if (type != TYPE_MEDIA_VIDEODATA && type != TYPE_MEDIA_AUDIODATA) {
        return 0;
    }
    return Send(type, (type == TYPE_MEDIA_AUDIODATA ? bufferAudio : bufferVideo) + offset, len);
}

/**
 * 当不在需要发送视频时（停止投屏）
 * 可以主动调用这个接口，用于清空线程中保存的VIDEO/AUDIO数据
 * 避免有线程中缓存的数据发送
 */
void TcpClient::StopCtrl() {
    working = false;
	avplayer->hideUI();
	mediaDecoderReady = false;
	for (auto it = tcp->clientpalyerList.begin(); it != tcp->clientpalyerList.end(); it++) {
	 	if(*it == avplayer->ClientID)
	 	{
	 		printf("LCA delete List ClientID:%d\n",*it);
	        tcp->clientpalyerList.erase(it);
			tcp->ClientPlayernum--;
			break;
	 	}
	} 
	mReceivedAudioProcessor.ClearThreadData();
	mReceivedVideoProcessor.ClearThreadData();
	
    sendCacheManager.ClearAudioData();
    sendCacheManager.ClearVideoData();
}

/**
 * 接收Socket的线程
 */
void *TcpClient::threadRecv(void *p) {
    TcpClient *client = (TcpClient *) p;
    client->tidRecv = pthread_self();
    //pthread_detach(pthread_self());
    client->DoRecvLoop();
	client->fd_socket = -1;
    return nullptr;
}

/**
 * 上报到JAVA的线程
 */
void *TcpClient::threadHeartbeat(void *p) {
    TcpClient *client = (TcpClient *) p;
    client->hbToken = (long) timestamp();
    client->DoHeartbeatLoop(client->hbToken);
    return nullptr;
}


uint32_t TcpClient::SetUdpSupport(uint32_t udpSupport) {
    this->udpSupportType = udpSupport;
    //如果是作为服务端，需要通知到Client
    if (tcp) {
        UdpSupportMsg udpSupportMsg;
        udpSupportMsg.version = sizeof(UdpSupportMsg);
        udpSupportMsg.port = tcp->GetUdpListenPort();
        udpSupportMsg.sliceSize = UDP_PACKENGTH;
        udpSupportMsg.supportType = this->udpSupportType;
        Send(TYPE_UDP_SUPPORT, &udpSupportMsg, sizeof(UdpSupportMsg));
    }
    return this->udpSupportType;
}

void TcpClient::DoRecvLoop() {
    //PC端1920*1080视频数据一帧最大有1M，设置2M的缓存
    const int BUFSIZE = ((1024 + 1024) * 1024);
    int value;
    int total = 0;
    CombinePackage combinePackage;
    TcpNativeInfo *nativeInfo1 = this->nativeInfo;
    //某次接收的缓冲区，可以接收一整包数据
    const int RECV_BUFSIZE = MAXSIZE_SOCKPACKAGE + SOCKPACKAGEHEAD_SIZE;
    uint8_t *buffer = new uint8_t[RECV_BUFSIZE];
    //因为在分包处理时，每次处理完总是移动数据，使得[0]就是包头，所以可以直接强制转换
    SocketPackageData *packageData = (SocketPackageData *) buffer;
    //包括一整包的所有数据缓冲区，可能是分成多个包接收的
    combinePackage.AllocBuffer(BUFSIZE);
    //确保已经加入到TcpClient数组中
    if (tcp) {
        value = 100;
        while (--value > 0) {
            if (tcp->CheckTcpClient(this)) {
                break;
            }
            usleep(10 * 1000);
        }
    }
    total = sizeof(value);
    if (0 == getsockopt(fd_socket, SOL_SOCKET, SO_RCVBUF, (void *) &value, (socklen_t *) &total)) {
        if (value < MAXSIZE_SOCKPACKAGE) {
            //设置接收缓冲区
            value = MAXSIZE_SOCKPACKAGE;
            setsockopt(fd_socket, SOL_SOCKET, SO_RCVBUF, (void *) &value, sizeof(value));
        }
    }
    struct timeval timeout = {1, 0};
    setsockopt(fd_socket, SOL_SOCKET, SO_RCVTIMEO, (char *) &timeout, sizeof(struct timeval));
    //当前接收的位置
    int pos = 0;
    //通知Client支持UDP
    if (tcp) {
        SetUdpSupport(tcp->GetDefaultUdpSupport());
    } else {
        //上报 TYPE_NEW_CLIENT 消息。Server端的在接收到 TYPE_CLIENT_INFO 后才会上报
        sprintf((char *) buffer, "%s:%d", inet_ntoa(addr.sin_addr), addr.sin_port);
        nativeInfo1->JniMessageReport(this, TYPE_NEW_CLIENT, (char *) buffer, nullptr, 0, 0);
        newClientReported = true;
    }
    while (running) {
        if (pos < SOCKPACKAGEHEAD_SIZE || pos < SOCKPACKAGE_SIZE(packageData)) {
            //从socket中读取数据
            ssize_t num = recv(fd_socket, buffer + pos, RECV_BUFSIZE - pos, 0);
            //ALOGI("HQ TCP recv num = %d", num);
            if (num < 0) {
                //判断总的接收超时（HEARTBEAT也没有接收到）
                //15秒还没有接收到对方的Socket数据，认为连接中断了
                //请注意：15仅仅是PAPA同屏器端，TV端保持5秒不变
                if (++mRecvTimeoutCounter >= 5) {
                    break;
                }
                if (errno == EAGAIN || errno == EINTR) {
                    continue;
                }
            }
            mRecvTimeoutCounter = 0;
            if (!running || num <= 0) {
                break;
            }
            pos += num;
            continue;
        }
        if (packageData->curSlice == 0) {
            combinePackage.InitPackage(packageData, packageData->dataSize);
        } else {
            combinePackage.PutPackage(packageData);
        }
        if (combinePackage.IsDone()) {
            //已经收到足够完成的数据包了
            DoRecvMessage(combinePackage.GetPackageData());
            combinePackage.ResetData();
            //ALOGI("HQ Done Package. seq=%d", packageData->seq);
        }
        //抛弃已经处理好的数据
        int leave = pos - SOCKPACKAGE_SIZE(packageData);
        if (leave > 0) {
            memcpy(buffer, buffer + SOCKPACKAGE_SIZE(packageData), (size_t) leave);
        }
        pos = leave;
    }
    running = false;
    newClientReported = false;
	ALOGI("LCA DISCONNECT_CLIENT\n");
	if(working)
	{
		StopCtrl();
	}
	tcp->Clientnum--;
    nativeInfo1->JniMessageReport(this, TYPE_DISCONNECT_CLIENT,
                                  this->selfDisconnect ? "1" : nullptr, nullptr, 0, 0);
    delete[] buffer;
}

/**
 * 接收到一个数据包后做解析动作
 *
 * @param packageData 接收到的数据包
 */
void TcpClient::DoRecvMessage(SocketPackageData *packageData) {
    //ALOGI("HQ  Report type=0x%04x", packageData->type, packageData->dataSize);
    bool report = false;
    char buffer[64];
    //接收到数据后计数器复位。目前udp也会调用此函数
    mRecvTimeoutCounter = 0;
    ClientInfo *clientInfo;
    SocketHeartbeat hb = {0};
    switch (packageData->type) {
        case TYPE_CLIENT_NAME:
            //上报 TYPE_NEW_CLIENT 消息
            if (!newClientReported) {
                newClientReported = true;
                sprintf(buffer, "%s:%d", inet_ntoa(addr.sin_addr), addr.sin_port);
                nativeInfo->JniMessageReport(this, TYPE_NEW_CLIENT, (char *) buffer, nullptr, 0, 0);
            }
            nativeInfo->JniMessageReport(this, TYPE_CLIENT_NAME,
                                         (const char *) packageData->data, nullptr,
                                         0, 0);
            break;
        case TYPE_CLIENT_INFO:
            mHeartbeatFlags &= ~HBFLAG_CLIENTINFO;
            //上报 TYPE_NEW_CLIENT 消息
            if (!newClientReported) {
                newClientReported = true;
                sprintf(buffer, "%s:%d", inet_ntoa(addr.sin_addr), addr.sin_port);
                nativeInfo->JniMessageReport(this, TYPE_NEW_CLIENT, (char *) buffer, nullptr, 0, 0);
            }
            clientInfo = (ClientInfo *) packageData->data;
            nativeInfo->JniMessageReport(this, TYPE_CLIENT_NAME,
                                         (const char *) clientInfo->username, nullptr,
                                         0, 0);
            break;

        case TYPE_MEDIA_VIDEODATA:
            if (bufferVideo && lenBufVideo >= (int) packageData->dataSize) {
                //ALOGI("HQ  recv TYPE_MEDIA_VIDEODATA ...dataSize=%d", packageData->dataSize);
                mReceivedVideoProcessor.PutData(packageData);
            }
            break;
        case TYPE_MEDIA_AUDIODATA:
            if (bufferAudio && lenBufAudio >= (int) packageData->dataSize) {
                mReceivedAudioProcessor.PutData(packageData);
            }
            break;
        case TYPE_SOCKET_HEARTBEAT:
            //ALOGI("HQ  recv TYPE_SOCKET_HEARTBEAT ...");
            memcpy(&hb, packageData->data, sizeof(hb)<packageData->dataSize?sizeof(hb):packageData->dataSize);
            if (hb.rid == 0) {
                //是在Server端接收到了Client的消息
                hb.rid = hb.id;
                hb.msgFlags = mHeartbeatFlags;
                Send(TYPE_SOCKET_HEARTBEAT, &hb, sizeof(hb));
            } else {
                //Client接收到了Server返回的消息
                if (hb.msgFlags & HBFLAG_CLIENTINFO) {
                    //上报到上层处理
                    nativeInfo->JniMessageReport(this, TYPE_REQUEST_CLIENTINFO, nullptr, nullptr,
                                                 0, 0);
                }
            }
            break;
        case TYPE_UDP_SUPPORT: {
            UdpSupportMsg *udpSupportMsg = (UdpSupportMsg *) packageData->data;
            if (udpClient) {
                //更改UDPClient的地址
                udpClient->UpdateAddress(addr.sin_addr, udpSupportMsg->port);
            } else {
                udpClient = new UdpClient(this, addr.sin_addr, udpSupportMsg->port,
                                          udpSupportMsg->sliceSize);
            }
            this->udpSupportType = udpSupportMsg->supportType;
            break;
        }
        case TYPE_UDP_REQUEST: {
            //需要重新发送UDP包
            if (udpClient) {
                UdpPackageReply *udpReply = (UdpPackageReply *) packageData->data;
                //ALOGI("HQ UDP Request seq=%d, sliceMask=%x\n", udpReply->seq, udpReply->sliceMask);
                udpClient->ReSend(udpReply->type, udpReply->seq, udpReply->slicesMask);
            }
            break;
        }
        case TYPE_UDP_REQUEST2: {
            if (udpClient) {
                UdpPackageReply2 *udpReply = (UdpPackageReply2 *) packageData->data;
                //ALOGI("HQ UDP Request Range start=%d, end=%d, flag=%x\n", udpReply->seqStart,
                //      udpReply->seqEnd, udpReply->flag);
                udpClient->ReSendRange(udpReply->type, udpReply->seqStart, udpReply->seqEnd,
                                       udpReply->flag);
            }
            break;
        }
        case TYPE_NETWORK_ECHOTEST: {
            NetworkEchoTest *pEchoMsg = (NetworkEchoTest *) packageData->data;
            if (pEchoMsg->rid) {
                //是接收到回复的，打印出网络耗时
                uint64_t t = (uint64_t) timestamp();
                int d1 = (int) ((t - pEchoMsg->time1) / 1000);
                int d2 = (int) ((t - pEchoMsg->time2) / 1000);
                ALOGI("HQ Network Echo: d1=%d, d2=%d", d1, d2);
            } else {
                //Reply
                NetworkEchoTest msg;
                msg.version = sizeof(msg);
                msg.mid = 0;
                msg.rid = pEchoMsg->mid;
                msg.time1 = pEchoMsg->time1;
                msg.time2 = pEchoMsg->time2;
                Send(TYPE_NETWORK_ECHOTEST, &msg, sizeof(msg));
            }
            break;
        }
        case TYPE_MIRROR_RESOLUTION:
        case TYPE_MIRROR_RESOLUTION2:
            //检测到开始，清空数据
            sendCacheManager.ClearAudioData();
            sendCacheManager.ClearVideoData();
			if(!working)
			{
				tcp->ClientPlayernum++;
				printf("ClientPlayernum = %d,clientid:%d\n",tcp->ClientPlayernum,avplayer->ClientID);
				SendRequestQuality();
				tcp->clientpalyerList.push_back(avplayer->ClientID);
				printf("LCA Create List ClientID:%d\n",avplayer->ClientID);
			}
			avplayer->showUI();
            working = true;
			mediaDecoderReady = true;
            report = true;
            break;
        case TYPE_MIRROR_STOP:
            StopCtrl();
            report = true;
            break;
        case TYPE_TV_SCREEN_STOP:
            bzero(bufferVideo,2 * 1024 * 1024);
            report = true;
            break;
        default:
            report = true;
    }
    if (report) {
        nativeInfo->JniMessageReport(this, packageData->type, nullptr, packageData->data, 0,
                                     packageData->dataSize);
    }
}

/**
 * 定期发送心跳到Server
 * 只有Client才会启用，在 {@link #Connect}中自动创建线程
 *
 * 目前发现一个Bug：
 * 1. 客户端（手机端、同屏器等）连接到TV，TV上被链接成功。显示连接终端数（例如显示1）
 * 2. 客户端非正常断开。TV端Socket一直处于连接状态，因此一直显示1
 *
 * 使用下面的方式来解决：
 * 1. 客户端 Connect() TV端成功后，开启一个线程
 * 2. 线程中每隔1秒发送一个HEARTBEAT包
 * 3. 在 recv() 中设置一个超时。当超时未接收到数据包认为客户端已经断开
 * 4. 如果接收到的是 HEARTBEAT，则回复一个 HEARTBEAT（注意：仅仅需要对方主动发出的）
 *
 * 注意下面的条件：
 * 1. 仅仅是Client端，调用 Connect() 才会主动发起 HEARTBEAT
 * 2. recv() 接收到心跳，需要判断是否是对方回复的，如果是对方回复的则不需要再次发出
 */
void TcpClient::DoHeartbeatLoop(long token) {
    int fdSocket = this->fd_socket;
    int id = 1;
    while (true) {
        sleep(1);
        if (token != hbToken) {
            break;
        }
        if (!running) {
            break;
        }
        if (fdSocket != this->fd_socket) {
            break;
        }
        //发送心跳数据
        SocketHeartbeat hb;
        memset(&hb, 0x00, sizeof(hb));
        hb.version = sizeof(SocketHeartbeat);
        hb.id = ++id;
        Send(TYPE_SOCKET_HEARTBEAT, &hb, sizeof(hb));
    }
}

void *TcpClient::threadReportData(void *p) {
    ThreadDataProcessor *pProcessor = (ThreadDataProcessor *) p;
    TcpClient *pTHIS = pProcessor->pTcpClient;
    pTHIS->DoThreadReportData(pProcessor);
    return nullptr;
}

void TcpClient::DoThreadReportData(ThreadDataProcessor *pProcessor) {
    while (true) {
        if (!running) {
            break;
        }

        ThreadSendData *data = pProcessor->GetData();
        if (data != nullptr) {
            //需要将数据发送出去
            if (data->type == TYPE_MEDIA_AUDIODATA) {
				if (!this->aacDecoderReady) {
                    int cnt = 100;
                    while (this->running && this->working && !this->aacDecoderReady &&
                           (--cnt > 0)) {
                        usleep(100 * 1000);
                        ALOGI("HQ NOT audioDecoderReady ...");
                        continue;
                    }
                }
				/*
                if (bufferAudio && lenBufAudio >= data->dataSize) {
                    //ALOGI("HQ  recv TYPE_MEDIA_AUDIODATA ...dataSize=%d", packageData->dataSize);
                    memcpy(bufferAudio, data->data, data->dataSize);
                }
                nativeInfo->JniMessageReport(this, TYPE_MEDIA_AUDIODATA, nullptr, bufferAudio,
                                             data->time,
                                             data->dataSize);
				*/
				long ts = avplayer->get_sys_runtime(2);
				if (ts - playAudioFrameTimestamp >= 300) {
					//已经超过了播放时间
					playAudioFrameCount = 0;
				}
				playAudioFrameTimestamp = ts;
				playAudioFrameCount++;
				
				int ret = aacdec->decoder_one_aac(data->data,data->dataSize,pcmframe,&pcmsize);
				if(ret > 0)
				{
					printf("decode error:%d\n",ret);
				}
				else
				{
					/*if(aacdec->fp_pcm)
					 {
					 	fwrite(pcmframe,1, pcmsize,aacdec->fp_pcm);
					 }*/
					 avplayer->FeedOnePcmFrame((unsigned char*)pcmframe,pcmsize,playAudioFrameCount < 20);
			 		 memset(pcmframe,0,pcmsize);
				}
				
            }
			else if (data->type == TYPE_MEDIA_VIDEODATA) {
                if (!this->mediaDecoderReady) {
                    int cnt = 100;
                    while (this->running && this->working && !this->mediaDecoderReady &&
                           (--cnt > 0)) {
                        usleep(100 * 1000);
                        ALOGI("HQ NOT mediaDecoderReady ...");
                        continue;
                    }
                }
               // if (working) {
               		/*
                    if (bufferVideo && lenBufVideo >= data->dataSize) {
                        memcpy(bufferVideo, data->data, data->dataSize);
                    }
                    nativeInfo->JniMessageReport(this, TYPE_MEDIA_VIDEODATA, nullptr, bufferVideo,
                                                 data->time,
                                                 data->dataSize);
                    bzero(bufferVideo,2 * 1024 * 1024);
                	*/
                	avplayer->FeedOneH264Frame(data->data,data->dataSize);
                    //printf("上报反向投屏数据 size = %d\n",data->dataSize);
                //}
            }

            free(data);
        }
    }
}

TcpClient::ThreadDataProcessor::ThreadDataProcessor(TcpClient *p) {
    pTcpClient = p;
}

void TcpClient::ThreadDataProcessor::Create(int sizeCollections) {
    pthread_mutex_init(&mutex_thread_data, NULL);
    pthread_create(&tidThreadSendData, nullptr, threadReportData, this);
}

void TcpClient::ThreadDataProcessor::Destroy() {
    if (tidThreadSendData != 0) {
        pthread_t tid = tidThreadSendData;
        tidThreadSendData = 0;
        event.Signal();
        pthread_join(tid, nullptr);
        pthread_mutex_lock(&mutex_thread_data);
        for (auto it = threadDataList.begin(); it != threadDataList.end(); it++) {
            free(*it);
        }
        threadDataList.clear();
        pthread_mutex_unlock(&mutex_thread_data);
        pthread_mutex_destroy(&mutex_thread_data);
    }
}

void TcpClient::ThreadDataProcessor::ClearThreadData(){
	 pthread_mutex_lock(&mutex_thread_data);
     threadDataList.clear();
     pthread_mutex_unlock(&mutex_thread_data);
}

/**
 * 获取数据。
 * 注意：会阻塞线程
 * @return null表示未获取到数据，需要循环再次获取
 * 注意：掉哟处需要执行 free(retval)
 */
TcpClient::ThreadSendData *TcpClient::ThreadDataProcessor::GetData() {
    ThreadSendData *data = nullptr;
    if (threadDataList.empty()) {
        event.Wait(1000);
    }
    pthread_mutex_lock(&mutex_thread_data);
    if (threadDataList.size() > 0) {
        data = threadDataList.front();
        threadDataList.pop_front();
    }
    pthread_mutex_unlock(&mutex_thread_data);
    return data;
}

void TcpClient::ThreadDataProcessor::PutData(SocketPackageData *packageData) {
    //bool full = threadDataList.size() >= maxListCount;
    bool full = false;
    if (full) {
        //数据满了，不能加入到
        if (packageData->type == TYPE_MEDIA_VIDEODATA) {
            //ALOGI("HQ Ignore VideoData .... ");
        } else {
            //ALOGI("HQ Ignore AudioData .... ");
        }
        return;
    }
    ThreadSendData *data = nullptr;

    data = (ThreadSendData *) malloc(
            sizeof(ThreadSendData) + packageData->dataSize);
    data->type = packageData->type;
    data->dataSize = packageData->dataSize;
    data->time = packageData->time;
    data->capacity = packageData->dataSize;
    memcpy(data->data, packageData->data, packageData->dataSize);
    pthread_mutex_lock(&mutex_thread_data);
    threadDataList.push_back(data);
    pthread_mutex_unlock(&mutex_thread_data);

    //通知到线程触发上报
    event.Signal();
}


TcpClient::SendCacheManager::SendCacheManager(TcpClient *p) {
    pTcpClient = p;
    tidWrite = 0;
    sendBuffer = new uint8_t[MAXSIZE_SENDSOCKPACKAGE + SOCKPACKAGEHEAD_SIZE];
}

TcpClient::SendCacheManager::~SendCacheManager() {
    delete sendBuffer;
    sendBuffer = nullptr;
}

void TcpClient::SendCacheManager::Create() {
    pthread_mutex_init(&mutex, nullptr);
    pthread_create(&tidWrite, nullptr, SendCacheManager::threadSendData, this);
}

void TcpClient::SendCacheManager::Destroy() {
    if (tidWrite > 0) {
        event.Signal();
        pthread_join(tidWrite, nullptr);
        tidWrite = 0;
        pthread_mutex_lock(&mutex);
        //释放所有的List中数据
        for (auto it = cmdList.begin(); it != cmdList.end(); it++) {
            free(*it);
        }
        cmdList.clear();
        for (auto it = audioList.begin(); it != audioList.end(); it++) {
            free(*it);
        }
        audioList.clear();
        for (auto it = videoList.begin(); it != videoList.end(); it++) {
            free(*it);
        }
        videoList.clear();
        pthread_mutex_unlock(&mutex);
        pthread_mutex_destroy(&mutex);
    }
}

/**
 * 将待发送的数据放到缓冲列表中
 * @param type 待发送数据类型
 * @param data 数据内容
 * @param len 数据长度
 */
int TcpClient::SendCacheManager::PutSendData(int type, void *data, int len) {
    if (!pTcpClient->running) {
        return 0;
    }
    pthread_mutex_lock(&mutex);
    std::list<ThreadSendData *> *pList = nullptr;
    if (type == TYPE_MEDIA_VIDEODATA) {
        if (videoList.size() < 30) {
            pList = &videoList;
        }
    } else if (type == TYPE_MEDIA_AUDIODATA) {
        if (videoList.size() < 200) {
            pList = &audioList;
        }
    } else {
        if (videoList.size() < 30) {
            pList = &cmdList;
        }
    }
    if (pList != nullptr) {
        ThreadSendData *ele = (ThreadSendData *) malloc(sizeof(ThreadSendData) + len);
        ele->type = type;
        ele->time = timestamp();
        if(data && len>0) {
            ele->dataSize = len;
            memcpy(ele->data, data, len);
        } else {
            ele->dataSize = 0;
        }
        pList->push_back(ele);
    }
    pthread_mutex_unlock(&mutex);
    event.Signal();
    return len;
}

/**
 * 从 SendCacheManager 获取将要写入到Socket的数据
 * 此处会对需要发送的优先级排序，优先发送命令控制指令
 * @return null表示找不到需要发送的数据
 * 注意：
 * 1 多实例.该函数会阻塞
 * 2.调用处必须要 free(retval)
 */
TcpClient::ThreadSendData *TcpClient::SendCacheManager::GetSendData() {
    if (!pTcpClient->running) {
        return nullptr;
    }
    std::list<ThreadSendData *> *pList = nullptr;
    if (cmdList.empty() && audioList.empty() && videoList.empty()) {
        event.Wait(1000);
    }
    Locker locker(&mutex);
    if (!cmdList.empty()) {
        pList = &cmdList;
    } else if (!audioList.empty()) {
        pList = &audioList;
    } else {
        pList = &videoList;
    }
    if (pList->empty()) {
        return nullptr;
    }
    ThreadSendData *data = pList->front();
    pList->pop_front();
    return data;
}

void TcpClient::SendCacheManager::ClearAudioData() {
    Locker locker(&mutex);
    audioList.clear();
}

void TcpClient::SendCacheManager::ClearVideoData() {
    Locker locker(&mutex);
    videoList.clear();
}

void *TcpClient::SendCacheManager::threadSendData(void *p) {
    SendCacheManager *pCacheManager = (SendCacheManager *) p;
    pCacheManager->DoThreadSendData();
    return nullptr;
}

void TcpClient::SendCacheManager::DoThreadSendData() {
    uint32_t seq = 0;

    while (true) {
        if (!pTcpClient->running) {
            break;
        }
        ThreadSendData *pSendData = GetSendData();
        if (pSendData != nullptr) {
            //写入到Socket
            if (pTcpClient->fd_socket > 0) {
                pthread_mutex_lock(&pTcpClient->mutex_write);
                //ALOGI("AAA Send len=%d", len);
                uint8_t *d = (uint8_t *) pSendData->data;
                int len = pSendData->dataSize;
                const int slices = (len <= 0 ? 1 : (len + MAXSIZE_SENDSOCKPACKAGE - 1) /
                                                   MAXSIZE_SENDSOCKPACKAGE);
                SocketPackageData *packageData = (SocketPackageData *) sendBuffer;
                packageData->magic = SOCKMAGIC_HEAD;
                packageData->seq = seq++;
                packageData->type = (uint16_t) pSendData->type;
                packageData->time = pSendData->time;
                packageData->slices = (uint8_t) slices;
                for (int s = 0; s < slices; s++) {
                    const int leave =
                            len >= MAXSIZE_SENDSOCKPACKAGE ? MAXSIZE_SENDSOCKPACKAGE : len;
                    packageData->curSlice = (uint8_t) s;
                    packageData->dataSize = (uint32_t) leave;
                    if (len > 0) {
                        memcpy(packageData->data, d, (size_t) leave);
                    }
                    //ALOGI("AAA Send seq=%d, dataSize=%d, magic=%x", packageData->seq,
                    //      packageData->dataSize, packageData->magic);
                    ssize_t sd = send(pTcpClient->fd_socket, sendBuffer,
                                      SOCKPACKAGE_SIZE(packageData),
                                      MSG_NOSIGNAL);
                    if (sd <= 0) {
                        //发送失败或者超时
                        break;
                    }
                    d += leave;
                    len -= leave;
                }
                pthread_mutex_unlock(&pTcpClient->mutex_write);
            }
            free(pSendData);
        }
    }
}


void TcpClient::SendRequestQuality() {
	RequestQuality requestQuality = { 0 };
	requestQuality.version = sizeof(RequestQuality);
	if(tcp->ClientPlayernum == 1)
	{
		requestQuality.quality = 5;
		requestQuality.width = 1920;
		requestQuality.height = 1080;
	}
	else if(tcp->ClientPlayernum == 2)
	{
		requestQuality.quality = 4;
		requestQuality.width = 1600;
		requestQuality.height = 900;
	}
	else if(tcp->ClientPlayernum == 3)
	{
		requestQuality.quality = 3;
		requestQuality.width = 1600;
		requestQuality.height = 900;
	}
	else if(tcp->ClientPlayernum == 4)
	{
		requestQuality.quality = 2;
		requestQuality.width = 1366;
		requestQuality.height = 768;
	}
	else
	{
		requestQuality.quality = 0;
	}
	requestQuality.quality = 5;
	printf("LCA QUALITY quality:%d,wxh[%d x %d]\n",requestQuality.quality,requestQuality.width,requestQuality.height);
	requestQuality.video_fps = 0;
	Send(TYPE_REQUEST_QUALITY, &requestQuality, sizeof(requestQuality));
}

void TcpClient::mediadecoderinit()
{
	avplayer = new AVPlayer(this);
	avplayer->ClientID = tcp->ClientId;
	int ret = avplayer->InitVideo();
	if(ret)
	{
		printf("vedio deocder init error\n");
	}
	ret= avplayer->InitAudio(0,0);
	if(ret)
	{
		printf("audio palyer init error\n");
	}
	
	aacdec = new AACDecoder();
	pcmframe=(INT_PCM *)malloc(1024*5);
	ret = aacdec->audioDecoderInit();
	if(ret)
	{
		printf("audio deocder init error\n");
	}
	else
	{
		aacDecoderReady = true;
	}
	
	return;
}

