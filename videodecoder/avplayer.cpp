#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <android/native_window.h>
#include <binder/IPCThreadState.h>
#include <binder/IServiceManager.h>
#include <binder/Parcel.h>

extern "C" {
//#include <lollipop_socket_ipc.h>
}
#include "socket/TcpClient.h"
#include "socket/Tcp.h"
#include "avplayer.h"



#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

static const char* kMimeTypeAvc = "video/avc";


AVPlayer::AVPlayer(TcpClient *TcpClient)
{
	mVideoFrameCount = 0;
	mBeginTime = 0;
	pTcpClient = TcpClient;
}

AVPlayer::~AVPlayer(){
	Dispose();
}


int AVPlayer::InitVideo()
{
    mWidth = SCREEN_WIDTH;
    mHeight = SCREEN_HEIGHT;
	
    mRendering = true;
	
    ProcessState::self()->startThreadPool();
    DataSource::RegisterDefaultSniffers();
	
    mFormat = new AMessage;
    mLooper = new ALooper;
    mLooper->start();
    
    mComposerClient = new SurfaceComposerClient;
    mComposerClient->initCheck();
    sp<IBinder> dtoken(SurfaceComposerClient::getBuiltInDisplay(
                ISurfaceComposer::eDisplayIdMain));
    DisplayInfo dinfo;
    status_t status = SurfaceComposerClient::getDisplayInfo(dtoken, &dinfo);
    printf("-w=%d,h=%d,xdpi=%f,ydpi=%f,fps=%f,ds=%f\n",
            dinfo.w, dinfo.h, dinfo.xdpi, dinfo.ydpi, dinfo.fps, dinfo.density);
    if (status) {
        printf("get displayinfo err");
        return -1;
    }
	
    mControl = mComposerClient->createSurface(String8("Lollipop_wifi UI Surface"),
            dinfo.w, dinfo.h, PIXEL_FORMAT_RGBA_8888, 0);
    SurfaceComposerClient::openGlobalTransaction();
    mControl->setLayer(1000);
    mControl->setPosition(0,0);
    mControl->setSize(dinfo.w,dinfo.h);
    mControl->show();
    SurfaceComposerClient::closeGlobalTransaction();
    mSurface = mControl->getSurface();

    if(mSurface == NULL )
    {
        printf("mSurface is null");
        return -1;
    }
    gbp = mSurface->getIGraphicBufferProducer();


    //lollipop_socket_client_send(SOCK_FILE_SOFTAP, IPC_UI_HIDE);

    sp<NativeWindowWrapper> window = new NativeWindowWrapper(mSurface);

    mCodec = MediaCodec::CreateByType(
                mLooper, kMimeTypeAvc, false);
    
    sp<AMessage> format = new AMessage;
    format->setString("mime", kMimeTypeAvc);
    format->setInt32("width", mWidth);
    format->setInt32("height", mHeight);

	
    mCodec->configure(
                format,
                window->getSurfaceTextureClient(),
                //NULL,
                NULL,
                0);
				
    mCodec->start();
	
	int err = mCodec->getInputBuffers(&mBuffers[0]);
    CHECK_EQ(err, (status_t)OK);
    
    err = mCodec->getOutputBuffers(&mBuffers[1]);
    CHECK_EQ(err, (status_t)OK);

	//MakeBackground();
	char outfilename[256];
	sprintf(outfilename,"/data/yuvscale_%d.yuv",ClientID);
	//printf("LCA outfilename = %s\n",outfilename);
    mFout = fopen(outfilename, "wb+");
    if (!mFout) {
        printf("failed to open output file.\n");
        return -2;
    }
	
	pthread_t tid;
    pthread_create(&tid, NULL, VideoRenderThread, this);	
	
    return 0;
}

void AVPlayer::MakeBackground()
{
	mControlBG = mComposerClient->createSurface(
		String8("A Surface"),
		mWidth,
		mHeight,
		PIXEL_FORMAT_RGB_565,
		0);
	
	CHECK(mControlBG != NULL);
    CHECK(mControlBG->isValid());
	
	SurfaceComposerClient::openGlobalTransaction();
	CHECK_EQ(mControlBG->setLayer(0x20000000 - 1), (status_t)OK);
    CHECK_EQ(mControlBG->show(), (status_t)OK);
    SurfaceComposerClient::closeGlobalTransaction();
	
	sp<Surface> service = mControlBG->getSurface();
	
	ANativeWindow_Buffer ob;
    service->lock(&ob, NULL);
    service->unlockAndPost();
}

void AVPlayer::CheckIfFormatChange()
{
	mCodec->getOutputFormat(&mFormat);
	if(pTcpClient->tcp->ClientPlayernum == 1 || pTcpClient->tcp->ClientPlayernum == 2)
	{
	 	TVCanvaswidth = SCREEN_WIDTH / pTcpClient->tcp->ClientPlayernum;
		TVCanvasheight = SCREEN_HEIGHT;
	}
	else if(pTcpClient->tcp->ClientPlayernum == 3 || pTcpClient->tcp->ClientPlayernum == 4)
	{
		TVCanvaswidth = SCREEN_WIDTH / 2;
		TVCanvasheight = SCREEN_HEIGHT / 2;
	}

	int playerid = 0;
	for (auto it = pTcpClient->tcp->clientpalyerList.begin(); it != pTcpClient->tcp->clientpalyerList.end(); it++) {
		if(*it == ClientID)
		{
			break;
	 	}
		playerid++;
    }
	
	int width, height;
	if (mFormat->findInt32("width", &width) &&
		mFormat->findInt32("height", &height)) {
		float scale_x = (TVCanvaswidth + 0.0) / width;
		float scale_y = (TVCanvasheight + 0.0) / height;
		float scale = (scale_x < scale_y) ? scale_x : scale_y;
		
		scale = (scale > 1) ? 1 : scale;
		
		if (scale < 1) {
			int new_width = width * scale;
			int new_height = height * scale;
			
			new_width = (new_width > TVCanvaswidth) ? TVCanvaswidth : new_width;
			new_height = (new_height > TVCanvasheight) ? TVCanvasheight : new_height;
			
			width = new_width;
			height = new_height;
		}
		
		if (width > TVCanvaswidth)
			width = TVCanvaswidth;
		
		if (height > TVCanvasheight)
			height = TVCanvasheight;

		int x = (TVCanvaswidth - width) / 2 + TVCanvaswidth * (playerid < 2 ? playerid : (playerid - 2));
		int y = (TVCanvasheight - height) / 2 + TVCanvasheight * (playerid < 2 ? 0 : 1);
			
		if (width != mWidth || height != mHeight || old_x != x || old_y != y) {
			mWidth = width;
			mHeight = height;
			old_x = x;
			old_y = y;
			SurfaceComposerClient::openGlobalTransaction();
			mControl->setSize(width, height);
			mControl->setPosition(x, y);
			SurfaceComposerClient::closeGlobalTransaction();
		}
	}	
}

void AVPlayer::GetDisplayerResolvingPower(int Mirrorwidth,int Mirrorheight)
{
	if(pTcpClient->tcp->ClientPlayernum == 1 || pTcpClient->tcp->ClientPlayernum == 2)
	{
	 	TVCanvaswidth = SCREEN_WIDTH / pTcpClient->tcp->ClientPlayernum;
		TVCanvasheight = SCREEN_HEIGHT;
	}
	else if(pTcpClient->tcp->ClientPlayernum == 3 || pTcpClient->tcp->ClientPlayernum == 4)
	{
		TVCanvaswidth = SCREEN_WIDTH / 2;
		TVCanvasheight = SCREEN_HEIGHT / 2;
	}
	
	float scaleMirror = (float)Mirrorwidth / (float)Mirrorheight;		//投屏的原图比例
	float scaleTVCanvas = (float)TVCanvaswidth / (float)TVCanvasheight;	//TV端画布的比例
		
	if (scaleTVCanvas < scaleMirror) {
			//TV端画布高度比例更高一些，应该以width做基准缩放
			displayerwidth = TVCanvaswidth;
			displayerheight = (int)(displayerwidth / scaleMirror);
	}
	else 
	{
		displayerheight = TVCanvasheight;
		displayerwidth = displayerheight * scaleMirror;
	}
	displayerwidth = (displayerwidth + 1) & ~0x01;
	displayerheight = (displayerheight + 1) & ~0x01;
	
	int x = (TVCanvaswidth - displayerwidth)/2 + TVCanvaswidth * ClientID;
	int y = (TVCanvasheight - displayerheight)/2;

	printf("ClentID : %d wxh[%dx%d] x.y[%d.%d]\n",ClientID,displayerwidth,displayerheight,x,y);
	SurfaceComposerClient::openGlobalTransaction();
	mControl->setSize(displayerwidth, displayerheight);
	mControl->setPosition(x, y);
	SurfaceComposerClient::closeGlobalTransaction();
}


void AVPlayer::RenderFrames()
{
	size_t index, offset, size;
	int64_t pts;
	uint32_t flags;
	int width,height;
	int err = OK;
	
	do {
		
		CheckIfFormatChange();
		
		err = mCodec->dequeueOutputBuffer(
			&index, &offset, &size, &pts, &flags);

		if (err == OK) {	
			/*sp<ABuffer> buffer;  
   			mCodec->getOutputBuffer(index, &buffer);		
			fwrite(buffer->data(),1,size,mFout);
			mCodec->releaseOutputBuffer(index);*/
			/*
			if(old_num != pTcpClient->tcp->ClientPlayernum||old_w != width||old_h !=height)
			{
				mCodec->getOutputFormat(&mFormat);
				if (mFormat->findInt32("width", &width) && mFormat->findInt32("height", &height)) 
				{
					GetDisplayerResolvingPower(width,height);
					old_num = pTcpClient->tcp->ClientPlayernum;
					old_w = width;
					old_h = height;
				}
			}
			*/
			
			mCodec->renderOutputBufferAndRelease(index);
			
			/*
			mVideoFrameCount++;
			if (mBeginTime == 0) {
				mBeginTime = clock();
			} else {
				float fps = mVideoFrameCount / (float(clock() - mBeginTime) / CLOCKS_PER_SEC);
				//printf("### %f\n", fps);
			}
			*/
			
			/*
			fps_out++;
			if(fps_out == fps_time && time_flag ==1)
			{
				printf("###deconeframe time = %ld,fps_time %d\n",get_sys_runtime(2) - time,fps_time);
				time_flag = 0;
			}
			*/
		}
	} while(err == OK
                || err == INFO_FORMAT_CHANGED
                || err == INFO_OUTPUT_BUFFERS_CHANGED);
}

void* AVPlayer::VideoRenderThread(void* arg)
{
	AVPlayer* player = (AVPlayer*)arg;

    while(player->mRendering) {
        player->RenderFrames();
    }
    
    return NULL;	
}

int AVPlayer::FeedOneH264Frame(unsigned char* frame, int size)
{
	size_t index;

	int err = mCodec->dequeueInputBuffer(&index, -1ll);
	
	CHECK_EQ(err, (status_t)OK);
	
	const sp<ABuffer> &dst = mBuffers[0].itemAt(index);

	CHECK_LE(size, dst->capacity());
	
	dst->setRange(0, size);
	memcpy(dst->data(), frame, size);
	/*
	fps_in++;
	if(time_flag == 0)
	{
		time = get_sys_runtime(2);
		fps_time = fps_in;
		time_flag = 1;
	}
	*/
	err = mCodec->queueInputBuffer(
					index,
					0,
					size,
					0ll,
					0);
	return err;
}

void AVPlayer::hideUI(void) {
    SurfaceComposerClient::openGlobalTransaction();
    CHECK_EQ(mControl->hide(), (status_t)OK);
    SurfaceComposerClient::closeGlobalTransaction();
}

void AVPlayer::showUI(void) {
	SurfaceComposerClient::openGlobalTransaction();
     CHECK_EQ(mControl->show(), (status_t)OK);
    SurfaceComposerClient::closeGlobalTransaction();
}


void AVPlayer::Dispose()
{
	mAudioThreadRunning = false;
	mCodec->stop();
	mCodec->reset();
	mCodec->release();
	mLooper->stop();
	
	
	mRendering = false;
	//SurfaceComposerClient::openGlobalTransaction();
    //CHECK_EQ(mControl->hide(), (status_t)OK);
	//CHECK_EQ(mControlBG->hide(), (status_t)OK);
    //SurfaceComposerClient::closeGlobalTransaction();	
	
	mComposerClient->dispose();
	mControl->clear();
	//mControlBG->clear();

    //lollipop_socket_client_send(SOCK_FILE_SOFTAP, IPC_UI_SHOW);
	fclose(mFout);
	if (mAudioTrack != NULL) {
		mAudioTrack->stop();
	}
	mAudioThreadRunning = false;
}

/************************************************************************
 ** 函数名:     get_sys_runtime
 ** 函数描述:   返回系统运行时间
 ** 参数:       [in]  1 - 秒,2 - 毫秒
 ** 返回:       秒/毫秒
 ************************************************************************/
long AVPlayer::get_sys_runtime(int type)
{
	struct timespec times = {0, 0};
	long time;

	clock_gettime(CLOCK_MONOTONIC, &times);

	if (1 == type){
		time = times.tv_sec;
	}else{
		time = times.tv_sec * 1000 + times.tv_nsec / 1000000;
	}
	
    return time;
}

int AVPlayer::InitAudio(int sample_rate, int channel)
{
	size_t frameCount = 0;
	
	AudioTrack::getMinFrameCount(&frameCount, AUDIO_STREAM_MUSIC, 48000);
	
    if (mAudioTrack) {
        mAudioTrack = NULL;
    }

    mAudioTrack = new AudioTrack(AUDIO_STREAM_MUSIC, 48000, 
                      AUDIO_FORMAT_PCM_16_BIT, audio_channel_out_mask_from_count(2), 
                      frameCount, AUDIO_OUTPUT_FLAG_NONE);

    if (mAudioTrack->initCheck() != OK) {
        mAudioTrack = NULL;
        return -1;
    }
	
	AudioQueueInit();
	
	pthread_t tid;
    pthread_create(&tid, NULL, AudioThread, this);
	return 0;
}

int AVPlayer::FeedOnePcmFrame(unsigned char* frame, int len,bool mute)
{
	if (mute) {
      for (int i = 0; i < len; i++) 
	  {
         frame[i] = 0xff;
      }
   }
	AudioQueueBuffer(frame, len);
	return 0;
}

bool AVPlayer::AudioQueueBuffer(unsigned char* buffer, int len)
{
    if (mFreeQueue.empty())
        return false;

    struct audio_frame frame = mFreeQueue.front();
    mFreeQueue.pop();

    memcpy(frame.data, buffer, len);
    frame.len = len;

    mDataQueue.push(frame);
    return true;
}

void AVPlayer::AudioQueueInit()
{
    for (int i = 0; i < FRAME_COUNT; i++) {
        struct audio_frame frame;
        frame.data = &(mAudioBuffer[i * FRAME_SIZE]);
        frame.len = FRAME_SIZE;
        mFreeQueue.push(frame);    
    }
}

void AVPlayer::ProcessAudioData()
{
	mAudioThreadRunning = true;
	
    do {
        if (mDataQueue.empty()) {
            usleep(1 * 1000);
            continue;        
        }

        struct audio_frame frame = mDataQueue.front();
        mDataQueue.pop();


		if (mAudioTrack != NULL) {
			if (mAudioTrack->stopped()) {
				mAudioTrack->start();
			}
			
			mAudioTrack->write(frame.data, frame.len);
		}
		
        mFreeQueue.push(frame);
    } while(mAudioThreadRunning);    
}

void* AVPlayer::AudioThread(void* arg)
{
    AVPlayer* avplayer = (AVPlayer*)arg;
    avplayer->ProcessAudioData();
    return NULL;
}

