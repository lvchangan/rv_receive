#ifndef _RV108_RECEIVE_H_
#define _RV108_RECEIVE_H_

#include <cstdio>
#include <sys/types.h>
#include <unistd.h>
#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <sys/socket.h>
#include <netinet/in.h>
#include <list>
#include <fcntl.h>

#include "socket/Event.h"

using namespace std;
/*
void decoder_h264_test();

typedef struct DecoderH264Buffer_s{
	int width;
	int height;
	int h264size;
	unsigned char *buffer;
}DecoderH264Buffer;

static pthread_mutex_t mutex_H264Buffer_data;
static list <DecoderH264Buffer *> H264BufferList;

static Event h264_event;
*/

#endif
