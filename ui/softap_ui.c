#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#define LOG_TAG "softap_ui"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "utility/string_ext.h"
#include "utility/list.h"
#include "../thread/lollipop_thread.h"
#include "../wifi/list_network.h"
#include "softap_ui.h"
#include "p2p_ui.h"
#include "../version.h"
#include "ui.h"

extern int softap_ui_tid;

void clear_ui_all(void)
{
	show_text("mode","switch", NULL);
	show_text("ap", NULL, "");
	show_text("device_ip0", NULL, "");
	show_text("device_ip1", NULL, "");
	show_text("information1", NULL, "");
	show_text("information2", NULL, "");
	show_text("ap_ssid1", NULL, "");
	show_text("ap_ssid2", NULL, "");
	show_text("or", NULL, "");
	show_text("webpage1", NULL, "");
	show_text("webpage2", NULL, "");
	show_text("info_ssid_1", NULL, "");
	show_text("info_ssid_2", NULL, "");
	show_text("when_connected_to_1", NULL, "");
	show_text("when_connected_to_2", NULL, "");
	show_text("new_update_notify", NULL, "");
	show_text("download", NULL, "");
	show_text("download_status", NULL, "");
	show_text("new_update_notify", NULL, "");
	show_text("download_progress", NULL, "");
	show_text("new_version", NULL, "");
	show_text("version_tag",NULL, "");
	show_text("version_no",NULL, "");
	show_text("device_name",NULL, "");
}

void clear_ap_info(void) {

	show_text("device_ip1", NULL, "");
	show_text("ap", NULL, "");
	show_text("webpage2", NULL, "");
	show_text("info_ssid_2", NULL, "");
	show_text("when_connected_to_2", NULL, "");
	show_text("ap_ssid2", NULL, "");
	show_text("or", NULL, "");
}

void clear_peer_info(void) {
	show_text("peer_device_name", NULL, "");
	show_text("peer_ip_address", NULL, "");	
}

void show_dongle_signal(void) {
	show_image("/system/usr/share/png/signal.png", 608, 220);
}

void hide_dongle_signal(void) {
	show_image("/system/usr/share/png/signal_n.png", 608, 220);
}

void show_ap_signal(void) {
	show_image("/system/usr/share/png/signal.png", 1030, 220);
}

void hide_ap_signal(void) {
	show_image("/system/usr/share/png/signal_n.png", 1030, 220);
}

void show_phone_connection(void) {
	show_image("/system/usr/share/png/arrow.png", 296, 360);
}

void hide_phone_connection(void) {
	show_image("/system/usr/share/png/arrow_n.png", 296, 360);
}

void show_dongle_connection(void) {
	show_image("/system/usr/share/png/arrow.png", 684, 360);
}

void hide_dongle_connection(void) {
	show_image("/system/usr/share/png/arrow_n.png", 684, 360);
}

#define FBIOPUT_SET_CURSOR_EN     0x4609
#define FBIOPUT_SET_CURSOR_IMG    0x460a
#define FBIOPUT_SET_CURSOR_POS    0x460b
#define FBIOPUT_SET_CURSOR_CMAP   0x460c
#define FBIOPUT_GET_CURSOR_RESOLUTION    0x460d
#define FBIOPUT_GET_CURSOR_EN    0x460e
#include <linux/fb.h>
#include <fcntl.h>
static void disable_hardware_cursor_and_back_full_screen(void) {
	// do again disable hardware cursor and back full screen here
	// because wfd process may exit abnormal without do this two work
	//disable hardware cursor
	int fb2 = open("/dev/graphics/fb2", O_RDWR, 0);
	if(fb2 < 0) {
	    ALOGE("open fb2 fail");
	} else {
	    int cursor_en = 0;
	    if (ioctl(fb2, FBIOPUT_SET_CURSOR_EN, &cursor_en) < 0) {
	        ALOGE("set cursor enable failed");
	    }
	    close(fb2);
	}
	//full screen
	FILE *fd = fopen("/sys/class/graphics/fb0/scale", "wr");
	if (fd == NULL){ 
	    ALOGE("open fb0 scale node failed");
	} else {
            char comp[16];
            getLollipopConf("fb_scale", comp);
	    if (fwrite(comp, strlen(comp), 1, fd) != 1) {
	       ALOGE("fwrite comp failed");
	    }
	    fclose(fd);
	}
}

void* softap_ui_thread(void* arg) {
	char device_name[32];
	char ip0[32];
	char ip1[32];
	char pwd[32];
	char peer_name[32];
	char peer_ip[32];
	char ssid[SSID_LEN_MAX];
	STRUCT_WIFI_SIGN *tmp_wifi_sign;
	int btconfig = 0;

	while(1) {
                bool needredraw = true;
		void * args;
		int msg = getMsg(HG1_app_ui_tid, &args);
			if (msg == 0) continue;

		ALOGD("tid %d receive msg: %d\n", softap_ui_tid, msg);
		if(msg == SOFTAP_UI_MSG_INIT)
			show_image("/system/usr/share/jpg/wfd.jpg",0,0);
			
		msg = getMsg(softap_ui_tid, &args);
		if (msg == 0) continue;
		ALOGD("tid %d receive msg: %d\n", softap_ui_tid, msg);
		switch (msg) {
			case SOFTAP_UI_MSG_INIT:{
				show_image("/system/usr/share/jpg/wfd.jpg",0,0);
				show_text("version_tag",NULL, NULL);
				show_text("version_no",NULL, VERSION);
				if (args && !strcmp(args, "btconfig")) {
					btconfig = 1;
				} else {				
				//show_text("mode", "dlna", NULL);
				show_image("/system/usr/share/png/phone.png", 151, 300);
				show_image("/system/usr/share/png/dongle.png", 618, 300);
				show_image("/system/usr/share/png/ap.png", 1004, 300);
#ifdef WIFI_BT_CONFIG
				show_image("/system/usr/share/png/qr.png", 10, 540);	
#else
				show_image("/system/usr/share/png/qr.png", 10, 540);
#endif
				hide_phone_connection();
				hide_dongle_signal();
				hide_dongle_connection();
				hide_ap_signal();
				}
				
				break;
			}
			case SOFTAP_UI_MSG_DEVICE_NAME:{
				show_text("device_name",NULL, (char *)args);
#ifdef WIFI_BT_CONFIG
				if (btconfig){
					show_text("wait_net_config_tag",NULL, NULL);
					show_image("/system/usr/share/png/qr.png", 10, 540);
					//show_text("btcfg_info1", NULL, NULL);
                    show_text("btcfg_info2", NULL, NULL);
                    show_text("btcfg_webpage", NULL, NULL);
				}
				else
#endif
				show_dongle_signal();
				memset(device_name, 0, sizeof(device_name));
				strcpy(device_name, (char *)args);
				break;
			}
			case SOFTAP_UI_MSG_STATUS: {
				if(args == NULL) break;
				int state = *(int *)args;
				ALOGD("state=%d", state);
				switch (state) {
					case SOFTAP_STATE2UI_WAITTING: {
						clear_ap_info();
						clear_peer_info();
						break;
					}
					case SOFTAP_STATE2UI_CONNECTED_TO_AP: {
						show_ap_signal();
						show_dongle_connection();
						break;				
					}
					case SOFTAP_STATE2UI_DISCONNECTED_FROM_AP: {
						clear_ap_info();
						hide_ap_signal();
						hide_dongle_connection();

						break;
					}
					default: {
						break;
					}
				}
				break;
			}
			case SOFTAP_UI_MSG_MODE_SWITCH: {
				show_text("mode","switch", NULL);
				show_text("ap", NULL, "");
				show_text("device_ip0", NULL, "");
				show_text("device_ip1", NULL, "");
				show_text("information1", NULL, "");
				show_text("information2", NULL, "");
				show_text("ap_ssid1", NULL, "");
				show_text("ap_ssid2", NULL, "");
				show_text("or", NULL, "");
				show_text("webpage1", NULL, "");
				show_text("webpage2", NULL, "");
				show_text("info_ssid_1", NULL, "");
				show_text("info_ssid_2", NULL, "");
				show_text("when_connected_to_1", NULL, "");
				show_text("when_connected_to_2", NULL, "");
				show_text("new_update_notify", NULL, "");
				show_text("download", NULL, "");
				show_text("download_status", NULL, "");
				show_text("new_update_notify", NULL, "");
				show_text("download_progress", NULL, "");
				show_text("new_version", NULL, "");

				break;
			}
			case SOFTAP_UI_MSG_PEER_NAME:
			case P2P_UI_MSG_PEER_NAME: {
				strcpy(peer_name, (char *)args);
				break;
			}
			case SOFTAP_UI_MSG_PEER_IP:
			case P2P_UI_MSG_PEER_IP: {
				strcpy(peer_ip, (char *)args);
				clear_peer_info();
				show_text("peer_ip_address", NULL, peer_ip);
				show_text("peer_device_name", NULL, peer_name);
				break;
			}
			case SOFTAP_UI_MSG_CLR_PEER: {
				clear_peer_info();
				break;
			}
			case SOFTAP_UI_MSG_IP0: {
#ifdef WIFI_BT_CONFIG
				show_text("btcfg_info1", NULL, NULL);
				show_text("btcfg_info2", NULL, NULL);
				show_text("btcfg_webpage", NULL, NULL);
#else
				char * tempBuf = (char *)malloc(256);
				//show_text("device_ip0", NULL, (char *)args);
				show_text("information1", NULL, NULL);
				sprintf(tempBuf, "'%s' (pwd: %s).", device_name, pwd);
				show_text("ap_ssid1", NULL, tempBuf);
				show_text("information2", NULL, NULL);
				strcpy(ip0, (char *)args);
				memset(tempBuf, 0, 256);
				sprintf(tempBuf, "http://%s/index.html", ip0);
				show_text("webpage1", NULL, tempBuf);
				free(tempBuf);
#endif
				break;
			}
			case SOFTAP_UI_MSG_PWD: {
				memset(pwd, 0, sizeof(pwd));
				strcpy(pwd, (char *)args);
                                needredraw = false;
				break;
			}
			case SOFTAP_UI_MSG_IP1: {
				show_text("device_ip1", NULL, (char *)args);
				memset(ip1, 0, sizeof(ip1));
				strcpy(ip1, (char *)args);
#ifdef WIFI_BT_CONFIG
                                show_text("btcfg_info1", NULL, NULL);
                                show_text("btcfg_info2", NULL, NULL);
                                show_text("btcfg_webpage", NULL, NULL);
#else
				char * tempBuf = (char *)malloc(256);
				show_text("information1", NULL, NULL);
				sprintf(tempBuf, "'%s'.", ssid);
				show_text("ap_ssid1", NULL, tempBuf);
				show_text("information2", NULL, NULL);
				strcpy(ip0, (char *)args);
				memset(tempBuf, 0, 256);
				sprintf(tempBuf, "http://%s/index.html", ip1);
				show_text("webpage1", NULL, tempBuf);
				free(tempBuf);
#endif
				break;
			}
			case SOFTAP_UI_MSG_NO_IP: {
				show_text("device_ip1", NULL, "NO IP");
				break;
			}
			case SOFTAP_UI_MSG_AP: {
				show_text("ap", NULL, (char *)args);
				strcpy(ssid, (char*)args);
			
				break;
			}
			case SOFTAP_UI_MSG_UPDATE: {
				lollipop_ui_update();
				break;
			}
			case SOFTAP_UI_MSG_HIDE: {
				lollipop_ui_hide();
				break;
			}
			case SOFTAP_UI_MSG_SHOW: {
				lollipop_ui_show();
				break;
			}
			case SOFTAP_UI_MSG_OTA_NEW_UPDATE: {		
				show_text("new_update_notify", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_NEW_VERSION: {
				show_text("new_version", NULL, (char *)args);
				break;
			}
			case SOFTAP_UI_MSG_OTA_DOWNLOAD_PERCENT: {
				int per = *(int *)args;
				char tempBuf[8] = {0};
				sprintf(tempBuf, "%%%d", per);
				show_text("download", NULL, NULL);
				show_text("download_progress", NULL, tempBuf);

				break;
			}
			case SOFTAP_UI_MSG_OTA_DOWNLOAD_COMPLETE: {
				show_text("download_status", "download_complete", NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_DOWNLOAD_FAIL: {
				show_text("download_status", "download_error", NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_CHECK_FW_OK: {
				show_text("firmware_ready", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_CHECK_FW_FAIL: {
				show_text("firmware_error", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_UI_INIT :{
				clear_ui_all();
				show_image("/system/usr/share/jpg/ota.jpg",0,0);
                                show_text("version_tag",NULL, NULL);
				show_text("version_no", NULL, VERSION);

				//show_text("mode_tag", NULL, NULL);
				show_text("mode", "ota", NULL);

				//show_text("update_to", NULL, NULL);
				show_text("warnning", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_UI_PRE : {
				show_text("new_update_waitting", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_OTA_UI_PRE_CNT: {
				if (args == NULL) {
					show_text("new_update_waitting_count", NULL, "");
					break;
				}
				char pre = *(char *)args;
				char *tempBuf = (char *)malloc(8);
				sprintf(tempBuf, "%d s", pre);
				show_text("new_update_waitting_count", NULL, (char *)tempBuf);
				free(tempBuf);
				break;
			}
			case SOFTAP_UI_MSG_OTA_UI_PRE_OK : {
				show_text("new_update_waitting_ok", NULL, NULL);
				break;
			}
			case SOFTAP_UI_MSG_SHOW_PCBA:{
				if (args == NULL) {
					ALOGD("======null singal\n");
					break;
				}
				char level[2] = {0};
				tmp_wifi_sign = (STRUCT_WIFI_SIGN*)args;
				level[0] ='0' + tmp_wifi_sign->level;
				level[1] = '\n';
				switch(tmp_wifi_sign ->num)
				{
				case 0:	
				{
					ALOGD("0\n");
					show_text("wifi_ap0", NULL, (char*)(tmp_wifi_sign->ssid));
					show_text("wifi_level0", NULL, (char*)level);
					break;
				}
				case 1:	
				{
					ALOGD("1\n");
					show_text("wifi_ap1", NULL, (char*)(tmp_wifi_sign->ssid));
					show_text("wifi_level1", NULL, (char*)level);
					break;
				}
				case 2:	
				{
					ALOGD("2\n");
					show_text("wifi_ap2", NULL, (char*)(tmp_wifi_sign->ssid));
					show_text("wifi_level2", NULL, (char*)level);
					break;
				}				
				case 3:	
				{
					ALOGD("3\n");
					show_text("wifi_ap3", NULL, (char*)(tmp_wifi_sign->ssid));
					show_text("wifi_level3", NULL, (char*)level);
					break;
				}
				case 4:	
				{
					ALOGD("4\n");
					show_text("wifi_ap4", NULL,(char*)( tmp_wifi_sign->ssid));
					show_text("wifi_level4", NULL, (char*)level);
					break;
				}	
				default:
				{
					ALOGE("SEND ERROR\n");
					break;
				}
			}
				break;
			}
			case P2P_UI_MSG_STATUS: {
				if(args == NULL) break;
				int state = *(int *)args;
				ALOGD("state=%d", state);
				switch (state) {
					case P2P_STATE2UI_INIT: {
					
						break;
					}
					case P2P_STATE2UI_WAITTING: {
						clear_peer_info();
						disable_hardware_cursor_and_back_full_screen();

						show_text("status","p2p_waiting", NULL);
						break;
					}
					case P2P_STATE2UI_CONNECTING: {
						show_text("status","p2p_connecting", NULL);
						break;				
					}case P2P_STATE2UI_OBTAIN_IP: {
						show_text("status","p2p_obtain_ip", NULL);
						break;
					}
					case P2P_STATE2UI_DISPATCH_IP: {
						show_text("status","p2p_dispatch_ip", NULL);
						break;
					}
					case P2P_STATE2UI_PREPARE_DISPLAY: {
						show_text("status","prepare_display", NULL);
						break;
					}
					case P2P_STATE2UI_READY_DISPLAY: {
						show_text("status","ready_to_display", NULL);
						break;
					}
					case P2P_STATE2UI_START_RTSP: {
						show_text("status","start_rtsp", NULL);
						break;
					}
					case P2P_STATE2UI_DISCONNECTING: {
						show_text("status","p2p_disconnecting", NULL);
						break;
					}
					case P2P_STATE2UI_DEVIE_LOST: {
						show_text("status","p2p_device_lost", NULL);
						break;
					}
					default: {
						break;
					}
				}
				break;
			}			
				
			default: {
				break;
			}
		}

                if (needredraw) 
		   lollipop_ui_update();
		if (args != NULL) free(args);
	}
}
