/*
 * Copyright (C) 2011 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <pthread.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "skia_ui.h"
#define LOG_TAG "skia_ui"
#include "cutils/log.h"
#include "textList.h"

#include <SkTypeface.h>
#include <SkTemplates.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>
#include <SkStream.h>
#include <SkImageDecoder.h>
#include <SkImageEncoder.h>
#include <SkMatrix.h>

#include <gui/Surface.h>
#include <gui/SurfaceComposerClient.h>
#include <android/native_window.h>
#include <SkXfermode.h>
#include <SkColor.h>
#include <SkPaint.h>
#define printf //ALOGD

#define DBG 0

extern "C" {
	extern int getLollipopConf(char * param, char * value);
	extern void show_text(char* name, char* subname, char* data);
	extern void show_image(const char* filePath,int x,int y);
	extern void show_image_fullscreen(const char* filePath);
	extern void show_image_fitscreen(const char* filePath);
	extern void lollipop_ui_init(void);
	extern void lollipop_ui_deinit(void);
	extern void lollipop_ui_delete(void);
	extern void lollipop_ui_reinit(void);
	extern void lollipop_ui_update();
	extern void lollipop_ui_hide();
	extern void lollipop_ui_show();
}

bool imageDecode( SkBitmap* bitmap, const char srcPath[]);

using namespace android ;

///////////////////////////////////////////////////////////////////////
//for C code invoke
static SkiaUI* ui = NULL;

void show_text(char* name, char * subname, char* data) {
	ui->show_text(name, subname, data);
}

void show_image(const char* filePath,int x,int y) {
	ui->show_image(filePath, x, y);
}

void show_image_fullscreen(const char* filePath) {
	//ui->show_image_fullscreen(filePath);
}

void show_image_fitscreen(const char* filePath) {
	//ui->show_image_fitscreen(filePath);
}

void lollipop_ui_init(void) {
	ALOGD("lollipop_ui_init");
	ui = new SkiaUI();
}

void lollipop_ui_update() {
	ALOGD("lollipop_ui_update");
	ui->UpdateUI();
}

void lollipop_ui_deinit(void) {
	ALOGD("lollipop_ui_deinit");
	delete ui;
}

void lollipop_ui_delete(void) {
	ALOGD("lollipop_ui_delete");
        ui->deletSurface();
}

void lollipop_ui_reinit(void) {
	ALOGD("lollipop_ui_reinit");
        ui->initSurface();
}

void lollipop_ui_hide(void) {
        ALOGD("lollipop_ui_hide");
	ui->hideUI();
}

void lollipop_ui_show(void) {
        ALOGD("lollipop_ui_show");
	ui->showUI();
}

static const char * OrientTab[9] = {
	"Undefined",
	"Normal",           // 1
	"flip horizontal",  // left right reversed mirror
	"rotate 180",       // 3
	"flip vertical",    // upside down mirror
	"transpose",        // Flipped about top-left <--> bottom-right axis.
	"rotate 90",        // rotate 90 cw to right it.
	"transverse",       // flipped about top-right <--> bottom-left axis
	"rotate 270",       // rotate 270 to right it.
};

SkiaUI::SkiaUI() {
	//get overscan config
	char fb_scale[4] = {0};
	getFbScaleConfig(fb_scale);
	ALOGD("overscan is %s", fb_scale);
	setFbScale(fb_scale);

//////////////////////////////////////////////////////
	isInited = false;
	mUiLock = PTHREAD_MUTEX_INITIALIZER;
	//init surface
	initSurface();
//////////////////////////////////////////////////////

	//get language
	char xmlPath[256] = {0};
	char fontPath[256] = {0};
	memset(mLanguage,0,sizeof(mLanguage));	
	getLollipopConf("language",mLanguage);
	sprintf(xmlPath, "%s%s.xml", FONT_PATH, mLanguage);
	//sprintf(fontPath, "%s%s.ttf", FONT_PATH, mLanguage);
#ifdef SUPPORT_CHINESE_LANGUAGE
        sprintf(fontPath, "%sChinese.ttf", FONT_PATH);
#else
        sprintf(fontPath, "%sEnglish.ttf", FONT_PATH);
#endif

	mTextList = new TextList(xmlPath, fontPath);
	mFont = SkTypeface::CreateFromFile(fontPath);

	mImageList = new ImageList();

	usleep(5000);
}

SkiaUI::~SkiaUI() {
	delete mTextList;
	delete mImageList;
}

void SkiaUI::deletSurface(void) {
	if (!isInited) {
		return;
	}
	pthread_mutex_lock(&mUiLock);
	isInited = false;
        mSurface.clear();
        mComposerClient.clear();
        mSurfaceControl.clear();
	mSurface = NULL;
	mComposerClient = NULL;
        mSurfaceControl = NULL;
	pthread_mutex_unlock(&mUiLock);
}

void SkiaUI::initSurface(void) {
	if (isInited) {
		return;
	}
	pthread_mutex_lock(&mUiLock);
	isInited = true;
        mComposerClient = new SurfaceComposerClient;
        ASSERT_EQ(NO_ERROR, mComposerClient->initCheck());

        mSurfaceControl = mComposerClient->createSurface(
                String8("Lollipop_wifi UI Surface"), DISP_WIDTH, DISP_HEIGHT,
                PIXEL_FORMAT_RGBA_8888, 0);
        ASSERT_TRUE(mSurfaceControl->isValid());

        SurfaceComposerClient::openGlobalTransaction();
        ASSERT_EQ(NO_ERROR, mSurfaceControl->setLayer(COMMON_LAYER));
        ASSERT_EQ(NO_ERROR, mSurfaceControl->setPosition(0, 0));
        ASSERT_EQ(NO_ERROR, mSurfaceControl->show());
        SurfaceComposerClient::closeGlobalTransaction(true);
	pthread_mutex_unlock(&mUiLock);
}

void SkiaUI::hideUI(void) {
	if (!isInited) {
		return;
	}
	pthread_mutex_lock(&mUiLock);
        SurfaceComposerClient::openGlobalTransaction();
        ASSERT_EQ(NO_ERROR, mSurfaceControl->hide());
        SurfaceComposerClient::closeGlobalTransaction();
	pthread_mutex_unlock(&mUiLock);
}

void SkiaUI::showUI(void) {
	if (!isInited) {
		return;
	}
	pthread_mutex_lock(&mUiLock);
        SurfaceComposerClient::openGlobalTransaction();
        ASSERT_EQ(NO_ERROR, mSurfaceControl->show());
        SurfaceComposerClient::closeGlobalTransaction();
	pthread_mutex_unlock(&mUiLock);
}

int SkiaUI::updateImage(struct list_head * pTagList, SkCanvas *canvas) {
	if (!isInited) {
		return 0;
	}
	struct list_head * pList;

	list_for_each(pList, pTagList) {
		if(pList == NULL) break;
		struct ImageTag * tag = list_entry(pList, struct ImageTag, i_list);
		if(tag == NULL) break;

		//ALOGD("updateImage: %s, x=%d, y=%d", tag->filepath, tag->x, tag->y);

		SkBitmap bitmap;
		imageDecode(&bitmap, tag->filepath);

		SkRect dst{tag->x, tag->y, bitmap.width()+tag->x, bitmap.height()+tag->y};
		SkIRect src{0,0,bitmap.width(), bitmap.height()};
		canvas->drawBitmapRect(bitmap, &src, dst);
	}

	return 0;
}

#ifdef USE_MINIKIN	
#include <unicode/unistr.h>
#include <unicode/utf16.h>

//#include <minikin/MinikinFontFreeType.h>
#include <minikin/Layout.h>

#include <SkCanvas.h>
#include <SkGraphics.h>
#include <SkImageEncoder.h>
#include <SkTypeface.h>
#include <SkPaint.h>

//#include "MinikinSkia.h"
#include "MinikinSkia.cpp"

using std::vector;

FT_Library library;  // TODO: this should not be a global

FontCollection *makeFontCollection() {
    vector<FontFamily *>typefaces;

#if 0
    FontFamily *family = new FontFamily();
    FT_Face face;
    FT_Error error;
    for (size_t i = 0; i < sizeof(fns)/sizeof(fns[0]); i++) {
        const char *fn = fns[i];
        SkTypeface *skFace = SkTypeface::CreateFromFile(fn);
        MinikinFont *font = new MinikinFontSkia(skFace);
        family->addFont(font);
    }
    typefaces.push_back(family);
#else
    FontFamily *family = new FontFamily();
    const char *fn = "/system/fonts/NotoSans-Regular.ttf";
	//const char *fn = "/system/fonts/English.ttf";
    SkTypeface *skFace = SkTypeface::CreateFromFile(fn);
    MinikinFont *font = new MinikinFontSkia(skFace);
    family->addFont(font);
    typefaces.push_back(family);
#endif

    return new FontCollection(typefaces);
}

// Maybe move to MinikinSkia (esp. instead of opening GetSkTypeface publicly)?

void drawToSkia(SkCanvas *canvas, SkPaint *paint, Layout *layout, float x, float y) {
    size_t nGlyphs = layout->nGlyphs();
    uint16_t *glyphs = new uint16_t[nGlyphs];
    SkPoint *pos = new SkPoint[nGlyphs];
    SkTypeface *lastFace = NULL;
    SkTypeface *skFace = NULL;
    size_t start = 0;

    paint->setTextEncoding(SkPaint::kGlyphID_TextEncoding);
    for (size_t i = 0; i < nGlyphs; i++) {
        MinikinFontSkia *mfs = static_cast<MinikinFontSkia *>(layout->getFont(i));
        skFace = mfs->GetSkTypeface();
        glyphs[i] = layout->getGlyphId(i);
        pos[i].fX = x + layout->getX(i);
        pos[i].fY = y + layout->getY(i);
        if (i > 0 && skFace != lastFace) {
            paint->setTypeface(lastFace);
            canvas->drawPosText(glyphs + start, (i - start) << 1, pos + start, *paint);
            start = i;
        }
        lastFace = skFace;
    }
    paint->setTypeface(skFace);
    canvas->drawPosText(glyphs + start, (nGlyphs - start) << 1, pos + start, *paint);
    delete[] glyphs;
    delete[] pos;
}
#endif
int SkiaUI::updateText(struct list_head * pTagList, SkCanvas *canvas) {
	if (!isInited) {
		return 0;
	}
	struct list_head * pList;
	SkPaint skPaint_txt;

	list_for_each(pList, pTagList) {
		if(pList == NULL) break;
		struct TextTag * tag = list_entry(pList, struct TextTag, i_list);
		if(tag == NULL) break;
		
		if(DBG) ALOGD("updateText---%s---%d", tag->str, strlen(tag->str));
		//skPaint_txt.setStrokeWidth(4);
		skPaint_txt.setTypeface(mFont);

		skPaint_txt.setARGB(255,
			tag->color.red,
			tag->color.green,
			tag->color.blue);

		skPaint_txt.setTextSize(tag->text_size);
		skPaint_txt.setAntiAlias(true);
#ifndef USE_MINIKIN		
		canvas->drawText(tag->str,
				strlen(tag->str), 
				tag->tag_x, 
				tag->tag_y + tag->text_size, 
				skPaint_txt);
#else
#if 1
    //Layout::init();
	int x = tag->tag_x, y = tag->tag_y + tag->text_size;
    FontCollection *collection = makeFontCollection();
    Layout layout;
    layout.setFontCollection(collection);
    //const char *text = "fine world \xe0\xa4\xa8\xe0\xa4\xae\xe0\xa4\xb8\xe0\xa5\x8d\xe0\xa4\xa4\xe0\xa5\x87";
    //const char *style = "font-size: 32; font-weight: 700;";
    int bidiFlags = 0;
    FontStyle fontStyle(7);
    MinikinPaint minikinPaint;
    minikinPaint.size = tag->text_size;
    icu::UnicodeString icuText = icu::UnicodeString::fromUTF8(tag->str);
    layout.doLayout(icuText.getBuffer(), 0, icuText.length(), icuText.length(), bidiFlags, fontStyle, minikinPaint);
    layout.dump();

    //skPaint_txt.setARGB(255, 0, 0, 128);
    //skPaint_txt.setStyle(SkPaint::kStroke_Style);
    //skPaint_txt.setStrokeWidth(2);
    //skPaint_txt.setTextSize(100);
    //skPaint_txt.setAntiAlias(true);
    canvas->drawLine(x, y, x /*+ layout.getAdvance()*/, y, skPaint_txt);
    //skPaint_txt.setStyle(SkPaint::kFill_Style);
    drawToSkia(canvas, &skPaint_txt, &layout, x, y);
#else
			float x = tag->tag_x;
			float y = tag->tag_y + tag->text_size;

		    FontCollection *collection = makeFontCollection();
		    Layout layout;
		    layout.setFontCollection(collection);
		    int bidiFlags = 0;
		    FontStyle fontStyle(4);
		    MinikinPaint minikinPaint;
		    minikinPaint.size = tag->text_size;
		    minikinPaint.scaleX = 1.0f; //wjx this is important
		    icu::UnicodeString icuText = icu::UnicodeString::fromUTF8(tag->str);
		    layout.doLayout(icuText.getBuffer(), 0, icuText.length(), icuText.length(),
				bidiFlags, fontStyle, minikinPaint);
		    layout.dump();

		    if (tag->align == SkPaint::kCenter_Align) {
				x = x - SkScalarHalf(layout.getAdvance());
				if (x < 0.0f) x = 0.0f;
		    }
		    drawToSkia(canvas, &skPaint_txt, &layout, x, y);
#endif
#endif
	}

	return 0;
}

void SkiaUI::UpdateUI() {
	if (!isInited) {
		return;
	}
	SkBitmap bitmap;
        ANativeWindow_Buffer outBuffer;
        ARect inOutDirtyBounds;

	pthread_mutex_lock(&mUiLock);
	mSurface = mSurfaceControl->getSurface();
	ASSERT_TRUE(mSurface != NULL);

        mSurface->lock(&outBuffer, &inOutDirtyBounds);
        
        ssize_t bpr = outBuffer.stride * bytesPerPixel(outBuffer.format);
        bitmap.installPixels(SkImageInfo::MakeN32Premul(outBuffer.width, outBuffer.height),
                                            outBuffer.bits, bpr);

        SkCanvas canvas(bitmap);

	struct list_head * list_img = mImageList->getImageList();
	updateImage(list_img, &canvas);

	//skbitmap.eraseColor(0);
	struct list_head * list_txt = mTextList->getTextList();
	updateText(list_txt, &canvas);

	mSurface->unlockAndPost();
	pthread_mutex_unlock(&mUiLock);
}

bool imageDecode( SkBitmap* bitmap, const char srcPath[]) {
	SkFILEStream stream(srcPath);
	if (!stream.isValid()) {
		return false;
	}

        SkAutoTDelete<SkImageDecoder> codec(SkImageDecoder::Factory(&stream));
        if (NULL == codec.get()) {
            return false;
        }

	if (!codec->decode(&stream, bitmap, kN32_SkColorType,
		SkImageDecoder::kDecodePixels_Mode)) {
		return false;
	}

	return true;
}

bool imageDecode( SkBitmap* bitmap, char *data, int length) {
	SkMemoryStream stream(data, length, false);

        SkAutoTDelete<SkImageDecoder> codec(SkImageDecoder::Factory(&stream));
        if (NULL == codec.get()) {
            return false;
        }

	if (!codec->decode(&stream, bitmap, kN32_SkColorType,
		SkImageDecoder::kDecodePixels_Mode)) {
		return false;
	}

	return true;
}

void SkiaUI::show_image(const char* filename, int x, int y) {
	mImageList->addImage(filename, x, y);
}

void SkiaUI::show_text(char* name , char* subname, char* data) {
	mTextList->addText(name, subname, data);
}

int SkiaUI::getFbScaleConfig(char *scale){
	return getLollipopConf("fb_scale", scale);
}

int SkiaUI::setFbScale(char* scale){
	FILE *f;
	f = fopen("/sys/class/graphics/fb0/scale", "w+");
	if (f) {
		fwrite(scale, 1, strlen(scale), f);
		fclose(f);
		ALOGD("### set fb_scale to %s ..",scale);
	}else {
		return -1;
	}

	return 0;
}

