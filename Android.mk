LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_MODULE := rkmpp
LOCAL_SRC_FILES := $(LOCAL_PATH)/libs/libmpp.so
include $(PREBUILT_SHARED_LIBRARY)


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
    	socket/CombinePackage.cpp
		
		
LOCAL_C_INCLUDES += \
			 $(LOCAL_PATH) \
			 $(LOCAL_PATH)/socket


#LOCAL_ARM_MODE := arm

LOCAL_MODULE_TAGS := optional

LOCAL_CFLAGS := -DANDROID 
LOCAL_CFLAGS += -DHAVE_CONFIG_H -DFLAC__NO_MD5 -DFLAC__INTEGER_ONLY_LIBRARY
LOCAL_CFLAGS += -D_REENTRANT -DPIC -DU_COMMON_IMPLEMENTATION -fPIC
LOCAL_CFLAGS += -O3 -funroll-loops -finline-functions
LOCAL_CFLAGS += -Wno-unused-result -Wno-unused-parameter  -Wno-unused-variable -Wno-Wsign-compare

LOCAL_CPPFLAGS:= -std=c++11 -pthread    

LOCAL_SHARED_LIBRARIES := rkmpp

LOCAL_SHARED_LIBRARIES := \
	libc \
#	libstlport \
#	libstagefright liblog libutils libbinder libstagefright_foundation \
#        libmedia libgui libcutils libui libtinyalsa libusbhost libz libbinder

LOCAL_C_INCLUDES := \
	external/stlport/stlport \
    bionic/libstdc++/include \
    bionic \
	frameworks/av/media/libstagefright \
	$(TOP)/frameworks/native/include/media/openmax \




LOCAL_C_INCLUDES :=prebuilts/ndk/8/sources/cxx-stl/stlport/stlport
APP_STL := stlprot_static
LOCAL_LDLIBS := -L$(SYSROOT)/usr/lib -llog \
	prebuilts/ndk/8/sources/cxx-stl/stlport/libs/armeabi-v7a/libstlport_static.a

include $(BUILD_EXECUTABLE)
