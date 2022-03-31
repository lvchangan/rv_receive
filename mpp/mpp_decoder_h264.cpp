//
// Created by root on 17-11-1.
//
#include "TcpClient.h"
#include "Tcp.h"
#include "mpp_decoder_h264.h"

using namespace std;

Codec::Codec(TcpClient *TcpClient) : 
				 mFps(0),
                 mID(0),
                 mDisPlay(0),
                 mFrmCnt(0),
                 mFrmRate(0.0),
                 mFout(NULL),
                 mCtx(NULL),
                 mApi(NULL),
                 mFrmGrp(NULL),
                 pTcpClient(TcpClient)
{
	
}

Codec::~Codec() {
	deinit();
	
    if (mCtx) {
        mpp_destroy(mCtx);
        mCtx = NULL;
    }

    if (mBuffer) {
        mpp_buffer_put(mBuffer);
        mBuffer = NULL;
    }

    if (mFrmGrp) {
        mpp_buffer_group_put(mFrmGrp);
        mFrmGrp = NULL;
    }

    if (mFout) {
        fclose(mFout);
        mFout = NULL;
    }
}

int Codec::init(MppCodingType type,
                int src_w, int src_h, int display) 
{
    int ret = 0;
    int x, y, i;
    RK_U32 need_split = 0;
    MpiCmd mpi_cmd = MPP_CMD_BASE;
    MppParam param = NULL;
	
    mDisPlay = display;

	/*	
	char outfilename[256];
	sprintf(outfilename,"/data/yuvscale_%dW.yuv",mID);
	filewidth = 1920;
	//printf("LCA outfilename = %s\n",outfilename);
    mFout = fopen(outfilename, "wb+");
    if (!mFout) {
        printf("failed to open output file.\n");
        return -1;
    }
	*/
	
	// 初始化解码器上下文，MppCtx MppApi
    ret = mpp_create(&mCtx, &mApi);
    if (ret != MPP_OK) {
        printf("failed to exec mpp_create ret %d", ret);
        return -2;
    }

	//配置解码器
    //*  - 解码文件需要 split 模式
    mpi_cmd = MPP_DEC_SET_PARSER_SPLIT_MODE;
    param = &need_split;
    ret = mApi->control(mCtx, mpi_cmd, param);
    if (ret != MPP_OK) {
        printf("failed to control MPP_DEC_SET_PARSER_SPLIT_MODE.\n");
        return  -3;
    }

    ret = mpp_init(mCtx, MPP_CTX_DEC, type);
    if (ret != MPP_OK) {
        printf("failed to exec mpp_init.\n");
        return -4;
    }
	/*
	int hor_stride = CODEC_ALIGN(srcW, 16);
    int ver_srride = CODEC_ALIGN(srcH, 16);
	printf("hor_stride x ver_srride [%d x %d]\n",hor_stride,ver_srride);
	mpp_buffer_get(mFrmGrp, &mBuffer, hor_stride * ver_srride * 3 / 2);
	*/
    return 0;
}

static RK_S64 get_time() {
    struct timeval tv_date;
    gettimeofday(&tv_date, NULL);
    return (RK_S64)tv_date.tv_sec * 1000000 + (RK_S64)tv_date.tv_usec;
}

void Codec::ReadYUV(unsigned char* ResBuf,unsigned char* PreBuf, int resstart, int prestart, int resoffset, int preoffset, int size, int height)
{
	int k;
	for (k = 0; k < height; k++)
	{
		memmove(ResBuf + resstart + k * (resoffset), PreBuf + prestart + k * (preoffset), size);//注意这里用memmov不用strncpy
	}
}

void Codec::GetDisplayerResolvingPower(int Mirrorwidth,int Mirrorheight)
{
	if(mNum == 1 || mNum == 2)
	{
	 	TVCanvaswidth = pTcpClient->tcp->TVScreenwidth / mNum;
		TVCanvasheight = pTcpClient->tcp->TVScreenheight;
	}
	else if(mNum == 3 || mNum == 4)
	{
		TVCanvaswidth = pTcpClient->tcp->TVScreenwidth / 2;
		TVCanvasheight = pTcpClient->tcp->TVScreenheight / 2;
	}
	float scaleMirror = (float)Mirrorwidth / (float)Mirrorheight;		//投屏的原图比例
	float scaleTVCanvas = (float)TVCanvaswidth / (float)TVCanvasheight;	//TV端画布的比例

	if (scaleTVCanvas < scaleMirror) {
        //TV端画布高度比例更高一些，应该以width做基准缩放
        displayerwidth = TVCanvaswidth;
        displayerheight = (int)(displayerwidth / scaleMirror);
	}
    else {
        displayerheight = TVCanvasheight;
        displayerwidth = displayerheight * scaleMirror;
    }

	displayerwidth = (displayerwidth + 1) & ~0x01;
    displayerheight = (displayerheight + 1) & ~0x01;
}

int Codec::libyuv_scale(uint8 *src_buf,int src_w,int src_h,uint8 *dst_buf,int dst_w,int dst_h,enum libyuv::FilterMode filtering)
{
	uint8 * i420buffer = (uint8 *)malloc(src_w * src_h * 1.5);
	libyuv::NV12ToI420(&src_buf[0]			 ,src_w,
			   &src_buf[src_w * src_h]       ,src_w,
			   &i420buffer[0]				 ,src_w,
			   &i420buffer[src_w * src_h]	 ,src_w>>1,
			   &i420buffer[(src_w*src_h*5)>>2] ,src_w>>1,
			   src_w,src_h
		);

	libyuv::I420Scale(
		  &i420buffer[0],                          src_w,
          &i420buffer[src_w * src_h],            src_w >> 1,
          &i420buffer[(src_w * src_h * 5) >> 2], src_w >> 1,
          src_w, src_h,
          &dst_buf[0],                         dst_w,
          &dst_buf[dst_w * dst_h],            dst_w >> 1,
          &dst_buf[(dst_w * dst_h * 5) >> 2], dst_w >> 1,
          dst_w, dst_h,
          filtering
		);
	free(i420buffer);
	return 0;
}

int Codec::dump_mpp_frame_to_file(MppFrame frame, FILE *fp)
{
    RK_U32 width = 0;
    RK_U32 height = 0;
    RK_U32 h_stride = 0;
    RK_U32 v_stride = 0;
    MppFrameFormat fmt = MPP_FMT_YUV420P;
    MppBuffer buffer = NULL;
    RK_U8 *base = NULL;
	/*
    if (!fp || !frame)
        return -1;
	*/
    width = mpp_frame_get_width(frame);
    height = mpp_frame_get_height(frame);
    h_stride = mpp_frame_get_hor_stride(frame);
    v_stride = mpp_frame_get_ver_stride(frame);
    fmt = mpp_frame_get_fmt(frame);
    buffer = mpp_frame_get_buffer(frame);
    if(!buffer)
    {
    	printf("write error\n");
        return -2;
    }

	if(oldmNum != mNum || oldsrcW != width || oldsrcH != height)
	{
		GetDisplayerResolvingPower(width,height);
		oldmNum = mNum;
		oldsrcW = width;
		oldsrcH = height;
		pthread_mutex_lock(&mutex_YUVSplicing);	
		memset(pTcpClient->tcp->YUVSplicingBuffer,0,pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight);
		memset(pTcpClient->tcp->YUVSplicingBuffer+pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight,
			128,pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight/4);
		memset(pTcpClient->tcp->YUVSplicingBuffer+pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight*5/4,
			128,pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight/4);
		pthread_mutex_unlock(&mutex_YUVSplicing);
		printf("ClientID:%d width x height:[%d x %d] displayerwidth x displayerheight:[%d x %d] mNum:%d\n",pTcpClient->ClientID,width,height,displayerwidth,displayerheight,mNum);
	}
	
    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);
	
	uint8 * scalesrcbuffer = (uint8 *)malloc(width * height * 1.5);
	uint8 * scaledstbuffer = (uint8 *)malloc(displayerwidth * displayerheight * 1.5);
	int total =0;
    RK_U8 *base_y = base;
    RK_U8 *base_uv = base + h_stride * v_stride;
	
    for (int i = 0; i < height; i++, base_y += h_stride)
    {  	
		memcpy(scalesrcbuffer+total,base_y,width);
		total = total + width;
    }
    for (int i = 0; i < height / 2; i++, base_uv += h_stride)
    {
    	memcpy(scalesrcbuffer+total,base_uv,width);
		total = total + width;
    }

	libyuv_scale(scalesrcbuffer,width,height,scaledstbuffer,displayerwidth,displayerheight,(enum libyuv::FilterMode)3);
	//fwrite(scaledstbuffer,1,displayerwidth * displayerheight * 1.5,mFout);
	free(scalesrcbuffer);

	int playerid = 0;
	for (auto it = pTcpClient->tcp->clientpalyerList.begin(); it != pTcpClient->tcp->clientpalyerList.end(); it++) {
		playerid++;
		if(*it == pTcpClient->ClientID)
		{
			break;
	 	}
    }
	
	int Y_start_section = 0;  //预处理图片Y分量存入目标区域的起始区域
	int U_start_section = 0;
	int V_start_section = 0;
	int blackoffset = 0;
	int resoffset =  pTcpClient->tcp->TVScreenwidth;
	if(TVCanvasheight == displayerheight)
	{
		blackoffset = (TVCanvaswidth - displayerwidth)/2;
	}	
	else if(TVCanvaswidth == displayerwidth)
	{
		blackoffset = (TVCanvasheight - displayerheight)/2 * pTcpClient->tcp->TVScreenwidth;
	}
	if(mNum == 1 || mNum == 2)
	{
		if(playerid == 1)
		{
			Y_start_section = blackoffset;
			if(TVCanvasheight == displayerheight)
			{
				U_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight + blackoffset/2;
				V_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight * 1.25 + blackoffset/2 ;
			}
			else if(TVCanvaswidth == displayerwidth)
			{
				U_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight + blackoffset/4;
				V_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight *1.25 + blackoffset/4;
			}
		}
		else if(playerid == 2 && mNum == 2)
		{
			Y_start_section = TVCanvaswidth + blackoffset;
			if(TVCanvasheight == displayerheight)
			{
				U_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight+TVCanvaswidth/2+blackoffset/2;
				V_start_section = pTcpClient->tcp->TVScreenwidth*pTcpClient->tcp->TVScreenheight*1.25+TVCanvaswidth/2+blackoffset/2 ;
			}
			else if(TVCanvaswidth == displayerwidth)
			{
				U_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight+TVCanvaswidth/2+blackoffset/4;
				V_start_section = pTcpClient->tcp->TVScreenwidth * pTcpClient->tcp->TVScreenheight *1.25+TVCanvaswidth/2+blackoffset/4;
			}
		}
	}

	if(pTcpClient->mFYUVout && pTcpClient->tcp->YUVSplicingBuffer)
	{
		//printf("LCA:write file wxh[%dx%d],hxv[%dx%d],fmt:0x%x\n",width,height,h_stride,v_stride,fmt);
pthread_mutex_lock(&mutex_YUVSplicing);		
ReadYUV(pTcpClient->tcp->YUVSplicingBuffer,scaledstbuffer,Y_start_section, 0, resoffset, displayerwidth, displayerwidth, displayerheight);
ReadYUV(pTcpClient->tcp->YUVSplicingBuffer,scaledstbuffer,U_start_section,displayerwidth*displayerheight, resoffset/2, displayerwidth/2, displayerwidth/2, displayerheight/2);
ReadYUV(pTcpClient->tcp->YUVSplicingBuffer,scaledstbuffer,V_start_section,displayerwidth*displayerheight*1.25, resoffset/2, displayerwidth/2, displayerwidth/2, displayerheight/2);
		fwrite(pTcpClient->tcp->YUVSplicingBuffer,1,pTcpClient->tcp->YUVSplicingBufferSize,pTcpClient->mFYUVout);
pthread_mutex_unlock(&mutex_YUVSplicing);
	}
	

	free(scaledstbuffer);

	
/*
void Codec::ReadYUV(unsigned char* ResBuf,unsigned char* PreBuf, int resstart, int prestart, int resoffset, int preoffset, int size, int height)
{
	int k;
	for (k = 0; k < height; k++)
	{
		memmove(ResBuf + resstart + k * (resoffset), PreBuf + prestart + k * (preoffset), size);//注意这里用memmov不用strncpy
	}
}
*/

	/*
	else if(mNum == 2)
	{
		if(mID == 0)
		{
			Y_start_section = 0;
			UV_start_section = two_ResYSize;
		}
		else if(mID == 1)
		{
			Y_start_section = width;
			UV_start_section = two_ResYSize + width;
		}
	}
	else if(mNum == 3)
	{
		if(mID == 0)
		{
			Y_start_section = 0;
			UV_start_section = three_ResYSize;
		}
		else if(mID == 1)
		{
			Y_start_section = width;
			UV_start_section = three_ResYSize + width;
		}
		else if(mID == 2)
		{
			Y_start_section = two_ResYUVSize;
			UV_start_section = two_ResYUVSize + three_ResYSize;
		}
	}
	else if(mID == 4)
	{
		if(mID == 0)
		{
			Y_start_section = 0;
			UV_start_section = four_two_ResYSize;
		}
		else if(mID == 1)
		{
			Y_start_section = width;
			UV_start_section = four_two_ResYSize + width;
		}
		else if(mID == 2)
		{
			Y_start_section = four_two_ResYUVSize;
			UV_start_section = four_two_ResYUVSize + four_two_ResYSize;
		}
		else if(mID == 3)
		{
			Y_start_section = four_two_ResYUVSize + width;
			UV_start_section = four_two_ResYUVSize + four_two_ResYSize + width;
		}
	}
	*/
	/*
	if(width == 1600 && filewidth != width)
	{
		if(mFout)
		{
			fclose(mFout);
			char outfilename[256];
			sprintf(outfilename,"/data/yuv_%d_1600x900.yuv",mID);
			filewidth = 1600;
    		mFout = fopen(outfilename, "wb+");
		}
	}
	
	printf("LCA:write file wxh[%dx%d],hxv[%dx%d],fmt:0x%x\n",width,height,h_stride,v_stride,fmt);
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_uv = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, mFout);
        for (i = 0; i < height / 2; i++, base_uv += h_stride)
 			fwrite(base_uv, 1, width, mFout);
    }
	*/
    return 0;
}

int Codec::decode_one_pkt(unsigned char *buf, int size) {

    RK_U32 pkt_done = 0;	//完成送包
    RK_U32 pkt_eos  = 0;
    RK_U32 err_info = 0;
	
    MPP_RET ret = MPP_OK;
    MppPacket packet = NULL;
    MppFrame  frame  = NULL;

    ret = mpp_packet_init(&packet, buf, size);
    //mpp_packet_set_pts(packet, av_packet->pts);

	do {
        RK_S32 times = 5;
        // send the packet first if packet is not done
        if (!pkt_done) {
            ret = mApi->decode_put_packet(mCtx, packet);
            if (MPP_OK == ret)
                pkt_done = 1;
        }

		// then get all available frame and release
        do {
            RK_S32 get_frm = 0;
            RK_U32 frm_eos = 0;

            try_again:
            ret = mApi->decode_get_frame(mCtx, &frame);
            if (MPP_ERR_TIMEOUT == ret) {
                if (times > 0) {
                    times--;
                    usleep(2000);
                    goto try_again;
                }
                printf("decode_get_frame failed too much time\n");
            }
			
			if (MPP_OK != ret) {
                printf("decode_get_frame failed ret %d\n", ret);
				break;
            }

			if (frame) {
                if (mpp_frame_get_info_change(frame)) {
                    RK_U32 width = mpp_frame_get_width(frame);
                    RK_U32 height = mpp_frame_get_height(frame);
                    RK_U32 hor_stride = mpp_frame_get_hor_stride(frame);
                    RK_U32 ver_stride = mpp_frame_get_ver_stride(frame);
                    RK_U32 buf_size = mpp_frame_get_buf_size(frame);

                    printf("decode_get_frame get info changed found\n");
                    printf("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d\n",width, height, hor_stride, ver_stride, buf_size);

					if (NULL == mFrmGrp) {
						 /* If buffer group is not set create one and limit it */
                        ret = mpp_buffer_group_get_internal(&mFrmGrp, MPP_BUFFER_TYPE_ION);
                        if (ret) {
                            printf("get mpp buffer group failed ret %d.\n", ret);
                            break;
                        }
						/* Set buffer to mpp decoder */
						ret = mApi->control(mCtx, MPP_DEC_SET_EXT_BUF_GROUP, mFrmGrp);
						if (ret) {
                            printf("set buffer group failed ret %d\n", ret);
                            break;
                        }
                   }
					else{
	                        /* If old buffer group exist clear it */
	                        ret = mpp_buffer_group_clear(mFrmGrp);
	                        if (ret) {
	                            printf("clear buffer group failed ret %d\n", ret);
	                            break;
	                        }
                	}
					
       			    ret = mpp_buffer_group_limit_config(mFrmGrp, buf_size, 24);
                    if (ret) {
                        printf("limit buffer group failed ret %d\n", ret);
                        break;
                    }
					
                    /*
                     * All buffer group config done. Set info change ready to let
                     * decoder continue decoding
                     */              
                    mApi->control(mCtx, MPP_DEC_SET_INFO_CHANGE_READY, NULL);
					
				} 
				else {
						err_info = mpp_frame_get_errinfo(frame) | mpp_frame_get_discard(frame);
                        if (err_info) {
                             printf("decoder_get_frame get err info:%d discard:%d.\n",
                                mpp_frame_get_errinfo(frame), mpp_frame_get_discard(frame));
                        }

                        if (mDisPlay) {
							//printf("decode_get_frame ID:%d get srcFrm %d.\n", mID, mFrmCnt++);
                            //if (mFout) {
                                	dump_mpp_frame_to_file(frame, mFout);
                            //}
                        }
                }	
                frm_eos = mpp_frame_get_eos(frame);
                mpp_frame_deinit(&frame);

                frame = NULL;
                get_frm = 1;
            }
			
			// try get runtime frame memory usage
            /*if (data->frm_grp) {
                size_t usage = mpp_buffer_group_usage(data->frm_grp);
                if (usage > data->max_usage)
                    data->max_usage = usage;
            }*/

            // if last packet is send but last frame is not found continue
            if (pkt_eos && pkt_done && !frm_eos) {
                usleep(10000);
                continue;
            }

            if (frm_eos) {
                printf("found last frame\n");
                break;
            }

            /*if (data->frame_num > 0 && data->frame_count >= data->frame_num) {
                data->eos = 1;
                break;
            }*/

            if (get_frm)
                continue;
            break;
			
        }while(1);
		
		/*if (data->frame_num > 0 && data->frame_count >= data->frame_num) {
            data->eos = 1;
            printf("reach max frame number %d\n", data->frame_count);
            break;
        }*/

        if (pkt_done)
            break;

        usleep(3000);
	

	}while(1);
	
	mpp_packet_deinit(&packet);
    return ret;
}



int Codec::deinit() {
    int ret = 0;

    ret = mApi->reset(mCtx);
    if (ret != MPP_OK) {
        printf("failed to exec mApi->reset.\n");
        return -1;
    }
	return 0;
}


