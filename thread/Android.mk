#=========================							
#liblollipop_thread

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= \
		lollipop_thread.c \

LOCAL_SHARED_LIBRARIES := \
		libutils \
		libcutils \

#LOCAL_LDLIBS := -lpthread -ldl
LOCAL_MODULE:= liblollipop_thread
LOCAL_MODULE_TAGS := eng optional
include $(BUILD_SHARED_LIBRARY)

								
