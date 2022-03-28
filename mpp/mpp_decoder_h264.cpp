//
// Created by root on 17-11-1.
//
#include "TcpClient.h"
#include "mpp_decoder_h264.h"

using namespace std;

Codec::Codec(TcpClient *TcpClient) : 
				 mFps(0),
                 mEos(0),
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
    srcW = src_w;
    srcH = src_h;
	srcYsize = src_w * src_h;
	srcYUVsize = src_w * src_h *1.5;
		
	printf("LCA:wxh[%dx%d] display:%d\n",src_w,src_h,display);

	/*
	char outfilename[256];
	sprintf(outfilename,"/data/out_%d.yuv",mID);
	printf("LCA outfilename = %s\n",outfilename);
    mFout = fopen(outfilename, "wb+");
    if (!mFout) {
        printf("failed to open output file.\n");
        return -1;
    }
	*/
	if(!pTcpClient->mFYUVout)
	{
		pTcpClient->mFYUVout = fopen("YUVSpl.yuv", "wb+");
	    if (!pTcpClient->mFYUVout) {
	        printf("failed to open output file.\n");
	        return -1;
	    }
		else 
		{
			printf("LCA open YUVSpling file success.\n");
		}
	}
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

	int hor_stride = CODEC_ALIGN(srcW, 16);
    int ver_srride = CODEC_ALIGN(srcH, 16);
	printf("hor_stride x ver_srride [%d x %d]\n",hor_stride,ver_srride);
	mpp_buffer_get(mFrmGrp, &mBuffer, hor_stride * ver_srride * 3 / 2);

	if(pTcpClient->YUVSplicingBuffer == NULL)
	{
		pTcpClient->YUVSplicingBuffer = (unsigned char *)malloc(two_ResYUVSize);
	}
	
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
	//printf("LCA:write file wxh[%dx%d],hxv[%dx%d],fmt:0x%x\n",width,height,h_stride,v_stride,fmt);
    if(!buffer)
    {
    	printf("write error\n");
        return -2;
    }

    base = (RK_U8 *)mpp_buffer_get_ptr(buffer);

	int Y_start_section = 0;/*预处理图片Y分量存入目标区域的起始区域*/
	int UV_start_section = 0;

	int File_offset = 0;/*预处理文件偏移值*/
	int i_combine = 0, j_combine = 0, k_combine = 0;/*控制循环*/

	if(mID == 0)
	{
		Y_start_section = 0;
		UV_start_section = two_ResYSize;
	}
	else if(mID == 1)
	{
		Y_start_section = srcW;
		UV_start_section = two_ResYSize + srcW;
	}
	else if(mID == 2)
	{


	}
	else if(mID == 3)
	{


	}
	
	if(pTcpClient->mFYUVout && pTcpClient->YUVSplicingBuffer)
	{
		ReadYUV(pTcpClient->YUVSplicingBuffer, base , Y_start_section, 0, 2 * srcW, h_stride, srcW, srcH);
		ReadYUV(pTcpClient->YUVSplicingBuffer, base , UV_start_section,h_stride*v_stride, 2 * srcW, h_stride, srcW, srcH/2);
		fwrite(pTcpClient->YUVSplicingBuffer,1,two_ResYUVSize,pTcpClient->mFYUVout);
	}
	
	/*
    {
        RK_U32 i;
        RK_U8 *base_y = base;
        RK_U8 *base_uv = base + h_stride * v_stride;

        for (i = 0; i < height; i++, base_y += h_stride)
            fwrite(base_y, 1, width, fp);
        for (i = 0; i < height / 2; i++, base_uv += h_stride)
 			fwrite(base_uv, 1, width, fp);
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
                    printf("decoder require buffer w:h [%d:%d] stride [%d:%d] buf_size %d",
                            width, height, hor_stride, ver_stride, buf_size);

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


