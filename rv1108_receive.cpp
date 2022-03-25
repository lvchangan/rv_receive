#include <stdio.h>
#include "socket/Tcp.h"
#include "socket/TcpNativeInfo.h"
#include "rv1108_receive.h"
#include "UdpServer.h"

static TcpNativeInfo tcpNativeInfo;
static FILE *fp_H264;
static FILE *fp_aac;


static int fps = 0;
static int fps_aac = 0;
static int _on_socket_received(void *UNUSED(user), TcpClient *client, int type, const char *msg,
                             const void *data, int64_t off, long len)
{
    //printf("________0________on_socket_received type=0x%x\n", type);
	if(type == TYPE_NEW_CLIENT)
	{

	}
	else if(type == TYPE_CLIENT_NAME)
	{

	}
	else if(type == TYPE_MEDIA_VIDEODATA)
	{
		/*
		fps++;
		if(fps < 20)
		fwrite(data,1,len,fp_H264);
		if(fps == 20)
			fclose(fp_H264);
		*/
	}
	else if(type == TYPE_MEDIA_AUDIODATA)
	{
		fps_aac++;
		if(fps_aac < 500)
		fwrite(data,1,len,fp_aac);
		if(fps_aac == 500)
			fclose(fp_aac);
	}
	return 0;
}
							 

int main()
{
	//fp_H264 = fopen("/data/receive.h264","wb+");
	
	UdpServer *receive_UDP = new UdpServer();
	
	
	fp_aac = fopen("/data/receive.aac","wb+");
    tcpNativeInfo.SetCallback(nullptr, _on_socket_received);
	Tcp *rv1108_receive = new Tcp(&tcpNativeInfo);
	rv1108_receive->StartServer(6666);
	receive_UDP->StartServer(rv1108_receive,6666);
	while(1);
	delete rv1108_receive;
}

