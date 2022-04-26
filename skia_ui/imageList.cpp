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
#define LOG_TAG "ImageList"
#include "cutils/log.h"
#include "imageList.h"

#include <SkTypeface.h>
#include <SkTemplates.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>

extern "C" {
#include "../utility/string_ext.h"
}

#define printf //ALOGD

#define DBG 0

struct list_head * ImageList::getImageList() {
	return &mTagList;
}

int ImageList::addTag(struct ImageTag ** pTag) {

	struct list_head * pList;

	if (pTag == NULL) return -1;

	list_for_each(pList, &mTagList) {
		if(pList == NULL) break;
		struct ImageTag * tag = list_entry(pList, struct ImageTag, i_list);
		if(tag == NULL) break;
		if((!strcmp(tag->filepath, (*pTag)->filepath)) && 
			((*pTag)->x == tag->x) &&  ((*pTag)->y == tag->y)) {
			list_del(pList);
			if (tag != NULL) {
				free(tag);
			}

			break;
		}
	}

        list_add_tail(&(*pTag)->i_list, &mTagList);

        return 0;
}

int ImageList::addImage(const char* filepath , int x, int y) {

	struct ImageTag * pTag = (struct ImageTag *)malloc(sizeof(struct ImageTag));

	ALOGD("*****addImage: filepath: %s", filepath);
	ALOGD("addImage: x=%d, y=%d", x, y);
	strcpy(pTag->filepath, filepath);
	pTag->x = x;
	pTag->y = y;	

	addTag(&pTag);

	return 0;
}

void ImageList::clearImageTagList(void) {
	struct list_head *pList;
	struct list_head *pListTemp;

	list_for_each_safe(pList, pListTemp, &mTagList) {
		struct ImageTag * tag = list_entry(pList, struct ImageTag, i_list);
		list_del(pList);
		if (tag) {
			free(tag);
		}
	}
}


ImageList::ImageList() {
	INIT_LIST_HEAD(&mTagList);
}

ImageList::~ImageList() {
 	clearImageTagList();
}

       
