#ifndef IMAGE_LIST_H
#define IMAGE_LIST_H

#include <utils/Log.h>
#include <utils/String8.h>
#include <ui/DisplayInfo.h>

#include <sys/stat.h>
#include <setjmp.h>

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

struct ImageTag{
	struct list_head  i_list;
	char	filepath[256];
	int	x;
	int	y;
};

class ImageList {
	public:
		ImageList();
		~ImageList();
	
		int addImage(const char* filepath , int x, int y);
		struct list_head * getImageList();
	private:
		int addTag(struct ImageTag ** pTag);
		void clearImageTagList(void);

		struct list_head mTagList;
};

#endif  //IMAGE_LIST_H
