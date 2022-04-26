#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <signal.h>
#include <pthread.h>



#define LOG_TAG "HG1_app"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"
#include "netutils/dhcp.h"
#include "version.h"

#include "switch/switch.h"
#include "utility/string_ext.h"
#include "thread/lollipop_thread.h"
#include "ui/softap_ui.h"
#include "ui/p2p_ui.h"
#include "utility/none_stop_service.h"
#include "ota/ota_base.h"
#include "wifi/list_network.h"
#include "wifi/lollipop_wifiScanAp.h"
#include "wifi/operate.h"
#include "utility/list.h"

extern void* softap_ui_thread(void* arg);
extern void lollipop_ui_init(void);
extern void lollipop_ui_deinit(void);
extern void lollipop_ui_delete(void);
extern void lollipop_ui_reinit(void);


int main(int argc, char **argv)
{
	
	ALOGD("argc=%d", argc);
	ALOGD("argv[0]=%s", argv[0]);
	ALOGD("argv[1]=%s", argv[1]);

	lollipop_ui_init();
	lollipop_thread_init();

	HG1_app_ui_tid = lollipop_thread_create(softap_ui_thread, NULL);

	sendMsg(HG1_app_ui_tid, SOFTAP_UI_MSG_INIT, NULL);
	return 0;
}





