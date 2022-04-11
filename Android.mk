LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)  
LOCAL_MODULE := Rk3229_receive

LOCAL_SRC_FILES := \
		rk3229_receive.cpp  	\
		socket/Tcp.cpp			\
    	socket/UdpServer.cpp \
    	socket/UdpCombinePackage.cpp \
    	socket/TcpNativeInfo.cpp \
    	socket/SendPackageCache.cpp \
    	socket/RingBuffer.cpp  \
    	socket/TcpClient.cpp \
    	socket/UdpClient.cpp \
    	socket/CombinePackage.cpp \
    	decoder/avplayer.cpp
		
		
LOCAL_C_INCLUDES += \
			 $(LOCAL_PATH) \
			 $(LOCAL_PATH)/socket \
			 $(LOCAL_PATH)/decoder

LOCAL_CLANG := true
LOCAL_CPPFLAGS := 
LOCAL_MODULE_TAGS := eng optional


LOCAL_CFLAGS := -DANDROID 
LOCAL_CFLAGS += -DHAVE_CONFIG_H -DFLAC__NO_MD5 -DFLAC__INTEGER_ONLY_LIBRARY
LOCAL_CFLAGS += -D_REENTRANT -DPIC -DU_COMMON_IMPLEMENTATION -fPIC
LOCAL_CFLAGS += -O3 -funroll-loops -finline-functions
LOCAL_CFLAGS += -Wno-unused-result -Wno-unused-parameter  -Wno-unused-variable

LOCAL_CPPFLAGS:= -std=c++11 -pthread    


LOCAL_C_INCLUDES :=prebuilts/ndk/8/sources/cxx-stl/stlport/stlport
APP_STL := stlprot_static
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog \
	prebuilts/ndk/8/sources/cxx-stl/stlport/libs/armeabi-v7a/libstlport_static.a

LOCAL_SHARED_LIBRARIES := \
	libc \
	libstagefright liblog libutils libbinder libstagefright_foundation \
        libmedia libgui libcutils libui libtinyalsa libusbhost libz libbinder

ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3036)
LOCAL_SHARED_LIBRARIES += liblollipop_socket_ipc
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk312x)
LOCAL_SHARED_LIBRARIES += liblollipop_socket_ipc
endif

LOCAL_C_INCLUDES := \
	external/stlport/stlport \
        bionic/libstdc++/include \
        bionic \
	frameworks/av/media/libstagefright \
	$(TOP)/frameworks/native/include/media/openmax \


ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk3036)
LOCAL_C_INCLUDES += $(TOP)/external/lollipop_wifi/socket_ipc
LOCAL_CFLAGS += -DRK3036
endif
ifeq ($(strip $(TARGET_BOARD_PLATFORM)), rk312x)
LOCAL_C_INCLUDES += $(TOP)/external/lollipop_wifi/socket_ipc
LOCAL_CFLAGS += -DRK3036
endif

LOCAL_LDFLAGS := -ldl

include $(BUILD_EXECUTABLE)
