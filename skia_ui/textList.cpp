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
#define LOG_TAG "TextList"
#include "cutils/log.h"
#include "textList.h"

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

struct list_head * TextList::getTextList() {
	return &mTagList;
}

int TextList::addTag(struct TextTag ** pTag) {

	struct list_head * pList;

	if (pTag == NULL) return -1;

	//if(DBG) ALOGD("addTag, pTag->textBuf=0x%x", (*pTag)->textBuf);
	if(DBG) ALOGD("addTag, pTag->width=%d", (*pTag)->width);
	if(DBG) ALOGD("addTag, pTag->height=%d", (*pTag)->height);

	list_for_each(pList, &mTagList) {
		if(pList == NULL) break;
		struct TextTag * tag = list_entry(pList, struct TextTag, i_list);
		if(tag == NULL) break;
		if(!strcmp(tag->name, (*pTag)->name)) {
			
			list_del(pList);

			if (tag->str != NULL) {
				free(tag->str);
				tag->str = NULL;
			}
/*
			if (tag->textBuf != NULL) {
				free(tag->textBuf);
				tag->textBuf = NULL;
			}
*/
			if (tag != NULL) {
				free(tag);
			}

			break;
		}
	}

        list_add_tail(&(*pTag)->i_list, &mTagList);

        return 0;
}

int TextList::getTagByName(const char * name, struct TextTag ** pTag) {

	struct list_head * pList;

	if (pTag == NULL) return -1;
	if (*pTag == NULL) return -1;

	list_for_each(pList, &mTagList) {
		if(pList == NULL) break;
		struct TextTag * tag = list_entry(pList, struct TextTag, i_list);
		if(tag == NULL) break;

		if(!strcmp(tag->name, name)) {
			if(DBG) ALOGD("found (%s)", name);
			*pTag = tag;
			return 0;
		}
	}

	return -1;
}

int TextList::getStr(const char* name , const char* subname, const char* data, struct TextTag *pTag) {
	if(DBG) ALOGD("getStr");
	if(DBG) ALOGD("name:%s", name);
	if(DBG) {
		if (subname != NULL) ALOGD("subname:%s", subname);
	}

	if(DBG) {
		if (data != NULL) ALOGD("data:%s", data);
	}

	char result[128];
	char value[256];
	char* tempname = NULL;
	char* tempsubname = NULL;
	char* color = NULL;
	char* location = NULL;
	char* size = NULL;
	char *temp_size = NULL;
	char *temp_color = NULL;
	char *temp_location = NULL ;
	char *temp_name = NULL;
	char *temp_subname = NULL;
	char *last;

	pTag->height = 0;
	pTag->width = 0;	
	
	if (pTag == NULL) {
		ALOGE("pTag is NULL");
		return -1;
	}

	if(mStringmap->getText(name, subname, result) == true ) {
		mStringmap->getValue(name, subname, value);
		tempname = strtok_r(result," ", &last);
		char * p = NULL;
		while(1) {
			p = strtok_r(NULL, " ", &last);
			if (p == NULL) break;
			if(DBG) ALOGD("p=%s", p);
			if(str_startsWith(p, (char *)"subname=")) tempsubname = p;
			else if(str_startsWith(p, (char *)"size=")) size = p;
			else if(str_startsWith(p, (char *)"color=")) color = p;
			else if(str_startsWith(p, (char *)"location=")) location = p;
		}
		/***************************************/
		if (size != NULL) {
			temp_size = strtok_r(size,"=", &last);
			temp_size = strtok_r(NULL,"=", &last);
                        if(temp_size != NULL)	
			    pTag->text_size = atoi(temp_size);
		}

		if (color != NULL) {
			temp_color = strtok_r(color,"=", &last);
			temp_color = strtok_r(NULL,":", &last);
                        if(temp_color != NULL)
			    pTag->color.red =  atoi(temp_color);
			temp_color = strtok_r(NULL,":", &last);
                        if(temp_color != NULL)
			    pTag->color.green =  atoi(temp_color);
			temp_color = strtok_r(NULL,":", &last);
                        if(temp_color != NULL)
			    pTag->color.blue= atoi(temp_color);
		}

		if (location != NULL) {
			temp_location = strtok_r(location,"=", &last);
			temp_location = strtok_r(NULL,":", &last);
                        if(temp_location != NULL)
			    pTag->tag_x =  atoi(temp_location);
			temp_location = strtok_r(NULL,":", &last);
                        if(temp_location != NULL)
			    pTag->tag_y= atoi(temp_location);
		}

		strcpy(pTag->name, name);
		if (subname != NULL) {
			strcpy(pTag->subname, subname);
		}

		if (pTag->str != NULL) {
			free(pTag->str);
		}

		if (data != NULL) {
			pTag->str = (char *)malloc(strlen(data)+1);
			strcpy(pTag->str, data);
			if(DBG) ALOGD("data=%s", data);
			return strlen(data);
		} else if(strcmp(value,"NONE") == 0) {
                        if(DBG) ALOGD("value = NONE : show nothing");
			return -1;
		} else {
			pTag->str = (char *)malloc(strlen(value)+1);
			if(DBG) ALOGD("value: %s", value);
			strcpy(pTag->str, value);
			return strlen(value);
		}

	} else {
		ALOGE("error: string(name=%s, subname=%s) not found in xml file", name, subname);
		return -1;
	}

	return -1;
}

int TextList::addText(const char* name , const char* subname, const char* data) {

	struct TextTag * pTag = (struct TextTag *)malloc(sizeof(struct TextTag));
	struct TextTag * pOldTag;

	pTag->str = NULL;
	//pTag->textBuf = NULL;

	if(DBG) ALOGD("*****addText: name: %s", name);

	if (subname != NULL) {
		if(DBG) ALOGD("addText: subname: %s", subname);
	}

	if (data != NULL) {
		if(DBG) ALOGD("addText: data: %s", data);
	}


	if (getStr(name, subname, data, pTag) < 0) {
		free(pTag);
		return -1;
	}

	ALOGD("------addText: string: %s", pTag->str);
	if(DBG) ALOGD("addText: strlen(pTag->str)=%d", strlen(pTag->str));
	if(DBG) ALOGD("addText: size=%d", pTag->text_size);
	
	ALOGD("pTag->name=%s", pTag->name);

	addTag(&pTag);

	return 0;
}

void TextList::clearTextTagList(void) {
	struct list_head *pList;
	struct list_head *pListTemp;

	list_for_each_safe(pList, pListTemp, &mTagList) {
		struct TextTag * tag = list_entry(pList, struct TextTag, i_list);
		list_del(pList);
		if (tag) {
			if (tag->str != NULL) free(tag->str);
			//if (tag->textBuf != NULL) free(tag->textBuf);
			free(tag);
		}
	}
}


TextList::TextList(char * xml_path, char * font_path) {
	//strcpy(mFontPath, font_path);

	mStringmap= new StringMap(xml_path);
	INIT_LIST_HEAD(&mTagList);
}

TextList::~TextList() {
 	clearTextTagList();
        delete mStringmap;
}

       
