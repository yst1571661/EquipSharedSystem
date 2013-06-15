#ifndef _TCP_CON_INC_
#define _TCP_CON_INC_

#include <pthread.h>
#include<sys/types.h>
#include<sys/stat.h>
#include<fcntl.h>

#define PORT           7000                       //监听端口
#define MAX_LINK_SOCK  1
#define RECV_BUFF_SIZE 5024
#define SEND_BUFF_SIZE 5212

#define		BASIC_VALUE		20000

#define SERVER_ERR    224              //服务器出错
#define SERVER_BUSY   225              //服务器忙
#define PACKET_ERR    226              //包解析出错
#define CMD_WAVE      0
#define CMD_QUERY     1
#define CMD_STDSET    2
#define CMD_ACK       3

#define QUERY_NONE    3                //查询未查到

#define WAVE_DATA_UNIT                  52    			//地震波形数据单元(10+21×2)=52
#define WAVE_PACK_UNIT  	(WAVE_DATA_UNIT+9)    	//地震波形数据包单元52+9
#define WAVE_BUFF_LEN 	(WAVE_DATA_UNIT*300)    	//52×100=5200

static int gheartbeat_fre;
static int gmax_nalarms;
static int gIP_change = 1;



#define BUFSIZE 8192
#define IPConfigfile_Path "/mnt/ipconfig.inf"			//IP信息配置文件默认目录，同时也是FTP默认目录
#define UserConfigfile_Path "user.inf"			//用户账号配置信息文件默认目录。同时也是FTP默认目录

#define NDEBUG  0
#define DEBUG_CONN	1
#define DEBUG_RECV	1
#define DEBUG_DATA 1
#define DEBUG_LOG	1							//1:output log	0:output screen
#define SCREEN_INFO	1						//1:output screen	important information	0:don't output information	on screen

#if  			DEBUG_LOG
#define DebugPrintf(args...)	log_error(LOG_DEBUG,##args)
#else
#define DebugPrintf	printf
#endif

#if 			SCREEN_INFO
#define	PrintScreen	printf
#else
#define	PrintScreen	/\
                                        /
#endif

typedef struct connectComm
{
        unsigned char dsp_data[WAVE_BUFF_LEN];      //波形发送缓冲区
        unsigned int dsp_num;                      		 //波形发送缓冲区数据大小
        pthread_mutex_t dsp_lock;
}stuConnComm;
stuConnComm sttDspRoute;

unsigned char sys_Time[15];
int beginsyncbmp;
int startsyncbmp;
int beginsendbmp;
char syncbeginFname[50];
char syncendFname[50];
int freqsendbmp;
int beginsendcard;
int beginsenddevice;
int beginsendsnrnum;
int time_change;
int beginsendtersta;
int beginsavewebpar;
int beginupload;


int trans_user;
int catchonemotion;
int		action_fd;
void IsSetaction(int action_fd);


//int obtain_card;
/****************/
extern pthread_mutex_t ordertime_lock;
extern pthread_mutex_t cardfile_lock;
int arrive_card;
int arrive_flag;
unsigned char card_to_check[50];
unsigned char card_record_check[50];
/****************/
int card_tlimit;
int update_user_xml;            //更新用户数据
int updata_ordertime_xml;
int reboot_flag;
unsigned long user_version;    //用户数据库版本号
unsigned long software_version;	//程序版本号
int user_count;
int led_state;
int device_mode;				//标志设备是开放还是半开放模式 0x01 开放模式	0x00 半开放模式
/*视频采集参数*/

int catch_mode;   // 0x01 动态检测  0x00 定时检测

int catch_freq;   //时间间隔  单位:秒

int catch_sen;    //采集灵敏度 0-4

extern void set_cation(int, int);

extern int action_fd;

/* 故障检测信息 */
struct err_check {
        unsigned char begincheck;
        unsigned char flash;
        unsigned char eeprom;
        unsigned char rtc;
        unsigned char card_checked;
        unsigned char card;
        unsigned char photo_checked;
        unsigned char photo;

        unsigned char year_high;
        unsigned char yead_low;
        unsigned char month;
        unsigned char day;
        unsigned char hour;
        unsigned char min;
        unsigned char sec;
        unsigned char issavvideo;
    //time_t		  time_now;
    struct tm time_now;
} Err_Check;
//int mode_reset;
//////////////

char snrnum[20];

int Led_delay;
int Led_on;
int Led_off;

unsigned int CRC_check(char bs[],int off,int len);

int net_configure(void) ;

int _ConnLoop();

 void FreeMemForEx();

 void* WavePacketSend(void *arg);

void* CardPacketSend(void *arg);

typedef void* ptexec(void *arg);
 int WorkThreadCreate(ptexec threadexec, int prio);

 void BmpFileSend(char * bmpfilename);

#endif

