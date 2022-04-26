#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <poll.h>
#include <sys/utsname.h>
#include <sys/time.h>
#include <signal.h>
#include "cutils/properties.h"

#define LOG_TAG "lollipop_utility"
#include "cutils/log.h"
#define DBG 0

int exec_command(char * in, char * out, int outLen) {
        FILE * fstream;
        int readLen = 0;

        if(in == NULL) return -1;
        if(out == NULL) return -1;

        if(DBG) ALOGD("command: %s", in);
        if(NULL==(fstream=popen(in,"r"))) {
                ALOGE("execute command \"%s\" failed: %s", in, strerror(errno));
                return -1;
        }

        readLen = fread(out, 1, outLen, fstream);

        if(readLen <= 0) return -1;
        if(DBG) ALOGD("command return len=%d", strlen(out));
        if(DBG) ALOGD("command %s return: \n%s", in, out);

        pclose(fstream);
        return 0;
}

static const int NAP_TIME = 200; /* wait for 200ms at a time */
int wait_for_property(const char *name, const char *desired_value)
{
    char value[PROPERTY_VALUE_MAX] = {'\0'};
	int maxwait = 5; // seconds
    int maxnaps = (maxwait * 1000) / NAP_TIME;

    if (maxnaps < 1) {
        maxnaps = 1;
    }

    while (maxnaps-- > 0) {
        usleep(NAP_TIME * 1000);
        if (property_get(name, value, NULL)) {
            if (desired_value == NULL ||
                    strcmp(value, desired_value) == 0) {
                ALOGD("%s: %s==%s success (maxnaps=%d)", __func__, name, desired_value, maxnaps);
                return 0;
            }
        }
    }
	ALOGD("%s: %s==%s failed (maxnaps=%d)", __func__, name, desired_value, maxnaps);
    return -1; /* failure */
}


