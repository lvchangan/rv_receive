#ifndef SURFACE_UI_H
#define SURFACE_UI_H
///////////////////////////

#include <gui/ISurfaceComposer.h>
#include <gui/SurfaceComposerClient.h>

#include <utils/Log.h>
#include <utils/String8.h>
#include <ui/DisplayInfo.h>

#include <sys/stat.h>
#include <setjmp.h>
#include "textList.h"
#include "imageList.h"
#include <SkCanvas.h>

using namespace android ;

#define ASSERT_TRUE(condition)  ;//if(!(condition)){ALOGE("value error!,return -1..");return -1;}
#define ASSERT_EQ ;
#define FONT_PATH	"/system/fonts/"

#define COMMON_LAYER 1000
#define DISP_WIDTH 1280
#define DISP_HEIGHT 720

class SkiaUI {
 	public:
		SkiaUI();
		~SkiaUI();

		void drawText(char * str, int x, int y);
		void show_text(char* name , char*subname, char* data );
		void show_image(const char* filePath,int x,int y);
		void show_image_fullscreen(const char* filePath);
		void show_image_fitscreen(const char* filePath);
		void UpdateUI();
		void hideUI();
		void showUI();
		void deletSurface();
		void initSurface();

	private:
		sp<SurfaceComposerClient> mComposerClient;
		sp<SurfaceControl> mSurfaceControl;
		//Surface::SurfaceInfo mSurfaceInfo;
		sp<Surface> mSurface;
		uint8_t *mSurfaceBuf;
		SkTypeface *mFont;
		bool isInited;
		pthread_mutex_t mUiLock;

		ImageList * mImageList;
		TextList * mTextList;
		char mLanguage[64];
		int updateText(struct list_head * pTagList, SkCanvas *canvas);
		int updateImage(struct list_head * pTagList, SkCanvas *canvas);
		int getFbScaleConfig(char *scale);
		int setFbScale(char* scale);
};

#endif  // SURFACE_UI_H
