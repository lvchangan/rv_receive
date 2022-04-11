#ifndef _AUDIO_DECODER_H_
#define _AUDIO_DECODER_H_

#include <stdio.h>
#include <stdlib.h>
#include "aacdecoder_lib.h"


class AACDecoder{

private:
	typedef struct AAC_Context_s{
	    HANDLE_AACDECODER handle;	//解码器句柄
		int  channels;	  //声道数
	    int  sample_rate; //采样率
	    int bit_rate;	  //比特率
	}AAC_Context;

	TRANSPORT_TYPE transportFmt; //原始数据格式
	
	AAC_Context *aac_ctx;
	
	CStreamInfo *aac_stream_info;
public:
	AACDecoder();
	~AACDecoder();
	int audioDecoderInit();
	int decoder_one_aac(unsigned char* bufferAAC, int buf_sizeAAC,INT_PCM *bufferPCM, int *buf_sizePCM);
	int getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size);

	FILE *fp_pcm;
};

#endif