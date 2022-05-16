#include "audiodecoder.h"

AACDecoder::AACDecoder()
{
	aac_stream_info = NULL;
	
	aac_ctx = (AAC_Context *)malloc(sizeof(AAC_Context));
	if(!aac_ctx)
	{
		printf("aac_ctx malloc error\n");
	} 

	fp_pcm = fopen("/data/out.pcm","wb+");
	if(!fp_pcm)
	{
		printf("open fp_pcm error\n");
	}
}

AACDecoder::~AACDecoder()
{
	if(fp_pcm)
	{
		fclose(fp_pcm);
	}
	
	if(aac_ctx)
	{
		free(aac_ctx);
		aac_ctx = NULL;
	}
}

int AACDecoder::audioDecoderInit()
{
	aac_ctx->handle = aacDecoder_Open(TT_MP4_ADTS, 1);  
    if (aac_ctx->handle == NULL) {  
        printf("aacDecoder open faild!\n");  
        return -1;  
    }  
/*
	int ret = aacDecoder_ConfigRaw(aac_ctx->handle, conf, &conf_len);  
    if (ret != AAC_DEC_OK) {  
        printf("Unable to set configRaw\n");  
         return -2;   
    }  
*/
	aac_stream_info = aacDecoder_GetStreamInfo(aac_ctx->handle);  
    if (aac_stream_info == NULL) {  
        printf("aacDecoder_GetStreamInfo failed!\n");  
        return -3;
    }  
    printf("1>stream info: channel = %d ample_rate = %d rame_size = %d t = %d itrate = %d\n",aac_stream_info->channelConfig, aac_stream_info->aacSampleRate, \  
            aac_stream_info->aacSamplesPerFrame, aac_stream_info->aot, aac_stream_info->bitRate); 

	return 0;
}

int AACDecoder::decoder_one_aac(unsigned char* bufferAAC, int buf_sizeAAC,INT_PCM *bufferPCM, int *buf_sizePCM)
{
	int decoderErr = AAC_DEC_NOT_ENOUGH_BITS;
	int pcm_pkt_size = 0;
	UINT aac_pkt_size = buf_sizeAAC;
	UINT valid_size =  buf_sizeAAC;
	UCHAR *input_buf[1] = {bufferAAC};
	while(buf_sizeAAC>0 && decoderErr==AAC_DEC_NOT_ENOUGH_BITS){
		
		int ret = aacDecoder_Fill(aac_ctx->handle, input_buf, (UINT*)&aac_pkt_size, &valid_size);  
	    if (ret != AAC_DEC_OK) {  
	        printf("Fill failed: %d\n", ret);  
	        *buf_sizePCM  = 0;  
	        return -1;  
	    }  

		decoderErr = aacDecoder_DecodeFrame(aac_ctx->handle, bufferPCM, pcm_pkt_size, 0);  
	    if (decoderErr == AAC_DEC_NOT_ENOUGH_BITS) {  
	        printf("DecodeFrame not enough\n");  
	        *buf_sizePCM  = 0;   
	    }
		else if (decoderErr != AAC_DEC_OK) {  
	        printf("aacDecoder_DecodeFrame error: 0x%x\n", ret);  
	        *buf_sizePCM  = 0;  
	        return -2;  
	    } 
		

		aac_stream_info = aacDecoder_GetStreamInfo(aac_ctx->handle);  
	    if (aac_stream_info == NULL) {  
	        printf("aacDecoder_GetStreamInfo failed!\n");  
	        return -3;
	    } 
#if 0
	    printf(">stream info:channel = %d sample_rate = %d samples = %d aot = %d bitRate = %d numTotalBytes = %d\n" \
	    		,aac_stream_info->channelConfig, aac_stream_info->aacSampleRate \ 
	            ,aac_stream_info->aacSamplesPerFrame, aac_stream_info->aot, aac_stream_info->bitRate \
	            ,aac_stream_info->numTotalBytes);
#endif
		//注意fdk_aac编码只接受16位,解码出来默认16位
		if(decoderErr == AAC_DEC_OK)
		{
			*buf_sizePCM = aac_stream_info->aacSamplesPerFrame * aac_stream_info->channelConfig * 2;
			break;
		}
	}

	return 0;
}


int AACDecoder::getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size){
	int size = 0;

	if(!buffer || !data || !data_size ){
		return -1;
	}

	while(1){
		if(buf_size  < 7 ){      //ADTS frame的首部ADTS header为7字节
			return -1;
		}
		/*
		Sync words    同步字,总是0xFFF,代表着一个ADTS帧的开始                       12bit
		ADST 首部：https://blog.csdn.net/tantion/article/details/82743942
		*/
		if((buffer[0] == 0xff) && ((buffer[1] & 0xf0) == 0xf0) ){   //(buffer[1] & 0xf0) == 0xf0)   保证前四位为0xf

			//aac_frame_length有13bit  size为aac_frame_length   1帧ADST的大小 
			size |= ((buffer[3] & 0x03) <<11);     //high 2 bit
			size |= buffer[4]<<3;                //middle 8 bit
			size |= ((buffer[5] & 0xe0)>>5);        //low 3bit
			break;
		}
		--buf_size;
		++buffer;
	}

	if(buf_size < size){
		return 1;
	}
	memcpy(data, buffer, size);
	*data_size = size;

	return 0;
}