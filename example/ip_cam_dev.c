/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  ip_cam_dev.c
 *       Compiler:  gcc
 *
 *    Description:  implement ip cam device 
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  05/20/10 17:07:23
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>


#include <unistd.h>
#include <sys/mman.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include "spct_type.h"
#include "spct6100_dev.h"
#include "ip_cam_dev.h"
#include "xml.h"

#define CAM_STATUS_RUN	0x1234
#define CAM_STATUS_INVAL 0
#define CAM_STATUS_FREE 0xabcd

#ifndef ENABLE
#define ENABLE 1
#endif

#ifndef DISABLE
#define DISABLE 0
#endif

#define PAL_WIDTH_SIZE		352
#define PAL_HEIGHT_SIZE		288
#define NTSC_WIDTH_SIZE		352
#define NTSC_HEIGHT_SIZE	240

static int fd_video1 = 0;

typedef struct query_buf_res
{
	void * start;
	int length;
} Query_Buf_Res;


#define EXIT(x)                                                         \
    do{                                                                 \
        fprintf(stderr, "%s:%d failed: %s\n", __FILE__, __LINE__, x);   \
        exit(1);                                             \
    }while(0);

struct av_packet_header{
	unsigned int header_size;      //0x0
	unsigned int packet_size;      //0x04
	unsigned int pad0;             //0x08
	unsigned int frame_type;       //0x0c
	unsigned int video_timestamp;  //0x10
	unsigned int video_length;     //0x14
	unsigned int audio_length;     //0x18
	unsigned int reserved0;        //0x1c bsps
	unsigned int video_offset;     //0x20
	unsigned int audio_offset;     //0x24
	unsigned int reserved1;        //0x28 channel id
	char videolost_enable;         //0x2c
	char motion_enable;            //0x2d
	char encode_enable;            //0x2e
	char audio_enable;             //0x2f
	unsigned int bvideolost;       //0x30
	unsigned int motion_length;    //0x34
	unsigned int audio_timestamp;  //0x38
	unsigned char pad1[88 - 0x3c]; //0x3c
} __attribute__((packed));

struct packet_header{
	unsigned int video_flag;
	unsigned int video_type;
	char * video_addr;
	unsigned int video_length;
	unsigned int video_timestamp;

	unsigned int audio_flag;
	char * audio_addr;
	unsigned int audio_length;
	unsigned int audio_timestamp;

	unsigned int motion_flag;
	char * motion_addr;
	unsigned int motion_length;

	unsigned int videolost_flag;
};

/* buffer used between ap layer and v4l2method layer*/
struct buffer_av{
    unsigned int buf_index;
    unsigned int input;
    unsigned int flags;
    unsigned int length;
    unsigned int offset;
    struct timeval timestamp;
	unsigned int bytesused;
};

int streaming_off(int fd) {
    enum v4l2_buf_type type;
    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return ioctl(fd, VIDIOC_STREAMOFF, &type);
}



static int request_buffer(int fd) {
    struct v4l2_requestbuffers req;

    memset (&(req), 0, sizeof (req));
    req.count = 12;
    req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    req.memory = V4L2_MEMORY_MMAP;
    if( ioctl(fd, VIDIOC_REQBUFS, &req) != 0) {
        fprintf (stderr, "SPCT6100 V4L2 Driver does not support memory mapping\n");
        exit (EXIT_FAILURE);
    }

    return req.count; 
}

static int query_buffer(int fd, int index, unsigned int *length, unsigned int *offset ) {
    struct v4l2_buffer buf;

    memset(&(buf), 0, sizeof(buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    if(ioctl(fd, VIDIOC_QUERYBUF, &buf) !=0) {
        EXIT ("VIDIOC_QUERYBUF");
    }
    *length = buf.length;
    *offset = buf.m.offset;

    return 0;
}

static int queue_buffer(int fd, int index) {
    struct v4l2_buffer buf;

    memset (&(buf), 0, sizeof (buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;
    buf.index = index;
    return ioctl(fd, VIDIOC_QBUF, &buf);
}

static int dequeue_buffer(int fd, struct buffer_av *buf_av) {
    struct v4l2_buffer buf;

    memset (&(buf), 0, sizeof (buf));
    buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    buf.memory = V4L2_MEMORY_MMAP;

    if(ioctl(fd, VIDIOC_DQBUF, &buf) !=0) {
        EXIT ("VIDIOC_DQBUF");
    }
    buf_av->buf_index = buf.index;
    buf_av->input = buf.input;
    buf_av->flags = buf.flags;
    buf_av->length = buf.length;
    buf_av->offset = buf.m.offset;
    buf_av->timestamp = buf.timestamp;
    buf_av->bytesused = buf.bytesused;

    return 0;
}

static int streaming_on(int fd) {
    enum v4l2_buf_type type;

    type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
    return ioctl(fd, VIDIOC_STREAMON, &type);
}

void set_action(int, int *);

static unsigned int do_motion_process(char * addr, unsigned int length)		//动态采集具体处理
{
	int i = 0;
	
	static int count = 0;
	static char flag;
		if (1) {
			//printf("--------motion detection start-------------\n");
			
			for (i = 0; i < 36; i++) {
				if (*(addr + i) != 0) {
					//printf("i %d;   bit %x\n", i, *(addr + i));
					flag = 1;
					//printf("catch_sen %d \n", catch_sen);
				}
			}	
			
			for (i = 0; i < 9; i++) {
				if (*((unsigned int *)addr+i) != 0) {
					//printf("i %d; bit %x \n", i, *((unsigned int *)addr+i));
				}
			}
			
		}
	

	if (flag) {
		flag = 0;
		return 1;
	}
	return 0;
}

void set_action(int fd, int *sen);

#define		BASIC_LEVEL		35500
#define		BASIC_DISP		500
#define		BASIC_DIME		20000
#define		LEVEL_AMOUNT	5


int BASIC_LEVEL_ = 20000;
/*
static int is_redict = 1; //0 闇�瑕侀噸瀹氬悜
static int packet_count = 0;
static int set_ok = 0;
*/
int is_action = 0;

static void parse_packet(char ** packet_addr, struct packet_header * pkt_header)
{
	struct av_packet_header * av_pktp;

	av_pktp = (struct av_packet_header *)*packet_addr;

	if (av_pktp->encode_enable != 0 && av_pktp->video_length > 0){				//视频解码???根据av_pktp的内容修改pkt_header内容
		pkt_header->video_flag = 1;
		if(av_pktp->frame_type == 0)
			pkt_header->video_type = DATA_VIDEO_I;
		else if(av_pktp->frame_type == 2)
			pkt_header->video_type = DATA_VIDEO_P;
		else if(av_pktp->frame_type == 3)
			pkt_header->video_type = DATA_VIDEO_B;
		pkt_header->video_addr = (char *)av_pktp + av_pktp->video_offset;
		pkt_header->video_length = av_pktp->video_length;
		pkt_header->video_timestamp = av_pktp->video_timestamp;
	}
	else{
		pkt_header->video_flag = 0;
	}

	if (av_pktp->audio_enable != 0 && av_pktp->audio_length > 0){			//音频
		pkt_header->audio_flag = 1;
		pkt_header->audio_addr = (char *)av_pktp + av_pktp->audio_offset;
		pkt_header->audio_length = av_pktp->audio_length;
		pkt_header->audio_timestamp = av_pktp->audio_timestamp;
	}
	else{
		pkt_header->audio_flag = 0;	
	}

	if (av_pktp->motion_enable != 0 && av_pktp->motion_length > 0){			//动态采集使能
		char *ptr_bitmap;
		//pkt_header->motion_flag = 1;
		pkt_header->motion_addr = (char *)av_pktp + av_pktp->header_size;		//获取动作首地址
		pkt_header->motion_length = av_pktp->motion_length;						//获取动作长度(灵敏度???)
		ptr_bitmap = pkt_header->motion_addr + sizeof(struct av_packet_header);
		if (pkt_header->motion_flag = do_motion_process(pkt_header->motion_addr, pkt_header->motion_length)) {}
		/*
			if (is_redict && set_ok) {
				packet_count = 0;
				BASIC_LEVEL_ += BASIC_DISP;	
				set_action(fd_video1, &catch_sen);			
				printf ("\n\n\n   BASIC_LEVEL_  %d \n\n\n", BASIC_LEVEL_);
			}
		*/		
	else/*{
		if (is_redict) {
			packet_count++;
			if (packet_count == 5000)
			{
				is_redict = 0;
				for (packet_count = 0; packet_count < 100; packet_count++)
				printf ("\n\n\n   redict  okokokokokokok  %d \n\n\n", BASIC_LEVEL_);
			}
		}
		*/
		pkt_header->motion_flag = 0;
	}

	if (av_pktp->videolost_enable != 0 && av_pktp->bvideolost != 0){
	//if (av_pktp->bvideolost != 0){
		pkt_header->videolost_flag = 1;
	}
	else{
		pkt_header->videolost_flag = 0;
	}

	*packet_addr = (char*)*packet_addr + av_pktp->packet_size;
	return;
}


void set_action(int fd, int *sen)
{
	int i;
	int index;
	struct v4l2_control v4l2;
    int thres_level;
    int time_level;

    time_level = BASIC_DIME;
    thres_level = BASIC_LEVEL_ + BASIC_DISP*(*sen);		//得到动态采集阈值
	
	
	for (index = 0; index < 1; index++) {				//为什么这儿要用for循环?
		v4l2.id = V4L2_CID_MOTIONDETECT;
		v4l2.value = 0;									//关闭动作检测
		if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))	//选择S输入
				perror("VIDIOC_S_INPUT");
	
		if (-1 == ioctl(fd, VIDIOC_S_CTRL, &v4l2)) 		//得到或设置控制值
				perror("VIDIOC_S_CTRL V4L2_CID_MOTIONDETECT\n\n");
		else
			printf("\nMOTIONDETECT close successful\n");
				
		v4l2.id = V4L2_CID_MOTIONDETECT;				//开启动作检测
		v4l2.value = 1;
		if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))	//设置输入通道,没有视频输入则会出错，返回EINVAL(index用来存放输入的序号)
				perror("VIDIOC_S_INPUT");
	
		if (-1 == ioctl(fd, VIDIOC_S_CTRL, &v4l2)) 		//设置现在控制字的状态，v4l2该结构体告诉函数现在控制状态是什么
				perror("VIDIOC_S_CTRL V4L2_CID_MOTIONDETECT\n\n");
			
		else {											//成功设置动态检测
			printf("set action catch successfull\n\n");
			v4l2.id = V4L2_CID_MTHRESHOLD;				//动作采集阈值地址
			v4l2.value = thres_level;					//阈值大小

			if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))
				perror("V4L2_CID_MTHRESHOLD VIDIOC_S_INPUT");

			if (-1 == ioctl(fd, VIDIOC_S_CTRL, &v4l2)) 		//设置动态采集阈值
				perror("VIDIOC_S_CTRL V4L2_CID_MTHRESHOLD\n\n");
			
			else {
				printf("set threshold catch successfull\n\n");
				
				v4l2.id = V4L2_CID_MINTERVAL;				//设置动作检测间隔
				v4l2.value = time_level;					//时间单位是多少??????

				if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))
					perror("V4L2_CID_MINTERVAL VIDIOC_S_INPUT");
	
				if (-1 == ioctl(fd, VIDIOC_S_CTRL, &v4l2)) 
						perror("VIDIOC_S_CTRL V4L2_CID_MINTERVAL\n\n");
			
				else 
					printf("set INTERVAL catch successfull\n\n");
				
			}
		}

		if (-1 == ioctl(fd, VIDIOC_S_INPUT, &index))			//设置视频输入通道
			perror("VIDIOC_S_INPUT");

		memset (&(v4l2), 0, sizeof (v4l2));						//清空结构体阈值
		v4l2.id = V4L2_CID_MTHRESHOLD;

		if (-1 == ioctl(fd, VIDIOC_G_CTRL, &v4l2))				//获得控制字
			perror("VIDIOC_G_CTRL");	
		thres_level = v4l2.value;								//保存获得的动态采集阈值
		
		memset (&(v4l2), 0, sizeof (v4l2));
		v4l2.id = V4L2_CID_MINTERVAL;
		if (-1 == ioctl(fd, VIDIOC_G_CTRL, &v4l2))
			perror("VIDIOC_G_CTRL");
		time_level = v4l2.value;								//获得动态采集间隔时间
		*sen = ((thres_level - BASIC_LEVEL_)/BASIC_DISP)%LEVEL_AMOUNT;
		
		printf("\n-----current action level is %d    --------\n", thres_level);
		printf("\n------------catch_sensity is %d------------\n", *sen);
		
	}
}

void IsSetaction(int action_fd)			//检测灵敏度
{
	static int last_catch_sen = -1;		//静态变量，一直保持

	if (action_fd < 1)
		return;	
	
	if (last_catch_sen != catch_sen)		//检测是否需要修改灵敏度
	{
		set_action(action_fd, &catch_sen);
		last_catch_sen = catch_sen;
		//set_ok = 1;
	}
}


#define CHS_PER_CARD 4


static void * comp_proc(void * para)
{
	int i, j;
	int fd;
	int channel;
	int n_buffers = 0;
	unsigned int length, offset;
	int index, num;
	struct buffer_av buf_av;
	int result;
	struct pollfd pfd;
	unsigned int buf_index;
	unsigned int buf_num;
	char * packet_addr;
	struct packet_header pkt_header;
	Query_Buf_Res * buffers = NULL;				//用于保存获取视频数据
	Spct_Data comp_data;

	int got_video[CHS_PER_CARD];
	int got_audio[CHS_PER_CARD];
	struct timeval v_time_stamp[CHS_PER_CARD];
	struct timeval a_time_stamp[CHS_PER_CARD];
	unsigned int last_vtstamp[CHS_PER_CARD];
	unsigned int last_atstamp[CHS_PER_CARD];
	unsigned int duration;

	
	printf("thread compress\n");

	Ip_Cam_Device * ipcam = (Ip_Cam_Device *) para;

	memset(&buf_av, 0, sizeof(buf_av));

	memset(&got_video, 0, sizeof(got_video));
	memset(&got_audio, 0, sizeof(got_audio));
	memset(&last_vtstamp, 0, sizeof(last_vtstamp));
	memset(&last_atstamp, 0, sizeof(last_atstamp));


	fd = ipcam->comp_fd;
	action_fd = fd;
	n_buffers = request_buffer(fd);

	buffers = calloc(n_buffers, sizeof(* buffers));
	if(!buffers)
		EXIT("Out of memory.");

	for(i = 0; i < n_buffers; ++i)
	{
		if(query_buffer(fd, i, &length, &offset) != 0)
			EXIT("VIDIOC_QUERYBUF");
		buffers[i].length = length;
		buffers[i].start = mmap(NULL, length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
		printf("buffers[%d].start = 0x%x\n",i, buffers[i].start);
		if(MAP_FAILED == buffers[i].start)
			EXIT("mmap");
	}

	for(index = 0; index < n_buffers; index++)
		if(queue_buffer(fd, index) != 0)
			EXIT("VIDIOC_QBUF");

	if(streaming_on(fd) != 0)
		EXIT("VIDIOC_STREAMON");
	printf("card stream on================================\n");
	pfd.fd = fd;
	pfd.events = POLLIN;

	int comp_proc_count = 0;
	
	//set_action(fd, catch_sen); // fd threshold time

	while(ipcam->status == CAM_STATUS_RUN)					//检测ipcam是否处于运行状态
	{
		comp_proc_count++;
		if(comp_proc_count == 3000)							//定时打印信息
		{
			comp_proc_count = 0;
			printf("\n----- comp_proc thread running -----\n");
		}
		
		//IsSetaction(fd);
		
		result = poll(&pfd, 1, 15000);						//一个结构体，函数阻塞时间为15s

		if(result < 0)										//函数调用失败
			printf("pool ing errro ==================\n");
		if(result == 0)										//在规定时间内没有检测到可读套接字
		{
			printf("pool ing time out --------------\n");
			exit(1);
		}

		if(result < 0)										//??????????
			continue;
		if(result == 0)
			continue;
		
		dequeue_buffer(fd, &buf_av);
		buf_index = buf_av.buf_index;
		buf_num = buf_av.length;
		for(i = 0; i < CHS_PER_CARD; i++)
		{
			index = buf_index & 0xff;
			num = buf_num & 0xff;

			buf_index >>= 8;
			buf_num >>= 8;
			if(index != 0xff)
			{
				/*DATA PACKET*/
				channel = i;

				for(j = 0, packet_addr = buffers[index].start; j < num; j++)
				{
					parse_packet(&packet_addr, &pkt_header);					//数据包处理，动态处理就在此实现

					//pkt_header.motion_addr, 4 * 9
					if(pkt_header.motion_flag)			//捕捉到动作
					{
						printf("\n-------------receive a motion------------\n\n");	
						catchonemotion = 1;
						is_action = 1;
					}
					
					if (pkt_header.videolost_flag)
					{
						// printf("video lost\n");
					}

					if(pkt_header.audio_flag)
					{//length
						if(!got_audio[channel])
						{
							got_audio[channel] = 1;
							//a_time_stamp[channel] = buf_av.timestamp;
							a_time_stamp[channel].tv_usec = ((pkt_header.audio_timestamp % 32768) * 1000ULL * 1000ULL) >> 15;
							a_time_stamp[channel].tv_sec = (pkt_header.audio_timestamp >> 15) + (a_time_stamp[channel].tv_usec / 1000000);
							a_time_stamp[channel].tv_usec %= 1000000;
						}
						else
						{
							duration = pkt_header.audio_timestamp - last_atstamp[channel];
							a_time_stamp[channel].tv_usec += ((duration % 32768) * 1000ULL * 1000ULL) >> 15;
							a_time_stamp[channel].tv_sec += (duration >> 15) + (a_time_stamp[channel].tv_usec / 1000000);
							a_time_stamp[channel].tv_usec %= 1000000;

						}
						last_atstamp[channel] = pkt_header.audio_timestamp;

						//printf("audio frame\n");
						comp_data.channel = channel;
						comp_data.type = DATA_AUDIO;
						comp_data.flags = 0;
						comp_data.timestamp = a_time_stamp[channel];
						comp_data.size = pkt_header.audio_length;
						comp_data.data = pkt_header.audio_addr;
						ipcam->fun(ipcam->datahandler, &comp_data);
					}

					if(pkt_header.video_flag)
					{

						if(!got_video[channel])
						{
							got_video[channel] = 1;
							//v_time_stamp[channel] = buf_av.timestamp;
							v_time_stamp[channel].tv_usec = ((pkt_header.video_timestamp % 32768) * 1000ULL * 1000ULL) >> 15;
							v_time_stamp[channel].tv_sec = (pkt_header.video_timestamp >> 15) + (v_time_stamp[channel].tv_usec / 1000000);
							v_time_stamp[channel].tv_usec %= 1000000;
						}
						else
						{
							duration = pkt_header.video_timestamp - last_vtstamp[channel];
							v_time_stamp[channel].tv_usec += ((duration % 32768) * 1000ULL * 1000ULL) >> 15;
							v_time_stamp[channel].tv_sec += (duration >> 15) + (v_time_stamp[channel].tv_usec / 1000000);
							v_time_stamp[channel].tv_usec %= 1000000;
						}
						last_vtstamp[channel] = pkt_header.video_timestamp;

						comp_data.channel = channel;
						comp_data.type = DATA_VIDEO;
						comp_data.flags = pkt_header.video_type;
						comp_data.timestamp = v_time_stamp[channel];
						comp_data.size = pkt_header.video_length;
						comp_data.data = pkt_header.video_addr;
						ipcam->fun(ipcam->datahandler, &comp_data);
					}
				}
			}
		}
		queue_buffer(fd, buf_av.buf_index);
	}
	
	printf("\n-----comp_proc Thread exit-----\n");
	sleep(1);
	system("reboot");

	if(streaming_off(fd) != 0)
		EXIT("VIDIOC_STREAMOFF");

	for(i = 0; i < n_buffers; ++i)
		munmap(buffers[i].start, buffers[i].length);

	if(buffers)
		free(buffers);

	return NULL;
}



static int ip_cam_open_device(Ip_Cam_Device * ipcam, char * devname)
{
	int i;
	int fd;
	int ch_num;
	int ch_opt = 0;
	ch_num = 4;
	fd = open(devname, O_RDWR);
	if (fd < 0)
		return -1;
	if (ch_num <= 0)
		return -1;

	ipcam->comp_fd = fd;
	fd_video1 = ipcam->comp_fd;
	return 0;
}

int ip_cam_construct(Ip_Cam_Device * ipcam, char * devname)
{
	int res ;
	if(ipcam == NULL)
		return -1;
	if(ip_cam_open_device(ipcam, devname))
		return -1;
	ipcam->cmptid = 0;
	return 0;
}

int ip_cam_destruct(Ip_Cam_Device * ipcam)
{
	ipcam->status = CAM_STATUS_FREE;
	if(ipcam->cmptid)
		pthread_join(ipcam->cmptid, NULL);
	if(ipcam && ipcam->comp_fd)
		close(ipcam->comp_fd);
	return 0;
}

int cam_start_work(Ip_Cam_Device * ipcam)
{
	if(pthread_create(&ipcam->cmptid, NULL, comp_proc, (void*)ipcam))
	{
		return -1;
	}

	//dev stream on
	ipcam->status = CAM_STATUS_RUN;

	return 0;
}
	
int Init_Webserver_Par()
{
    int channelnum = 0;
    int new_brightness_value = 0;
	int new_contrast_value = 0;
	int new_saturation_value = 0;
	int new_hue_value = 0;
    struct v4l2_control brightness_control;
	struct v4l2_control contrast_control;
	struct v4l2_control saturation_control;
	struct v4l2_control hue_control;
	struct v4l2_control eqset_control;
	struct v4l2_control osdset_control;

	unsigned char *brightnessre;
	unsigned char *contrastre;
	unsigned char *saturationre;
	unsigned char *huere;
	unsigned char *cbrre;
	unsigned char *cbrminre;
	unsigned char *cbrmaxre;
	//unsigned char *vbrre;
	unsigned char *vbrqpire;
	unsigned char *vbrqpbre;
	unsigned char *vbrqppre;
	//unsigned char *osdstatusre;
	//unsigned char *datetimestatusre;
	//unsigned char *titlestatusre;

	
	typedef struct spct6100_eqset
	{
		unsigned int type;
		unsigned int minbitrate;
		unsigned int maxbitrate;
		unsigned int qpi;
		unsigned int qpb;
		unsigned int qpp;
	}Spct6100_Eqset, *Spct6100_Eqset_Ptr;
	Spct6100_Eqset_Ptr eqset_ptr;
	Spct6100_Eqset eqset_str;
	
	typedef struct spct6100_osdset
	{
		unsigned int tran;
		unsigned int acolor;
		unsigned int vscale;
		unsigned int hscale;
		unsigned int timer_enable;
		unsigned int osd_enable;
		unsigned int osd_pos_x;
		unsigned int osd_pos_y;
		unsigned int str[16];
	}Spct6100_Osdset, *Spct6100_Osdset_Ptr;
	Spct6100_Osdset_Ptr osdset_ptr;
	Spct6100_Osdset osdset_str;
	
	
/////////////////////////////////////////////////////////////////////////////////////////////	
	unsigned char brightnesswr[10];
	unsigned char contrastwr[10];
	unsigned char saturationwr[10];
	unsigned char huewr[10];
	unsigned char cbrwr[10];
	unsigned char cbrminwr[10];
	unsigned char cbrmaxwr[10];
	//unsigned char *vbrre;
	unsigned char vbrqpiwr[10];
	unsigned char vbrqpbwr[10];
	unsigned char vbrqppwr[10];
	if(read_at24c02b(0) == 11)
	{
		printf("\n--- read eeprom init webpar ---\n");
		int i;
		for (i=0; i<12; i++)
		{
			printf("\n--- read_at24c02b(%d) = %d ---\n", i, read_at24c02b(i));
		}
		sprintf(brightnesswr, "%d", read_at24c02b(1));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 1, 0, 0, "VIDEO_BRIGHT",brightnesswr);
		sprintf(contrastwr, "%d", read_at24c02b(2));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 2, 0, 0, "VIDEO_CONTRAST",contrastwr);
		sprintf(saturationwr, "%d", read_at24c02b(3));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 3, 0, 0, "VIDEO_SATURATION",saturationwr);
		sprintf(huewr, "%d", read_at24c02b(4));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 4, 0, 0, "VIDEO_HUE",huewr);
		if(read_at24c02b(5) == 1)
		{
			XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 5, 0, 0, "CBRStatus","Enable");
		}
		else if(read_at24c02b(5) == 0)
		{
			XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 5, 0, 0, "CBRStatus","Disable");
		}
		sprintf(cbrminwr, "%d", read_at24c02b(6));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 6, 0, 0, "CBR_MIN",cbrminwr);
		sprintf(cbrmaxwr, "%d", read_at24c02b(7));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 7, 0, 0, "CBR_MAX",cbrmaxwr);
		if(read_at24c02b(8) == 1)
		{
			XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 8, 0, 0, "VBRStatus","Enable");
		}
		else if(read_at24c02b(8) == 0)
		{
			XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 8, 0, 0, "VBRStatus","Disable");
		}
		sprintf(vbrqpiwr, "%d", read_at24c02b(9));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 9, 0, 0, "QPI",vbrqpiwr);
		sprintf(vbrqpbwr, "%d", read_at24c02b(10));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 10, 0, 0, "QPB",vbrqpbwr);
		sprintf(vbrqppwr, "%d", read_at24c02b(11));
		XmlChange("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 11, 0, 0, "QPP",vbrqppwr);
	}
	
//////////////////////////////////////////////////////////////////////////////////////////
	/*osdstatusre = XmlRead("/tmp/spct6100/www/setup/video_osd.xml", 2, 1, 0, 0, "OSDStatus");
		printf("\n-----osdstatusre = %s -----\n",osdstatusre);
	datetimestatusre = XmlRead("/tmp/spct6100/www/setup/video_osd.xml", 2, 2, 0, 0, "DateTimeStatus");
		printf("\n-----datetimestatusre = %s -----\n",datetimestatusre);
	titlestatusre = XmlRead("/tmp/spct6100/www/setup/video_osd.xml", 2, 3, 0, 0, "TitleStatus");
		printf("\n-----titlestatusre = %s -----\n",titlestatusre);
	if(!strcmp(osdstatusre,"Disable") && !strcmp(datetimestatusre,"Disable") && !strcmp(titlestatusre,"Disable"))
	{*/
		if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
		 perror("VIDIOC_S_INPUT");
		 
		osdset_str.timer_enable = 0;
		osdset_str.osd_enable = 0;
		memset (&(osdset_control), 0, sizeof (osdset_control));
		osdset_control.id = V4L2_CID_OSDSET;
		osdset_control.value = (int)&osdset_str;
		
		  if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &osdset_control))
			perror("VIDIOC_S_CTRL");
		  else
			printf("Changing osd of channel 0 success!\n");
		
	//}
//////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////	
	brightnessre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 1, 0, 0, "VIDEO_BRIGHT");
	printf("\n-----brightnessre = %s -----\n",brightnessre);
	if(brightnessre == NULL)
	{
			return -1;
	}
	
    if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
      perror("VIDIOC_S_INPUT");

    new_brightness_value = atoi(brightnessre);
    //validate_value(new_brightness_value);
    memset (&(brightness_control), 0, sizeof (brightness_control));
    brightness_control.id = V4L2_CID_BRIGHTNESS;
    if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &brightness_control))
      perror("VIDIOC_G_CTRL");
    else
      printf("The current brightness of channel 0 is: %d\n", brightness_control.value);
      brightness_control.id = V4L2_CID_BRIGHTNESS;
      brightness_control.value = new_brightness_value;
      if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &brightness_control))
        perror("VIDIOC_S_CTRL");
      else
        printf("Changing brightness of channel 0 success!\n");

      brightness_control.id = V4L2_CID_BRIGHTNESS;
      if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &brightness_control))
        perror("VIDIOC_G_CTRL");
      else
        printf("Now, the current brightness of channel 0 is: %d\n",
            brightness_control.value);
			
//////////////////////////////////////////////////////////////////////////////////////////////			
	contrastre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 2, 0, 0, "VIDEO_CONTRAST");
	printf("\n-----contrastre = %s -----\n",contrastre);
	if(contrastre == NULL)
	{
			return -1;
	}
    if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
      perror("VIDIOC_S_INPUT");

    new_contrast_value = atoi(contrastre);
    //validate_value(new_brightness_value);
    memset (&(contrast_control), 0, sizeof (contrast_control));
    contrast_control.id = V4L2_CID_CONTRAST;
    if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &contrast_control))
      perror("VIDIOC_G_CTRL");
    else
      printf("The current contrast of channel 0 is: %d\n", contrast_control.value);


      contrast_control.id = V4L2_CID_CONTRAST;
      contrast_control.value = new_contrast_value;
      if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &contrast_control))
        perror("VIDIOC_S_CTRL");
      else
        printf("Changing contrast of channel 0 success!\n");

     contrast_control.id = V4L2_CID_CONTRAST;
      if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &contrast_control))
        perror("VIDIOC_G_CTRL");
      else
        printf("Now, the current contrast of channel 0 is: %d\n",
            contrast_control.value);
//////////////////////////////////////////////////////////////////////////////////////////
	saturationre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 3, 0, 0, "VIDEO_SATURATION");
	printf("\n-----saturationre = %s -----\n",saturationre);
	if(saturationre == NULL)
	{
			return -1;
	}
    if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
      perror("VIDIOC_S_INPUT");

    new_saturation_value = atoi(saturationre);
    //validate_value(new_brightness_value);
    memset (&(saturation_control), 0, sizeof (saturation_control));
    saturation_control.id = V4L2_CID_SATURATION;
    if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &saturation_control))
      perror("VIDIOC_G_CTRL");
    else
      printf("The current saturation of channel 0 is: %d\n", saturation_control.value);


      saturation_control.id = V4L2_CID_SATURATION;
      saturation_control.value = new_saturation_value;
      if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &saturation_control))
        perror("VIDIOC_S_CTRL");
      else
        printf("Changing saturation of channel 0 success!\n");

     saturation_control.id = V4L2_CID_SATURATION;
      if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &saturation_control))
        perror("VIDIOC_G_CTRL");
      else
        printf("Now, the current saturation of channel 0 is: %d\n",
            saturation_control.value);
//////////////////////////////////////////////////////////////////////////////////////////
	huere = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 4, 0, 0, "VIDEO_HUE");
	printf("\n-----huere = %s -----\n",huere);
	if(huere == NULL)
	{
			return -1;
	}
    if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
      perror("VIDIOC_S_INPUT");

    new_hue_value = atoi(huere);
    //validate_value(new_brightness_value);
    memset (&(hue_control), 0, sizeof (hue_control));
    hue_control.id = V4L2_CID_HUE;
    if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &hue_control))
      perror("VIDIOC_G_CTRL");
    else
      printf("The current hue of channel 0 is: %d\n", hue_control.value);


      hue_control.id = V4L2_CID_HUE;
      hue_control.value = new_hue_value;
      if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &hue_control))
        perror("VIDIOC_S_CTRL");
      else
        printf("Changing hue of channel 0 success!\n");

     hue_control.id = V4L2_CID_HUE;
      if (-1 == ioctl(fd_video1, VIDIOC_G_CTRL, &hue_control))
        perror("VIDIOC_G_CTRL");
      else
        printf("Now, the current hue of channel 0 is: %d\n",
            hue_control.value);
//////////////////////////////////////////////////////////////////////////////////////////
	cbrre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 5, 0, 0, "CBRStatus");
	printf("\n-----cbrre = %s -----\n",cbrre);
	if(cbrre == NULL)
	{
			return -1;
	}
	if(!strcmp(cbrre,"Enable"))//cbrre != "Enable")
	{
		cbrminre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 6, 0, 0, "CBR_MIN");
		printf("\n-----cbrminre = %s -----\n",cbrminre);
		cbrmaxre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 7, 0, 0, "CBR_MAX");
		printf("\n-----cbrmaxre = %s -----\n",cbrmaxre);
		if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
		 perror("VIDIOC_S_INPUT");

		eqset_str.type = 0;
		eqset_str.minbitrate = atoi(cbrminre);
		eqset_str.maxbitrate = atoi(cbrmaxre);
		//validate_value(new_brightness_value);
		memset (&(eqset_control), 0, sizeof (eqset_control));

		  eqset_control.id = V4L2_CID_EQUALITY;
		  eqset_control.value = (int)&eqset_str;
		  if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &eqset_control))
			perror("VIDIOC_S_CTRL");
		  else
			printf("Changing cbr of channel 0 success!\n");

	
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vbrqpire = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 8, 0, 0, "VBRStatus");
	printf("\n-----vbrre = %s -----\n",vbrqpire);
	if(vbrqpire == NULL)
	{
			return -1;
	}	
	if(!strcmp(vbrqpire,"Enable"))//cbrre != "Enable")
	{
		vbrqpire = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 9, 0, 0, "QPI");
		printf("\n-----vbrqpire = %s -----\n",vbrqpire);
		vbrqpbre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 10, 0, 0, "QPB");
		printf("\n-----vbrqpbre = %s -----\n",vbrqpbre);
		vbrqppre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 11, 0, 0, "QPP");
		printf("\n-----vbrqppre = %s -----\n",vbrqppre);
		if (-1 == ioctl(fd_video1, VIDIOC_S_INPUT, &channelnum))
		 perror("VIDIOC_S_INPUT");

		eqset_str.type = 2;
		eqset_str.qpi = atoi(vbrqpire);
		eqset_str.qpb = atoi(vbrqpbre);
		eqset_str.qpp = atoi(vbrqppre);
		//validate_value(new_brightness_value);
		memset (&(eqset_control), 0, sizeof (eqset_control));
		
		  eqset_control.id = V4L2_CID_EQUALITY;
		  eqset_control.value = (int)&eqset_str;
		  if (-1 == ioctl(fd_video1, VIDIOC_S_CTRL, &eqset_control))
			perror("VIDIOC_S_CTRL");
		  else
			printf("Changing vbr of channel 0 success!\n");

	}

    return 0;
}

int Save_Webserver_Par()
{
    int channelnum = 0;
    int new_brightness_value = 0;
	int new_contrast_value = 0;
	int new_saturation_value = 0;
	int new_hue_value = 0;
    struct v4l2_control brightness_control;
	struct v4l2_control contrast_control;
	struct v4l2_control saturation_control;
	struct v4l2_control hue_control;
	struct v4l2_control eqset_control;
	struct v4l2_control osdset_control;

	unsigned char *brightnessre;
	unsigned char *contrastre;
	unsigned char *saturationre;
	unsigned char *huere;
	unsigned char *cbrre;
	unsigned char *cbrminre;
	unsigned char *cbrmaxre;
	//unsigned char *vbrre;
	unsigned char *vbrqpire;
	unsigned char *vbrqpbre;
	unsigned char *vbrqppre;
	//unsigned char *osdstatusre;
	//unsigned char *datetimestatusre;
	//unsigned char *titlestatusre;

	
	typedef struct spct6100_eqset
	{
		unsigned int type;
		unsigned int minbitrate;
		unsigned int maxbitrate;
		unsigned int qpi;
		unsigned int qpb;
		unsigned int qpp;
	}Spct6100_Eqset, *Spct6100_Eqset_Ptr;
	Spct6100_Eqset_Ptr eqset_ptr;
	Spct6100_Eqset eqset_str;
	
	typedef struct spct6100_osdset
	{
		unsigned int tran;
		unsigned int acolor;
		unsigned int vscale;
		unsigned int hscale;
		unsigned int timer_enable;
		unsigned int osd_enable;
		unsigned int osd_pos_x;
		unsigned int osd_pos_y;
		unsigned int str[16];
	}Spct6100_Osdset, *Spct6100_Osdset_Ptr;
	Spct6100_Osdset_Ptr osdset_ptr;
	Spct6100_Osdset osdset_str;
	printf("\n--- write eeprom save webpar\n");
	write_at24c02b(0,11);	
////////////////////////////////////////////////////////////////////////////////////////////////////////	
	brightnessre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 1, 0, 0, "VIDEO_BRIGHT");
	printf("\n-----brightnessre = %s -----\n",brightnessre);
	if(brightnessre == NULL)
	{
			return -1;
	}
    write_at24c02b(1, atoi(brightnessre));			
//////////////////////////////////////////////////////////////////////////////////////////////			
	contrastre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 2, 0, 0, "VIDEO_CONTRAST");
	printf("\n-----contrastre = %s -----\n",contrastre);
	if(contrastre == NULL)
	{
			return -1;
	}
    write_at24c02b(2, atoi(contrastre));
//////////////////////////////////////////////////////////////////////////////////////////
	saturationre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 3, 0, 0, "VIDEO_SATURATION");
	printf("\n-----saturationre = %s -----\n",saturationre);
	if(saturationre == NULL)
	{
			return -1;
	}
    write_at24c02b(3, atoi(saturationre));
//////////////////////////////////////////////////////////////////////////////////////////
	huere = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 4, 0, 0, "VIDEO_HUE");
	printf("\n-----huere = %s -----\n",huere);
	if(huere == NULL)
	{
			return -1;
	}
    write_at24c02b(4, atoi(huere));
//////////////////////////////////////////////////////////////////////////////////////////
	cbrre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 5, 0, 0, "CBRStatus");
	printf("\n-----cbrre = %s -----\n",cbrre);
	if(cbrre == NULL)
	{
			return -1;
	}
	if(!strcmp(cbrre,"Enable"))//cbrre != "Enable")
	{
		write_at24c02b(5,1);
		cbrminre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 6, 0, 0, "CBR_MIN");
		printf("\n-----cbrminre = %s -----\n",cbrminre);
		cbrmaxre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 7, 0, 0, "CBR_MAX");
		printf("\n-----cbrmaxre = %s -----\n",cbrmaxre);
		write_at24c02b(6, atoi(cbrminre));
		write_at24c02b(7, atoi(cbrmaxre));
	}
	else 
	{
		write_at24c02b(5,0);
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	vbrqpire = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 8, 0, 0, "VBRStatus");
	printf("\n-----vbrre = %s -----\n",vbrqpire);
	if(vbrqpire == NULL)
	{
			return -1;
	}	
	if(!strcmp(vbrqpire,"Enable"))//cbrre != "Enable")
	{
		write_at24c02b(8,1);
		vbrqpire = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 9, 0, 0, "QPI");
		printf("\n-----vbrqpire = %s -----\n",vbrqpire);
		vbrqpbre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 10, 0, 0, "QPB");
		printf("\n-----vbrqpbre = %s -----\n",vbrqpbre);
		vbrqppre = XmlRead("/tmp/spct6100/www/setup/video_global_settings.xml", 2, 11, 0, 0, "QPP");
		printf("\n-----vbrqppre = %s -----\n",vbrqppre);
		write_at24c02b(9, atoi(vbrqpire));
		write_at24c02b(10, atoi(vbrqpbre));
		write_at24c02b(11, atoi(vbrqppre));
	}
	else 
	{
		write_at24c02b(8,0);
	}

    return 0;
}
