/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_type.h
 *       Compiler:  gcc
 *
 *    Description:
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  04/09/10 16:34:29
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */



#ifndef  SPCT_TYPE_INC
#define  SPCT_TYPE_INC

#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/time.h>
#endif

#define    TRUE			1
#define    FALSE 		0

#ifndef    NULL
	#define    NULL		0
#endif

/* generl define */

#define PATH_LEN  128 /* path string len */
#define STR_LEN  64 /* general string len */
#define USR_LEN  20 /* user name string len */
#define PAS_LEN  20 /* pass string len */
#define VER_LEN  32 /* version string len */
#define MAG_LEN  32 /* magic num string len */
#define SER_LEN	 32	/* serial number len */

#define MAX_CHANNEL_NUM  64

/* data base name len */
#define DB_NAME_LEN          128

/* define module id*/
#define MID_SYSCTRL			0x1
#define MID_RD				0x2
#define MID_MOTION  		0x3
#define MID_VIDEO_LOST		0x4
#define MID_ALARMIN			0x5
#define MID_OSD		  		0x6
#define MID_MASK	  		0x7
#define MID_GRAPH	  		0x8
#define MID_CODEC	  		0x9
#define MID_PTZ		  		0xa
#define MID_NTP		  		0xb
#define MID_FTP		  		0xc
#define MID_SMTP	  		0xd
#define MID_LOG		  		0xe
#define MID_NET		  		0xf
#define MID_END 	  		0x10
#define MID_STREAM 	  		0x10
#define MID_STREAM_SESSION	0x11
#define MID_ALL				0xFFFFFFFF



/* define codec param */
//format
#define   PAL			0x1
#define   NTSC			0x2

//resolution
#define   SPCT_D1		0x1
#define   SPCT_CIF		0x2

/*
 *	alarm type
 */
#define ALARM_TYPE_MASK				0xff
#define ALARM_TYPE_VIDEO			0x1
#define ALARM_TYPE_AUDIO			0x2
#define ALARM_TYPE_MAIL				0x4
#define ALARM_TYPE_GPIO				0x8
#define ALARM_TYPE_FTP				0x10

/* define record type */

#define RD_FLAGS_ALL			0x1 // record all time
#define RD_FLAGS_MOTION			0x2 //motion event trigger
#define RD_FLAGS_ALARM       	0x4 //alarmin event trigger


/* ptz define */
#define PTZ_CMD_LEN			20
#define PTZ_MAX_PRESET		20
#define PRESET_NAME_LEN		20


#define PTZ_DRV_PELCO_D		0
#define PTZ_DRV_PELCO_P		1
#define PTZ_DRV_CUSTOM		2
#define PTZ_DRV_TRANSP		3


#define PTZ_OP_UP			0
#define PTZ_OP_DOWN			1
#define PTZ_OP_LEFT			2
#define PTZ_OP_RIGHT		3
#define PTZ_OP_ZOOM_IN		4
#define PTZ_OP_ZOOM_OUT		5
#define PTZ_OP_FOCUS_NEAR	6
#define PTZ_OP_FOCUS_FAR	7
#define PTZ_OP_IRIS_OPEN	8
#define PTZ_OP_IRIS_CLOSE	9
#define PTZ_OP_STOP			10

#define PTZ_OP_LIGHT_ON		11
#define PTZ_OP_LIGHT_OFF	12
#define PTZ_OP_GO_PRESET	13
#define PTZ_OP_SCAN_ON		16
#define PTZ_OP_SCAN_OFF		17

#define PTZ_OP_NUM			18 // max command


typedef struct cust_cmd
{
	int  size;
	char cmd[PTZ_CMD_LEN];
} Cust_Cmd;

/* serial prot define */
#define UARTMOD_RS232  0x1
#define UARTMOD_RS485  0x2
#define UARTMOD_RS422  0x3


#define SPCT_PARITY_NONE 0x1
#define SPCT_PARITY_ODD	0x2
#define SPCT_PARITY_EVEN 0x3

typedef struct serial_proty
{
	int uartmod;
	int baudrate;
	int databit;
	int stopbit;
	int paritybit;
} Serial_Proty;

/* protocol define */
#define NTP_NAME        "ntp"
#define FTP_NAME        "ftp"
#define SMTP_NAME       "smtp"

#define NTP_START	0x1
#define NTP_STOP	0x2

#define SRVC_USER_NAME_LEN              32
#define SRVC_PASSWORD_LEN               32
#define SRVC_IP_LEN                     20
#define SRVC_FILE_NAME_LEN              128
#define SRVC_FILE_PATH_LEN              128
#define SRVC_MAIL_ADDR_LEN              64
#define SRVC_MAIL_SUBJECT_LEN           128
#define SRVC_MAIL_TEXT_LEN              128

/* log search */
#define LOG_SEARCH_NUM			15
#define LOG_BUFFER_SIZE			768
#define LOG_MAX_SIZE			128

/* net define */
#define IP_LEN          20

#define IP_STATIC       0
#define IP_DYNAMIC      1

#define NET_LMT_NOW		0
#define NET_LMT_RESET	1

/*
 *-----------------------------------------------------------------------
 * time abstract
 *-----------------------------------------------------------------------
 */
typedef struct time
{
	char hour;
	char minute;
	char second;
} Time;

#define mk_sec(hour, minute, second) (((hour) * 3600) + ((minute) * 60) + (second))

/*
 *-----------------------------------------------------------------------
 * date abstract
 * if month ==  0, the day is weekday 0 is sunday 6 is saturday
 * if month == -1, the day is a day of every month
 *-----------------------------------------------------------------------
 */
typedef union spct_date
{
	int index;
	struct date
	{
		char day;
		char month;
		short year;
	}date;
} Spct_Date;

/* data define  */
#define DATA_LIVE	 0x400
#define DATA_AUDIO   0x1
#define DATA_VIDEO   0x200
#define DATA_OVER    0xFFFFFFFF
#define DATA_VIDEO_I 0x201
#define DATA_VIDEO_P 0x202
#define DATA_VIDEO_B 0x204


typedef struct spct_data
{
	int channel;
	int type;
	int flags;
	int size;
	struct timeval timestamp;
	char * data;
} Spct_Data;


/* event abstract define */
#define EVENT_NULL			0x0

typedef struct event
{
	int   id; // event id
	Time  time;	// time time of event occur
	int   channel; // the channel id
} Event;


#define event_construct(a, b, c)\
{\
	(a).id = (b);\
	(a).channel = (c);\
}


#endif   /* ----- #ifndef SPCT_TYPE_INC  ----- */
