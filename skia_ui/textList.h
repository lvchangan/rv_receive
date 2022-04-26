#ifndef TEXT_LIST_H
#define TEXT_LIST_H

#include <utils/Log.h>
#include <utils/String8.h>
#include <ui/DisplayInfo.h>

#include <sys/stat.h>
#include <setjmp.h>
#include <ft2build.h>
#include FT_FREETYPE_H

#include "string_map.h"
//#include "fillBuffer.h"

#include <SkTypeface.h>
#include <SkTemplates.h>
#include <SkRegion.h>
#include <SkDevice.h>
#include <SkRect.h>
#include <SkCanvas.h>
#include <SkBitmap.h>

extern "C" {
	#include "../utility/list.h"
}

struct rgb_color {
        uint8_t red;
        uint8_t green;
        uint8_t blue;
};

struct TextTag{
	struct list_head  i_list;
	char	name[64];
	char	subname[64];
	char	*str;
	int	text_size;
	struct	rgb_color color;
	int	tag_x;
	int	tag_y;
	int	width;
	int	height;
	//unsigned char * textBuf;
};

class TextList {
	public:
		TextList(char * xml_path, char * fontPath);
		~TextList();
	
		int addText(const char* name , const char*subname, const char* data);
		struct list_head * getTextList();
	private:
		int addTag(struct TextTag ** pTag);
		int getTagByName(const char * name, struct TextTag ** pTag);
		int getStr(const char* name , const char* subname, const char* data, struct TextTag *pTag);
		void clearTextTagList(void);

		StringMap* mStringmap;
		struct list_head mTagList;

		//char mFontPath[256];
		//char mLanguage[64];
};

#endif  //TEXT_LIST_H
