#ifndef LOLLIPOP_THREAD_H
#define LOLLIPOP_THREAD_H

extern int p2p_monitor_tid;
extern int p2p_handle_tid;
extern int p2p_listen_tid;
extern int p2p_ui_tid;
extern int softap_ui_tid;
extern int ota_ui_tid;
extern int ota_tid;
extern int reset_ui_tid;
extern int reset_tid;
extern int sm_switch_mode_tid;
extern int HG1_app_ui_tid;


extern void sendMsg(int tid, int msg, void * args);
extern int getMsg(int tid, void ** args);
extern int lollipop_thread_create(void*(*fn)(void*), void * arg);
extern void lollipop_thread_init(void);
extern void lollipop_thread_deinit(void);
#endif // LOLLIPOP_THREAD_H
