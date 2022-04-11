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

#include "avplayer.h"

#define SCREEN_WIDTH 1280
#define SCREEN_HEIGHT 720

static const char* kMimeTypeAvc = "video/avc";

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
    mControl->setLayer(0x20000000);
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
                //window->getSurfaceTextureClient(),
                NULL,
                NULL,
                0);
				
    mCodec->start();
	
	int err = mCodec->getInputBuffers(&mBuffers[0]);
    CHECK_EQ(err, (status_t)OK);
    
    err = mCodec->getOutputBuffers(&mBuffers[1]);
    CHECK_EQ(err, (status_t)OK);


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
	CHECK_EQ(mControlBG->setLayer(INT_MAX - 1), (status_t)OK);
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
		
	int width, height;
	if (mFormat->findInt32("width", &width) &&
		mFormat->findInt32("height", &height)) {
		float scale_x = (SCREEN_WIDTH + 0.0) / width;
		float scale_y = (SCREEN_HEIGHT + 0.0) / height;
		float scale = (scale_x < scale_y) ? scale_x : scale_y;
		
		scale = (scale > 1) ? 1 : scale;
		
		if (scale < 1) {
			int new_width = width * scale;
			int new_height = height * scale;
			
			new_width = (new_width > SCREEN_WIDTH) ? SCREEN_WIDTH : new_width;
			new_height = (new_height > SCREEN_HEIGHT) ? SCREEN_HEIGHT : new_height;
			
			width = new_width;
			height = new_height;
		}
		
		if (width > SCREEN_WIDTH)
			width = SCREEN_WIDTH;
		
		if (height > SCREEN_HEIGHT)
			height = SCREEN_HEIGHT;
		
		if (width != mWidth || height != mHeight) {
			mWidth = width;
			mHeight = height;
			
			int x = (SCREEN_WIDTH - width) / 2;
			int y = (SCREEN_HEIGHT - height) / 2;
			
			SurfaceComposerClient::openGlobalTransaction();
			mControl->setSize(width, height);
			mControl->setPosition(x, y);
			SurfaceComposerClient::closeGlobalTransaction();
		}
	}	
}

void AVPlayer::RenderFrames()
{
	size_t index, offset, size;
	int64_t pts;
	uint32_t flags;
	
	int err = OK;
	
	do {
		CheckIfFormatChange();
		
		err = mCodec->dequeueOutputBuffer(
			&index, &offset, &size, &pts, &flags);

		if (err == OK) {
			//mCodec->renderOutputBufferAndRelease(index);
			sp<ABuffer> buffer;  
   			mCodec->getOutputBuffer(index, &buffer);		
			fwrite(buffer->data(),1,size,mFout);
			mCodec->releaseOutputBuffer(index);
		
			mVideoFrameCount++;
			if (mBeginTime == 0) {
				mBeginTime = clock();
			} else {
				float fps = mVideoFrameCount / (float(clock() - mBeginTime) / CLOCKS_PER_SEC);
				printf("### %f\n", fps);
			}
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
	err = mCodec->queueInputBuffer(
					index,
					0,
					size,
					0ll,
					0);
	return err;
}
	
void AVPlayer::Dispose()
{
	mCodec->stop();
	mCodec->reset();
	mCodec->release();
	mLooper->stop();
	
	printf("LCA end avplayer\n");
	mRendering = false;
	
	SurfaceComposerClient::openGlobalTransaction();
    CHECK_EQ(mControl->hide(), (status_t)OK);
	//CHECK_EQ(mControlBG->hide(), (status_t)OK);
    SurfaceComposerClient::closeGlobalTransaction();	
	mComposerClient->dispose();
	mControl->clear();
	//mControlBG->clear();

    //lollipop_socket_client_send(SOCK_FILE_SOFTAP, IPC_UI_SHOW);
	fclose(mFout);
}

