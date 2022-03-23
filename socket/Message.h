/**
 * PC、同屏器、手机、TV发送Socket数据包时，统一对每条消息加入文件头
 * 此处定义每条消息（包）的消息
 */

#ifndef SOCKET_MESSAGE_H
#define SOCKET_MESSAGE_H

/**
 * 定义消息类型
 * 需要与JAVA层的对应。JNI层上报到JAVA处理时，也会附带消息类型
 */

/**
 * TV端接收到客户端连接
 */
const int TYPE_NEW_CLIENT = 0x0001;
/**
 * TV端客户端断开连接
 */
const int TYPE_DISCONNECT_CLIENT = 0x0002;

// 
// Client通知Server当前客户端的名称
// 一般取手机上设置的名称
// 
const int TYPE_CLIENT_NAME = 0x0003;

//
//  * Server发送此消息通知Client
//  * <p>
//  * data: String, Server的投屏码
//
const int TYPE_SCREEN_CODE = 0x0004;

/**
* Server和Client互相发送此消息通知版本
* <p>
* data: int, 版本号
 * 已废弃，请使用 TYPE_CLIENT_INFO, ClientInfo::appver, hidver
*/
const int TYPE_VERSION_CODE = 0x0005;
/**
 * Server发送是否支持遥控通知Client
 * <p>
 * data: int, 是否支持，0（不支持），1（支持）
 */
const int TYPE_FUNCTION_CONTROLLER = 0x0006;
/**
 * Socket的心跳包
 * 不会上报到JAVA层，Socket模块内部自己处理了
 */
const int TYPE_SOCKET_HEARTBEAT = 0x0007;
/**
  * 手机端请求TV端投屏
  */
const int TYPE_TV_SCREEN_START = 0x0008;
/**
  * TV投屏终止
  */
const int TYPE_TV_SCREEN_STOP = 0x0009;
/**
 * TV投屏失败，需要先关闭镜像
 */
const int TYPE_TV_SCREEN_FAIL = 0x000A;
/**
 * TCP连接后发送客户端类型，默认客户端为Android
 * 如果客户端为PC方或者同屏器硬件
 * 需要发送此消息通知TV端修改客户端类型
 * 已废弃，请使用TYPE_CLIENT_INFO,  ClientInfo::ostype
 */
const int TYPE_CLIENT_MODE = 0x000B;
/**
 * 向TV端询问是否为主讲人
 *
 * TV端会使用同一个MessageType返回结果，1为true，0为false
 */
const int TYPE_ASK_SPEAKER = 0x000C;
/**
  * 不再是主讲人
  */
const int TYPE_NO_LONGER_SPEAKER = 0x000D;
/**
  * 投屏已锁定
  */
const int TYPE_MIRROR_LOCK = 0x000E;
/**
  * 请求客户端名称
  * 用于MSG_CLIENT_NAME消息丢失时
  */
const int TYPE_REQUEST_NAME = 0x000F;
/**
 * ClientInfo 消息，用于替代以前的 TYPE_CLINET_NAME, TYPE_VERSION_CODE, TYPE_CLIENT_MODE
 */
const int TYPE_CLIENT_INFO = 0x0010;

/**
 * 当Server支持UDP协议时，发送此消息告知
 * UdpSupportMsg
 */
const int TYPE_UDP_SUPPORT = 0x0020;
/**
 * 当接收到UDP数据包后，使用此消息回应
 * 消息体为 UdpPackageReply
 */
const int TYPE_UDP_ACK = 0x0021;
/**
 * 请求重新发送某个UDP包
 * 消息体为 UdpPackageReply
 */
const int TYPE_UDP_REQUEST = 0x0022;
/**
 * 当接收到UDP数据包后，使用此消息回应
 * 用来回应seq的范围
 * 消息体为 UdpPackageReply2
 */
const int TYPE_UDP_ACK2 = 0x0022;
/**
 * 请求重新发送某个UDP包
 * 用来请求一定范围内的包
 * 消息体为 UdpPackageReply2
 */
const int TYPE_UDP_REQUEST2 = 0x0023;
/**
 * 投屏模式为NONE，一般为关闭时发送
 */
const int TYPE_PLAYER_MODE_NONE = 0x1001;
/**
 * 投屏模式为镜像
 */
const int TYPE_PLAYER_MODE_MIRROR = 0x1002;
/**
 * 投屏模式为视频
 */
const int TYPE_PLAYER_MODE_VIDEO = 0x1003;
/**
 * 投屏模式为音频
 */
const int TYPE_PLAYER_MODE_AUDIO = 0x1004;
/**
 * 镜像方式音频的格式。
 * 可选，如果未接收到此消息就有音频数据，默认为 PCM原始数据, 44.1K, 16bit, Stereo
 * |序号|字节数| 说明|
 * | 0  |  4   | 当前的配置版本。一般为 sizeof()操作
 * | 4  |  4   | 当前的文件格式。默认为0，表示PCM
 * | 8  |  4   | channels 通道个数。1表示单声道,2表示立体声
 * | 12 |  4   | SampleRate采样率。一般为 44100 或 48000
 * | 16 |  4   | BitsPerSample采样位数。取值为8或者16
 * | 20 |  4   | AvgBytesPerSecond 每秒平均字节数。这个数值是可以计算出来的
 * 上面的版本， sizeof()=24
 */
const int TYPE_MIRROR_AUDIOFORMAT = 0x2000;
/**
 * 镜像视频的分辨率。Client在推流前告知服务器视频的分辨率
 * int[2]转byte[]
 * 
 * @Deprecated 请使用 {@link #TYPE_MIRROR_RESOLUTION2} 替代
 */
const int TYPE_MIRROR_RESOLUTION = 0x2001;
/**
 * 视频数据
 */
const int TYPE_MEDIA_VIDEODATA = 0x2002;
/**
 * 音频数据
 */
const int TYPE_MEDIA_AUDIODATA = 0x2003;
/**
 * Client向Server发送视频数据
 */
const int TYPE_VIDEO_FRAME = 0x2004;
/**
 * Client向Server发送音频数据
 */
const int TYPE_AUDIO_FLUSH = 0x2005;
/**
 * Client向Server发送视频链接
 */
const int TYPE_VIDEO_SEND_URL = 0x2006;
/**
 * Client停止投屏
 */
const int TYPE_MIRROR_STOP = 0x2007;
/**
 * Server向指定Client请求投屏
 * 20210209: 扩展此消息。消息内容为 RequestMirror
 */
const int TYPE_REQUEST_MIRROR = 0x2008;
/**
 * Server关闭指定Client投屏
 */
const int TYPE_INTERRUPT_MIRROR = 0x2009;
/**
  * 发送视频旋转信息
  * data: int，旋转角度（0，90，180，270）
  */
const int TYPE_MIRROR_VIDEO_ROTATION = 0x200A;
/**
 * 镜像视频的分辨率，镜像视频要使用的VIEW类型（镜像SurfaceView，相机TextureView）
 * 用以兼容相机，代替之前MSG_MIRROR_RESOLUTION
 * int[5]转byte[]
 * int[0]: width
 * int[1]: height
 * int[2]: viewType, 1=Surface; 2=Texture
 * int[3](可选): VideoDataType, 0=H264, 1=H265
 * int[4](可选): AudioDataType, 0=Auto，需要发送 AACFORMT, 1=AAC
 */
const int TYPE_MIRROR_RESOLUTION2 = 0x200B;
/**
 * 镜像投屏音频如果采用AAC格式时，必须发送此消息通知TV端音频格式参数
 * 未收到此消息，按照PCM格式解码
 *
 * @Deprecated 请使用 TYPE_MIRROR_RESOLUTION2 来指定AAC音频
 */
const int TYPE_MIRROR_AAC_MEDIA_FORMAT = 0x200C;
/**
 * TV端控制声音是否播放（静音），反向通知到PC端
 * data: int32，0表示静音；!=0为正常声音播放
 */
const int TYPE_AUDIO_CONTROL = 0x200D;
/**
 * 接收端向视频编码发送端请求编码画质
 * 当TV端同时播放多个时(例如四分屏)，此时TV端受限于网络带宽和CPU性能，会导致严重的花屏、丢帧情况
 * 此时TV端可以通过此消息通知到Client，请求Client降低码流或编码分辨率
 *
 * RequestQuality
 */
const int TYPE_REQUEST_QUALITY = 0x200E;

/**
 * 接收端向Client请求ClientInfo消息
 * 在Client连接后会自动发送ClientInfo消息，但改消息可能会丢失
 * 当Server未收到此消息后，在心跳时会回复 HBFLAG_CLIENTINFO
 * 此时Client接收到此回复，会使用该消息上报到上层，上层再重新发送 TYPE_CLIENTINFO
 * 注意：该消息仅仅为内部使用，不通过Socket传输
 */
const int TYPE_REQUEST_CLIENTINFO = 0x200F;

/**
 * Client主动共享文件到TV端，TV端播放手机共享出来的文件
 * 内容为{@link cn.ktc.jkf.fileshare.FilePlayMessageContent}
 */
const int TYPE_START_PLAYFILE = 0x2101;
/**
 * 停止共享文件消息
 */
const int TYPE_STOP_PLAYFILE = 0x2102;
/**
 * 切换共享文件消息
 * data：int，文件编号
 */
const int TYPE_SWITCH_PLAYFILE = 0x2103;
/**
 * 共享文件播放时长消息
 * data：int，播放时长（ms）
 */
const int TYPE_DURATION_PLAYFILE = 0x2104;
/**
 * 共享文件播放暂停消息
 * data：int：播放（0），暂停（1）
 */
const int TYPE_PLAY_PAUSE_PLAYFILE = 0x2105;
/**
 * 共享文件声音调整消息
 * data：int：减小（0），增加（1）
 */
const int TYPE_VOLUME_PLAYFILE = 0x2106;
/**
  * 共享文件播放完成消息
  */
const int TYPE_PLAY_COMPLETION_PLAYFILE = 0x2107;
/**
  * 共享图片旋转消息
  * data：int：角度（0，90，180，270）
  */
const int TYPE_ROTATE_PLAYFILE = 0x2108;
/**
 * 共享文档成功
 */
const int TYPE_FILE_SHARE_SUCCESS_PLAYFILE = 0x2109;
/**
 * 共享文档失败
 */
const int TYPE_FILE_SHARE_FAIL_PLAYFILE = 0x210A;
/**
 * 播放、退出PPT时的消息
 * Windows端开始播放PPT和退出播放PPT会使用此消息通知到TV
 * data: int,  0=退出PPT播放（不在播放中）
 */
const int TYPE_PPT_PLAY = 0x210B;
/**
 * Client向Server发送视频的开始暂停命令
 */
const int TYPE_VIDEO_PLAY_OR_PAUSE = 0x3001;
/**
 * Client向Server发送视频的时间进度调整
 */
const int TYPE_VIDEO_SEEK = 0x3002;
/**
 * Client向Server请求视频的播放状态
 */
const int TYPE_VIDEO_GET_INFO_PLAYING = 0x3003;
/**
 * Client向Server请求视频的播放时间进度
 */
const int TYPE_VIDEO_GET_INFO_POS = 0x3004;
/**
 * Client向Server请求视频文件的时间
 */
const int TYPE_VIDEO_GET_INFO_DUR = 0x3005;
/**
 * 发送按键消息
 * data：int，按键值
 */
const int TYPE_CONTROLLER_KEY_CLICK = 0x4001;
/**
 * 发送鼠标按键消息
 */
const int TYPE_CONTROLLER_MOUSE_CLICK = 0x4002;
/**
 * 发送鼠标移动消息
 */
const int TYPE_CONTROLLER_MOUSE_MOVE = 0x4003;
/**
 * 发送鼠标拖动按下消息
 */
const int TYPE_CONTROLLER_MOUSE_DRAG_DOWN = 0x4004;
/**
 * 发送鼠标拖动抬起消息
 */
const int TYPE_CONTROLLER_MOUSE_DRAG_UP = 0x4005;
/**
 * 发送文本输入消息
 * data：string，消息内容
 */
const int TYPE_CONTROLLER_INPUT_TEXT = 0x4006;
/**
 * 反向投屏时，手机端分辨率
 */
const int MSG_TOUCH_SCREEN_RESOLUTION = 0x4007;
/**
 * 反向投屏触摸按下操作
 */
const int MSG_TOUCH_SCREEN_DOWN = 0x4008;
/**
 * 反向投屏触摸移动操作
 */
const int MSG_TOUCH_SCREEN_MOVE = 0x4009;
/**
 * 反向投屏触摸抬起操作
 */
const int MSG_TOUCH_SCREEN_UP = 0x400A;
/**
 * 手机端申请TV端音量
 */
const int MSG_CONTROLLER_APPLY_VOLUME = 0x400B;
/**
 * TV端发送音量
 */
const int MSG_CONTROLLER_TV_VOLUME = 0x400C;
/**
 * 手机端发送音量值
 */
const int MSG_CONTROLLER_VOLUME_NUMBER = 0x400D;
/**
 * 发送鼠标的绝对位置
 * data:int[4]，取值如下：
 * [0]: ABS_X X绝对坐标值
 * [1]: ABS_Y Y绝对坐标值
 * [2]: X_SCREEN PC端屏幕分辨率
 * [3]: Y_SCREEN PC端屏幕分辨率
 */
const int TYPE_CONTROLLER_MOUSE_ABS_POS = 0x4100;
/**
 * 定义鼠标指针的图标
 * 一定是一个32位的Bitmap数据
 * 数据结构如下所示：
 * int: version: 当前版本。默认为1
 * int: xHotspot
 * int: yHotspot
 * int: width: 图标Bitmap的宽度
 * int: height: 图标Bitmap的高度
 * int: dataSize: 实际数据的大小
 * int: headSize: data中前面Bitmap文件头的大小。偏移这些后就是纯的RGBA数据了
 * int: [2]保留，凑齐头为32字节
 * byte[32-]: 具体的图标RGBA的数据
 */
const int TYPE_CONTROLLER_MOUSE_CURSOR_ICON = 0x4101;
/**
 * ECHO测试。接收到后立即回复，用于测试网络延时
 */
const int TYPE_NETWORK_ECHOTEST = 0x4200;

/**
 * 定义每包数据
 * 定义下面数据结构时请注意字节对齐问题
 * int64必须要定义在 8 字节对齐位置，否则arm64上会自动对齐，造成32位与64位下不一致
 */
#define SOCKMAGIC_HEAD      0x21000012
typedef struct SocketPackageData_s {
    /**
     * 包头，必须是 MAGIC_HEAD
     */
    uint32_t magic;
    /**
     * 当前包的序号。没发送一个包序号增加
     * 注意：由于TV端是作为服务端，因此各个Client的序号可能不是连续的（TODO: 待定）
     */
    uint32_t seq;
    /**
     * 数据包的类型，取值为 TYPE_NEW_CLIENT, ...
     * 也作为cmdId
     */
    uint16_t type;
    /**
     * 如果需要分包发送的话，总共分成几个包
     */
    uint8_t slices;
    /**
     * 当前是第几个包。从序号0开始
     */
    uint8_t curSlice;
    /**
     * 该包的数据大小。
     * 注意：不是整包的数据大小
     */
    uint32_t dataSize;
    /**
     * 发送时的当前时间（微妙）
     */
    int64_t time;
    /**
     * 定义空data数组，便于直接获取数据缓冲指针
     */
    uint8_t data[0];
} SocketPackageData;

#define SOCKPACKAGEHEAD_SIZE    (int)sizeof(SocketPackageData)
#define SOCKPACKAGE_SIZE(pack)  (int)(pack->dataSize+SOCKPACKAGEHEAD_SIZE)

int SendSockData(int sock_fd, uint32_t *pseq, int type, uint8_t *data, int len);

/**
 * 定义采用UDP发送的数据
 */

/**
 * UDP包最大的大小。用于在代码中定义缓冲区
 */
#define UDP_MAXLENGTH   (64*1024)
/**
 * 每次UDP发送的数据大小。必须要<=UDP_MAXLENGTH
 * 仅仅是实际的数据大小，不包括 SocketPackageData 头
 * 请注意：由于 SocketPackageData 中 slices 使用 uint8_t 保存，因此分包不能太小，
 * 否则 当数据包大于1M时(256*4K=1M)会引发异常
 */
#define UDP_PACKENGTH   (32*1024)
/**
 * 通知Client，支持UDPServer
 * TYPE_UDP_SUPPORT
 */
#define UDPSUPPORT_VIDEODATA    (1<<0)  //TYPE_MEDIA_VIDEODATA 使用UDP发送
#define UDPSUPPORT_AUDIODATA    (1<<1)  //TYPE_MEDIA_AUDIODATA 使用UDP发送
#define UDPSUPPORT_OTHERDATA    (1<<2)  //其他数据包使用UDP发送
typedef struct UdpSupportMsg_s {
    /**
     * 当前的版本
     */
    int32_t version;
    /**
     * UDP监听的端口
     */
    int32_t port;
    /**
     * UDP发送分片包的大小
     */
    int32_t sliceSize;
    /**
     * 支持哪些包从UDP发送
     * 按位与。==0表示所有的包不使用UDP发送
     * 默认为 UDPSUPPORT_VIDEODATA|UDPSUPPORT_AUDIODATA
     *
     */
    uint32_t supportType;
} UdpSupportMsg;

/**
 * 请求重新发送该UDP报
 * TYPE_UDP_REQUEST
 */
typedef struct UdpPackageReply_s {
    int32_t version;
    /**
     * 需要发送的包的类型
     */
    int32_t type;
    /**
     * 包的序号
     */
    uint32_t seq;
    /**
     * 包中的分片序号。按位与
     * 由于最大分包有256，因此需要8个int32来保存
     */
    uint32_t slicesMask[8];
} UdpPackageReply;

/**
 * 一定范围内的UDP包
 */
#define UDPRANGE_START  (1<<0)
#define UDPRANGE_END    (1<<1)
typedef struct UdpPackageReply2_s {
    int32_t version;
    /**
     * 需要发送的包的类型
     */
    int32_t type;
    /**
     * 包的起始序号
     */
    uint32_t seqStart;
    /**
     * 包的结束序号。重发时不包括这个序号
     */
    uint32_t seqEnd;
    /**
     * 标志位，按位取值
     * (1<<0): seqStart无效，表示从Client已缓存的列表第一个开始
     * (1<<1): seqEnd无效，表示从seqStart一直到已缓存你的最后一个
     */
    uint32_t flag;
} UdpPackageReply2;
/**
 * TYPE_REQUEST_MIRROR 中的数据内容
 * 当前版本下与 RequestQuality 中的一致，仅仅是不同的结构体
 */
typedef struct RequestMirror_s {
    int32_t version;
    /**
     * 当前可展示的Surface宽高。编码器会根据此数值按等比例缩放
     */
    int32_t video_width;
    int32_t video_height;
    /**
     * 调节编码画质
     * -1: 保持当前画质
     * 0: 由编码端自动控制
     * 1-5： 总共5级画质，5为最高画质
     */
    int32_t video_quality;
    /**
     * 请求视频的帧率
     * 0(默认): 不变
     * >0: 请求需要设置的帧率。注意：Client端需要自己控制帧率范围
     * -1: 加大帧率
     * -2: 降低帧率。比如说TV端受性能限制解码缓慢，需要请求降低帧率
     */
    int32_t video_fps;
} RequestMirror;

/**
 * 请求编码质量控制
 * TYPE_REQUEST_QUALITY
 */
typedef struct RequestQuality_s {
    int32_t version;
    /**
     * 当前可展示的Surface宽高。编码器会根据此数值按等比例缩放
     */
    int32_t width;
    int32_t height;
    /**
     * 调节编码画质
     * -1: 保持当前画质
     * 0: 由编码端自动控制
     * 1-5： 总共5级画质，5为最高画质
     */
    int32_t quality;
    /**
     * 请求视频的帧率
     * 0(默认): 不变
     * >0: 请求需要设置的帧率。注意：Client端需要自己控制帧率范围
     * -1: 加大帧率
     * -2: 降低帧率。比如说TV端受性能限制解码缓慢，需要请求降低帧率
     */
    int32_t video_fps;
} RequestQuality;

/**
 * 用于测试网络延时，接收到接收到后立即回复
 * TYPE_NETWORK_ECHOTEST
 *
 * 注意：为了模拟视频数据，在实际发送时应该发送较大数据包（填充数据）
 * 回复时无需附加额外填充数据
 */
typedef struct NetworkEchoTest_s {
    int32_t version;
    /**
     * 发送方的ID。用于标识
     */
    int32_t mid;
    /**
     * 发送时设置为0，回复设置为mid
     * 如果!=0表示是接收到回复的，无需再次回复
     */
    int32_t rid;
    /**
     * 保存时间戳。当接收到回复时可以计算出数据包发送/接收之间的耗时
     */
    uint64_t time1;
    uint64_t time2;
} NetworkEchoTest;

typedef struct ClientInfo_s {
    int32_t version;
    /**
     * 当前OS类型，取值为：1:Windows; 2:Mac; 3:Android TV; 4:TypeC; 5: Android Phone
     */
    int32_t ostype;
    /**
     * 当前OS的版本
     */
    char osver[32];
    /**
     * 当前OS的名称
     */
    char osname[64];
    /**
     * Host上软件版本
     */
    int32_t appver;
    /**
     * HID固件的版本。==0表示未使用PAPA同屏器投屏(为软投屏)
     */
    int32_t hidver;
    /**
     * 当前登录的用户名。Android TV上统一为Android
     * 与消息 TYPE_CLIENT_NAME 名称相同
     */
    char username[64];
} ClientInfo;
/**
 * 心跳消息。以前是放到 TcpClient 类中定义，现在扩展功能
 */
/**
 * 请求发送 ClientInfo 消息
 */
#define HBFLAG_CLIENTINFO   0x00000001
typedef struct SocketHeartbeat_s {
    /**
     * 心跳包的版本，总是为 sizeof(SocketHeartbeat)
     */
    int32_t version;
    /**
     * 心跳包ID
     */
    int32_t id;
    /**
     * 针对某个心跳包的回复。==0表示主动发出的; !=0表示回复，此时不需要再次回复
     * Client发送出去时为0
     */
    int32_t rid;
    /**
     * 请求发送某些消息
     */
    uint32_t msgFlags;
} SocketHeartbeat;

#endif //SOCKET_MESSAGE_H
