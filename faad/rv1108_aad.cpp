
#include "rv1108_aad.h"


AAC2PCM::AAC2PCM()
{
	faadCtx = (FAADContext *)malloc(sizeof(FAADContext));
	if(faadCtx == NULL)
	{
		printf("faadCtx malloc error\n");
	}
	
	fp_pcm = fopen("/data/dec.pcm","wb+");
	if(fp_pcm == NULL)
	{
		printf("open fp_pcm error\n");
	}
}

AAC2PCM::~AAC2PCM()
{
	faad_decode_close();
	if(fp_pcm)
	{
		fclose(fp_pcm);
	}
}

void AAC2PCM::faad_decode_close()
{
    if(faadCtx->handle){
        NeAACDecClose(faadCtx->handle);
    }	
	if(faadCtx)
	{
    	free(faadCtx);
	}
}

int AAC2PCM::faad_decoder_create(int sample_rate, int channels, int bit_rate)
{
	NeAACDecHandle handle = NeAACDecOpen();
    if(!handle){
        printf("NeAACDecOpen failed\n");
       	return -1;
    }
	
    NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(handle);
    if(!conf){
        printf("NeAACDecGetCurrentConfiguration failed\n");
        return -2;
    }
	
    conf->defSampleRate = sample_rate;
    conf->outputFormat = FAAD_FMT_16BIT;
    conf->dontUpSampleImplicitSBR = 1;
    NeAACDecSetConfiguration(handle, conf);
    
    faadCtx->handle = handle;
    faadCtx->sample_rate = sample_rate;
    faadCtx->channels = channels;
    faadCtx->bit_rate = bit_rate;

	m_AACDecInit = false;
    return 0;
}


int AAC2PCM::getADTSframe(unsigned char* buffer, int buf_size, unsigned char* data ,int* data_size){
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


int AAC2PCM::faad_decode_frame(unsigned char* bufferAAC, int buf_sizeAAC,unsigned char* bufferPCM, int *buf_sizePCM)
{
    NeAACDecHandle handle = faadCtx->handle;
	
	if(!m_AACDecInit)
	{
		unsigned char channels = 0;
	    unsigned long sample_rate = 0;
	    
		long res = NeAACDecInit(handle, bufferAAC, buf_sizeAAC, &sample_rate, &channels);
	    if (res < 0) {
	        printf("NeAACDecInit failed\n");
	        return -1;
	    }
		printf("ret = %ld, channels = %u, samplerate = %lu\n", res, channels, sample_rate);
		faadCtx->sample_rate = sample_rate;
        faadCtx->channels = channels;
		m_AACDecInit = true;
	}

	NeAACDecFrameInfo frame_info;
    unsigned char *pcm_data = (unsigned char *)NeAACDecDecode(handle, &frame_info, bufferAAC, buf_sizeAAC);
	if (frame_info.error > 0)
	{
		printf("%s\n", NeAACDecGetErrorMessage(frame_info.error));
		return -2;
	}
	else if (pcm_data && frame_info.samples > 0)
	{
		printf("frame info: bytesconsumed %d, channels %d, header_type %d,object_type %d, samples %d, samplerate %d\n",
							   frame_info.bytesconsumed,
							   frame_info.channels, frame_info.header_type,
							   frame_info.object_type, frame_info.samples,
							   frame_info.samplerate);
		
		*buf_sizePCM = frame_info.samples * frame_info.channels;		
		memcpy(bufferPCM,pcm_data,*buf_sizePCM);	
	}
    return 0;
}



