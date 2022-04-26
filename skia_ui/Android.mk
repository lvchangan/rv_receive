
ifeq ($(UI_DEFINE), SKIA_UI)

LOCAL_PATH := $(call my-dir)
$(warning $(LOCAL_PATH))
include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
		skia_ui.cpp \
		textList.cpp \
		imageList.cpp \
		string_map.cpp \
		../utility/string_ext.c \

LOCAL_SHARED_LIBRARIES := \
		libutils \
		libcutils \
		libsurfaceflinger \
		libjpeg \
		libz \
		libgui \
                libui \
		liblollipop_config \
		libpng \
		libft2 \
		libskia

#LOCAL_STATIC_LIBRARIES := libpng \
			libft2 \

LOCAL_C_INCLUDES := \
	external/jpeg \
	external/libpng \
	external/freetype/include \
	external/zlib \
	external/jhead \
	external/skia/include/core \
	external/skia/include/utils \
	external/skia/include/images \
	external

#USE_MINIKIN=y
ifeq ($(USE_MINIKIN), y)
LOCAL_SHARED_LIBRARIES += \
	libutils \
	liblog \
	libcutils \
	libstlport \
	libharfbuzz_ng \
	libicuuc \
	libskia \
	libminikin \
	libft2

LOCAL_C_INCLUDES += \
	external/harfbuzz_ng/src \
	external/freetype/include \
	external/icu/icu4c/source/common \
	frameworks/minikin/include \
	frameworks/minikin/sample \
	external/skia/src/core

include external/stlport/libstlport.mk
LOCAL_CFLAGS += -DUSE_MINIKIN
endif

ifeq ($(strip $(BOARD_WITH_DONGLE_CHINESS_LANGUAGE)), true)
LOCAL_CFLAGS += -DSUPPORT_CHINESE_LANGUAGE
endif

LOCAL_MODULE:= libskia_ui

LOCAL_MODULE_TAGS := eng optional

include $(BUILD_SHARED_LIBRARY)

endif
