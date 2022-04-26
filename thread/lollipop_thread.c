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
#include <pthread.h>
#include "../utility/list.h"

#define LOG_TAG "lollipop_thread"
#include "cutils/log.h"
#include "cutils/memory.h"
#include "cutils/misc.h"
#include "cutils/properties.h"

#define DBG 0

int p2p_monitor_tid;
int p2p_handle_tid;
int p2p_listen_tid;
int p2p_ui_tid;
int softap_ui_tid;
int ota_ui_tid;
int ota_tid;
int reset_ui_tid;
int reset_tid;
int sm_switch_mode_tid;
int HG1_app_ui_tid;

struct lollipopMsgList_s {
	struct list_head i_list;
	int msg;
	void * args;
};

struct lollipopThread_s {
	struct list_head i_list;
	pthread_t tid;
	pthread_mutex_t mutex;
	pthread_cond_t eventCond;
	struct list_head msgList;
};

static struct list_head gThreadList;
pthread_mutex_t gThreadListMutex;

void sendMsg(int tid, int msg, void * args) {
	struct list_head * pList;
	struct lollipopThread_s * thr = NULL;
        int found = 0;

	pthread_mutex_lock(&gThreadListMutex);
	if(DBG) ALOGD("tid %d sendMsg %d", tid, msg);

	list_for_each(pList, &gThreadList) {
		thr = list_entry(pList, struct lollipopThread_s, i_list);
		if (tid == thr->tid) {
                        found = 1;
			break;
		}
	}

	pthread_mutex_unlock(&gThreadListMutex);

        if (!found) return;

	pthread_mutex_lock(&(thr->mutex));
	struct lollipopMsgList_s * msgList = (struct lollipopMsgList_s *)malloc(sizeof(*msgList));
	msgList->msg = msg;
	msgList->args = args;
	list_add_tail(&(msgList->i_list), &(thr->msgList));

	pthread_cond_signal(&(thr->eventCond));
	pthread_mutex_unlock(&(thr->mutex));
	if(DBG) ALOGD("sendMsg OUT");
}

int getMsg(int tid, void ** args) {
	int msg = 0;
	struct list_head * pList;
	struct list_head * pList2;
	struct lollipopThread_s * thr = NULL;
        int found = 0;

	pthread_mutex_lock(&gThreadListMutex);
	if(DBG) ALOGD("tid %d getMsg IN ----->", tid);

	list_for_each(pList, &gThreadList) {
		thr = list_entry(pList, struct lollipopThread_s, i_list);
		if (thr != NULL && tid == (int)(thr->tid)) {
                        found = 1;
			break;
		}
	}

	pthread_mutex_unlock(&gThreadListMutex);
        if (!found) return 0;

	pthread_mutex_lock(&(thr->mutex));

	/* enter sleep if event queue is empty */
	if (list_empty(&(thr->msgList))) {
		pthread_cond_wait(&(thr->eventCond), &(thr->mutex));
	}

	list_for_each(pList2, &(thr->msgList)) {
		struct lollipopMsgList_s * msgList = list_entry(pList2, struct lollipopMsgList_s, i_list);
		if (msgList != NULL) {
			msg = msgList->msg;
			ALOGD("get msg=%d", msg);
			*args = msgList->args;
			list_del(pList2);
			break;
		}
	}

	pthread_mutex_unlock(&(thr->mutex));

	if (DBG) ALOGD("getMsg OUT <-----");
	return msg;
}

int lollipop_thread_create(void*(*fn)(void*), void * arg) {
	struct lollipopThread_s * thr = (struct lollipopThread_s *)malloc(sizeof(*thr));
        memset(thr, 0, sizeof(struct lollipopThread_s));

	pthread_condattr_t eventattr;
	pthread_cond_init(&(thr->eventCond), &eventattr);
	pthread_mutex_init(&(thr->mutex), NULL);

	INIT_LIST_HEAD(&(thr->msgList));

	pthread_create(&(thr->tid), NULL, fn, arg);

	ALOGD("thread created!  tid=%d", (int)(thr->tid));

	pthread_mutex_lock(&gThreadListMutex);
	list_add(&(thr->i_list), &gThreadList);
	pthread_mutex_unlock(&gThreadListMutex);

	return (int)(thr->tid);
}

static int clear_msg_list(struct list_head * msgList) {
	struct list_head *pList;
	struct list_head *pListTemp;

	list_for_each_safe(pList, pListTemp, msgList) {
		struct lollipopMsgList_s * msg = list_entry(pList, struct lollipopMsgList_s, i_list);
		list_del(pList);
		if(msg != NULL) {
			if(msg->args != NULL) free(msg->args);
			free(msg);
		}
	}

	return 0;
}

void lollipop_thread_init(void) {
	INIT_LIST_HEAD(&gThreadList);
        pthread_mutex_init(&gThreadListMutex, NULL);
}

void lollipop_thread_deinit(void) {
	struct list_head *pList;
	struct list_head *pListTemp;

	list_for_each_safe(pList, pListTemp, &gThreadList) {
		struct lollipopThread_s *thr = list_entry(pList, struct lollipopThread_s, i_list);
		if(DBG) ALOGD("delete thread list, tid=%d", (int)(thr->tid));
		list_del(pList);
		if(thr != NULL) {
			clear_msg_list(&(thr->msgList));
			free(thr);
		}
	}
}
