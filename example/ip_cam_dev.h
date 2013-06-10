/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  ip_cam_dev.h
 *       Compiler:  gcc
 *
 *    Description:  this file describe device, and abstruct phyical card or other data source  
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  05/20/10 16:13:53
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */




#ifndef  IP_CAM_DEV_INC
#define  IP_CAM_DEV_INC

#include <pthread.h>

typedef void (*DataFun)(void * handler, void* data);

typedef struct ip_cam_device
{
	int comp_fd;
	int status;
	pthread_t cmptid;
	void * datahandler;
	DataFun fun;
} Ip_Cam_Device;

int catchonemotion;

int catch_sen;
int		action_fd;

void set_action(int fd, int *sen);

int ip_cam_construct(Ip_Cam_Device * ipcam, char * devname);

int ip_cam_destruct(Ip_Cam_Device * ipcam);

int cam_start_work(Ip_Cam_Device * ipcam);


#endif   /* ----- #ifndef IP_CAM_DEV_INC  ----- */


