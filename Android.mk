LOCAL_PATH := $(call my-dir)

UI_DEFINE := SKIA_UI

include $(call all-subdir-makefiles)

$(warning $(LOCAL_PATH))


#=========================							
#HG1_app
include $(CLEAR_VARS)
LOCAL_SRC_FILES := \
		HG1_app.c \
		#device_name.c \
		utility/string_ext.c \
		#switch/switch.c \
		ui/softap_ui.c \

LOCAL_SHARED_LIBRARIES := \
		libutils \
		libnetutils \
		libhardware \
		libhardware_legacy \
		libcutils \
		liblollipop_socket_ipc \
		liblollipop_config \
		liblollipop_thread \
		liblollipop_wifi \
		libmedia \
		libcrypto \
		libstlport \
		liblollipop_web_operate

ifeq ($(UI_DEFINE), SKIA_UI)
LOCAL_SHARED_LIBRARIES += libskia_ui
endif

ifeq ($(UI_DEFINE), SURFACE_UI)
LOCAL_SHARED_LIBRARIES += libsurface_ui
endif

LOCAL_C_INCLUDES := \
		external/openssl/include \

ifeq ($(BOARD_WIFI_VENDOR), broadcom)
LOCAL_CFLAGS += -DBROADCOM_WIFI_VENDOR
endif
ifeq ($(BOARD_WIFI_VENDOR), realtek)
LOCAL_CFLAGS += -DRTK_WIFI_VENDOR
endif

ifeq ($(BOARD_USE_FLASH_SPI), true)
LOCAL_CFLAGS += -DUPDATE_RESET_SPI
endif

ifeq ($(BOARD_USE_ETHERNET_SOFTAP), true)
LOCAL_SRC_FILES += ethernet.c
LOCAL_CFLAGS += -DSUPPORT_ETHERNET_SOFTAP
endif

ifeq ($(BOARD_ENABLE_PCBA), true)
LOCAL_CFLAGS += -DENABLE_PCBA
endif

ifeq ($(BOARD_MIX_MODE_SUPPORT), true)
LOCAL_CFLAGS += -DMIX_MODE_SUPPORT
endif

ifeq ($(BOARD_AIRKISS_SUPPORT), true)
LOCAL_CFLAGS += -DAIRKISS_SUPPORT
endif

ifeq ($(BOARD_WIFI_BT_CONFIG_SUPPORT), true)
LOCAL_CFLAGS += -DWIFI_BT_CONFIG
endif

#LOCAL_LDLIBS := -lpthread -ldl
LOCAL_MODULE:= HG1_app
LOCAL_MODULE_TAGS := eng
include $(BUILD_EXECUTABLE) 