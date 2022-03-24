#ifndef _RV1108_AAD_H_
#define _RV1108_AAD_H_

#include <stdio.h>
#include <stdlib.h>
#include <cstring>
#include <string.h>
#include "faad.h"

class AAC2PCM
{

public:
	AAC2PCM();
	~AAC2PCM();

	void faad_decode_close();
	int getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size);
	int faad_decoder_create(int sample_rate, int channels, int bit_rate);
	int faad_decode_frame(unsigned char* bufferAAC, int buf_sizeAAC,unsigned char* bufferPCM, int *buf_sizePCM);
private:
	typedef struct FAADContext_s{
	    NeAACDecHandle handle;	//解码器句柄
		int  channels;	  //声道数
	    int  sample_rate; //采样率
	    int bit_rate;	  //比特率
	}FAADContext;

	FAADContext *faadCtx;

	bool m_AACDecInit;
public:
	FILE *fp_pcm;
};

#endif


