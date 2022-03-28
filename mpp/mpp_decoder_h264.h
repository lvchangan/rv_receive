#ifndef MPP_DECODER_H264_H
#define MPP_DECODER_H264_H

#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <string.h>


#include "vpu.h"
#include "rk_mpi.h"
#include "rk_type.h"
#include "vpu_api.h"
#include "mpp_err.h"
#include "mpp_task.h"
#include "mpp_meta.h"
#include "mpp_frame.h"
#include "mpp_buffer.h"
#include "mpp_packet.h"
#include "rk_mpi_cmd.h"

#include "mpp_mem.h"
#include "mpp_env.h"
#include "mpp_common.h"


#include <list>
#include "Event.h"

//#define SZ_4K	0x00001000

#define PKT_SIZE    SZ_4K
#define CODEC_ALIGN(x, a)   (((x)+(a)-1)&~((a)-1))


#define two_ResYSize  (3840*1080)
#define two_ResYUVSize (3840*1080*1.5)

class Codec {
public:
    Codec(TcpClient *TcpClient);
    ~Codec();
    int init(MppCodingType type,
             int src_w, int src_h, int display);
    int deinit();
    int decode_one_pkt(unsigned char *buf, int size);
    int dump_mpp_frame_to_file(MppFrame frame, FILE *fp);
	void ReadYUV(unsigned char* ResBuf,unsigned char* PreBuf, int resstart, int prestart, int resoffset, int preoffset, int size, int height);
    double get_frm_rate() {
        return mFrmRate;
    }
	int mEos;
	int mID;
	int mNum;
private:
    int mFps;   
    int mDisPlay;
    
    int srcW;	//解码的宽度
    int srcH;	//解码的高度
    int srcYsize;
    int srcYUVsize;

    RK_S64 mTimeS;
    RK_S64 mTimeE;
    RK_S64 mTimeDiff;
    double mFrmRate;

    int mFrmCnt;	//解码帧数
    FILE *mFout;
    MppCtx mCtx;
    MppApi *mApi;
    MppBuffer mBuffer;
    MppBufferGroup mFrmGrp;
	
public:
	pthread_mutex_t mutex_YUVSplicing;
	
	TcpClient *pTcpClient;
};
	
#endif


