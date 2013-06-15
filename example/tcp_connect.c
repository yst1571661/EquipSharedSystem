#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <errno.h>
#include <signal.h>
#include <fcntl.h>

#include <unistd.h>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <net/if_arp.h>
#include <asm/types.h>
#include <netinet/ether.h>
#include <linux/netlink.h>
#include <linux/rtnetlink.h>
#include "tcp_connect.h"
#include <dirent.h>
#include "xml.h"
#include "db.h"
#include "log.h"
fd_set rdEvents, exEvents;
extern unsigned char BCD_decode_tab[];
typedef struct connectSock
{
        int fdSock;                                   //socket描述符
        int loginLegal;                               //登录合法
        pthread_t isPid;                              //线程id
        struct in_addr clientAddr;                    //客户端IP
        unsigned long remainPos;                      //上次通信未处理不完整包字节数
        unsigned long curReadth;                      //接收缓冲区当前未处理字节数
        unsigned char *packBuffIn;                    //用户接收缓冲区
        unsigned char *packBuffOut;                   //用户发送缓冲区
        pthread_mutex_t lockBuffIn;                   //互斥锁 保护当前socket用户接收缓冲区
        pthread_mutex_t lockBuffOut;                  //互斥锁 保护当前socket用户发送缓冲区

        int noProbes;                                 //心搏函数未检测次数
        //size_t sizeIn;
        //size_t sizeOut;
}stuConnSock;
stuConnSock sttConnSock[MAX_LINK_SOCK];           //最大允许客户端并发连接数

struct sockaddr_in gserver_addr;
struct sockaddr_in gclient_addr;

char ipdz[16]="192.168.1.7";//"172.20.57.30";
char zwym[16]="255.255.254.0";
char mrwg[16]="172.20.56.1";


//the following table correspongding to the results of CRC check of 0x00-0xff
int CRC_table[] = { 0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50a5,
                0x60c6, 0x70e7, 0x8108, 0x9129, 0xa14a, 0xb16b, 0xc18c, 0xd1ad,
                0xe1ce, 0xf1ef, 0x1231, 0x0210, 0x3273, 0x2252, 0x52b5, 0x4294,
                0x72f7, 0x62d6, 0x9339, 0x8318, 0xb37b, 0xa35a, 0xd3bd, 0xc39c,
                0xf3ff, 0xe3de, 0x2462, 0x3443, 0x0420, 0x1401, 0x64e6, 0x74c7,
                0x44a4, 0x5485, 0xa56a, 0xb54b, 0x8528, 0x9509, 0xe5ee, 0xf5cf,
                0xc5ac, 0xd58d, 0x3653, 0x2672, 0x1611, 0x0630, 0x76d7, 0x66f6,
                0x5695, 0x46b4, 0xb75b, 0xa77a, 0x9719, 0x8738, 0xf7df, 0xe7fe,
                0xd79d, 0xc7bc, 0x48c4, 0x58e5, 0x6886, 0x78a7, 0x0840, 0x1861,
                0x2802, 0x3823, 0xc9cc, 0xd9ed, 0xe98e, 0xf9af, 0x8948, 0x9969,
                0xa90a, 0xb92b, 0x5af5, 0x4ad4, 0x7ab7, 0x6a96, 0x1a71, 0x0a50,
                0x3a33, 0x2a12, 0xdbfd, 0xcbdc, 0xfbbf, 0xeb9e, 0x9b79, 0x8b58,
                0xbb3b, 0xab1a, 0x6ca6, 0x7c87, 0x4ce4, 0x5cc5, 0x2c22, 0x3c03,
                0x0c60, 0x1c41, 0xedae, 0xfd8f, 0xcdec, 0xddcd, 0xad2a, 0xbd0b,
                0x8d68, 0x9d49, 0x7e97, 0x6eb6, 0x5ed5, 0x4ef4, 0x3e13, 0x2e32,
                0x1e51, 0x0e70, 0xff9f, 0xefbe, 0xdfdd, 0xcffc, 0xbf1b, 0xaf3a,
                0x9f59, 0x8f78, 0x9188, 0x81a9, 0xb1ca, 0xa1eb, 0xd10c, 0xc12d,
                0xf14e, 0xe16f, 0x1080, 0x00a1, 0x30c2, 0x20e3, 0x5004, 0x4025,
                0x7046, 0x6067, 0x83b9, 0x9398, 0xa3fb, 0xb3da, 0xc33d, 0xd31c,
                0xe37f, 0xf35e, 0x02b1, 0x1290, 0x22f3, 0x32d2, 0x4235, 0x5214,
                0x6277, 0x7256, 0xb5ea, 0xa5cb, 0x95a8, 0x8589, 0xf56e, 0xe54f,
                0xd52c, 0xc50d, 0x34e2, 0x24c3, 0x14a0, 0x0481, 0x7466, 0x6447,
                0x5424, 0x4405, 0xa7db, 0xb7fa, 0x8799, 0x97b8, 0xe75f, 0xf77e,
                0xc71d, 0xd73c, 0x26d3, 0x36f2, 0x0691, 0x16b0, 0x6657, 0x7676,
                0x4615, 0x5634, 0xd94c, 0xc96d, 0xf90e, 0xe92f, 0x99c8, 0x89e9,
                0xb98a, 0xa9ab, 0x5844, 0x4865, 0x7806, 0x6827, 0x18c0, 0x08e1,
                0x3882, 0x28a3, 0xcb7d, 0xdb5c, 0xeb3f, 0xfb1e, 0x8bf9, 0x9bd8,
                0xabbb, 0xbb9a, 0x4a75, 0x5a54, 0x6a37, 0x7a16, 0x0af1, 0x1ad0,
                0x2ab3, 0x3a92, 0xfd2e, 0xed0f, 0xdd6c, 0xcd4d, 0xbdaa, 0xad8b,
                0x9de8, 0x8dc9, 0x7c26, 0x6c07, 0x5c64, 0x4c45, 0x3ca2, 0x2c83,
                0x1ce0, 0x0cc1, 0xef1f, 0xff3e, 0xcf5d, 0xdf7c, 0xaf9b, 0xbfba,
                0x8fd9, 0x9ff8, 0x6e17, 0x7e36, 0x4e55, 0x5e74, 0x2e93, 0x3eb2,
                0x0ed1, 0x1ef0 };

struct route_info
{
        u_int dstAddr;
        u_int srcAddr;
        u_int gateWay;
        char ifName[IF_NAMESIZE];
};

int lederrcount;
int ledtwinklebegin;
int sockreleasebegin;


static int card_ack;
static char card_to_send[50];
static int begin_query;
static pthread_mutex_t cardquery_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cardquery_cond = PTHREAD_COND_INITIALIZER;

static void maketimeout(struct timespec *tsp, long seconds);

static pthread_mutex_t cardsnd_lock = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cardsnd_cond = PTHREAD_COND_INITIALIZER;

static pthread_cond_t cardsend_cond = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t cardsend_lock = PTHREAD_MUTEX_INITIALIZER;

pthread_mutex_t cardfile_lock = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t ordertime_lock = PTHREAD_MUTEX_INITIALIZER;

static GDBM_FILE gdbm_card;
static GDBM_FILE gdbm_ordertime;
static GDBM_FILE gdbm_user;
static GDBM_FILE gdbm_device;

void ReadSysTime(void)
{
        time_t now;
        struct tm  *timenow;
    //int re;

        time(&now);
        timenow = localtime(&now);

        strftime(sys_Time,sizeof(sys_Time),"%Y%m%d%H%M%S",timenow);
}

 void FreeMemForEx()
{
        int i;
        for (i=0; i<MAX_LINK_SOCK; ++i)
        {
                if (sttConnSock[i].fdSock > 0)
                {
                        #if DEBUG_DATA
                        DebugPrintf("\n----FreeMemForEx--resource[%d] delete", i);
                        #endif

                        close(sttConnSock[i].fdSock);
                        if (sttConnSock[i].isPid > 0)
                        {
                                pthread_cancel(sttConnSock[i].isPid);
                        }

                        pthread_mutex_destroy(&sttConnSock[i].lockBuffIn);
                        pthread_mutex_destroy(&sttConnSock[i].lockBuffOut);

                        free(sttConnSock[i].packBuffIn);
                        free(sttConnSock[i].packBuffOut);
                sttConnSock[i].packBuffIn = NULL;
            sttConnSock[i].packBuffOut = NULL;
                }
        }
        pthread_mutex_destroy(&sttDspRoute.dsp_lock);
        trans_user = 0;
}

static void SetNonBlock(int fdListen)
{
        int opts;
        if ((opts = fcntl(fdListen, F_GETFL)) < 0)
        {
                perror("\nfcntl(F_GETFL) error");
                exit(1);
        }
        opts |= O_NONBLOCK;
        if (fcntl(fdListen, F_SETFL, opts) < 0)
        {
                perror("\nfcntl(F_SETFL) error");
                exit(1);
        }
}

static int SockServerInit()
{
        int fdListen, val = 1;
        //int fdListen, val = 100;
        if ((fdListen = socket(AF_INET, SOCK_STREAM, 0)) == -1)
        {
                perror("\nsocket error");
                return -1;
        }

        bzero(&gserver_addr, sizeof(gserver_addr));
        gserver_addr.sin_family = AF_INET;
        gserver_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        gserver_addr.sin_port = htons(PORT);

        setsockopt(fdListen, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int));
        //setsockopt把socket属性改成SO_REUSEADDR
        //DebugPrintf("\n-----=*=-----Current HOST IP: %s\n\n", inet_ntoa(gserver_addr.sin_addr));
        setsockopt(fdListen, SOL_SOCKET, SO_RCVLOWAT, &val, sizeof(int));
        setsockopt(fdListen, SOL_SOCKET, SO_SNDLOWAT, &val, sizeof(int));
        /*int nSendBuf = 16 * 1024;
        setsockopt(fdListen, SOL_SOCKET, SO_SNDBUF, &nSendBuf, sizeof( int ) );
        int nZero = 0;
        setsockopt(fdListen, SOL_SOCKET, SO_SNDBUF, ( char * )&nZero, sizeof( nZero ) );
        int nNetTimeout = 1000; // 1秒
        setsockopt( socket, SOL_SOCKET, SO_SNDTIMEO, ( char * )&nNetTimeout, sizeof( int ) );*/

        SetNonBlock(fdListen);

        if (bind(fdListen, (struct sockaddr*)(&gserver_addr), sizeof(gserver_addr)) == -1)
        {
                perror("\nbind error");
                return -1;
        }
        if (listen(fdListen, MAX_LINK_SOCK+1) == -1)
        {
                perror("\nlisten error");
                return -1;
        }

        return fdListen;
}

static void sigAlarm(int signo)
{
        int i, j;
        static int stime=0, ustime=0;
        for (i=0; i<MAX_LINK_SOCK; i++)
        {
                if (sttConnSock[i].fdSock > 0)
                {
#ifdef DEBUG
                        DebugPrintf("\n----sigAlarm");
#endif
                        ++sttConnSock[i].noProbes;
                }
        }
        alarm(gheartbeat_fre);
        return;
}

static int SockPackSend(unsigned char cmdWord, int fdConn, stuConnSock *sttParm, const unsigned char *sendData, size_t dataLen)
{
        size_t sentLen = 0;
        unsigned long packLen;
        int curWrite;
        unsigned char *tagAddr, msgNote[16];
        int i;
        //FILE *output = NULL;

        if (fdConn == 0)    //检查socket是否还存在
        {
#if DEBUG_DATA
              if (cmdWord != CMD_WAVE)
              {
                                DebugPrintf("\n----SockPackSend--sock has been release!----");
                                fflush(stdout);
              }
#endif
                return 0;
        }

        if (cmdWord >= 224)
        {
                tagAddr = msgNote;
        }
        else
        {
            if(sttParm->packBuffOut==NULL) return 0;
                tagAddr = sttParm->packBuffOut;
                memcpy(tagAddr+7, sendData, dataLen);      //有效数据
        }

        packLen = dataLen+7;                           //数据包长度
        tagAddr[0] = 0x1B;                             //包头
        tagAddr[1] = 0x10;
        tagAddr[2] = 0x1B;
        tagAddr[3] = 0x10;
        tagAddr[4] = cmdWord;                          //命令字
        tagAddr[5] = (packLen>>8) & 0xff;              //数据包长度2字节表示
        tagAddr[6] = packLen & 0xff;


#if NDEBUG
    do {
        if (packLen > 100)
            break;
        for (i = 0; i < packLen; i++)
            DebugPrintf("\n-----tagAddr[%d] = %x---", i, tagAddr[i]);
    } while(0);
#endif

        while (sentLen < packLen)
        {
                do
                {
                        curWrite = send(fdConn, tagAddr, packLen-sentLen, 0);
                        usleep(10000);
                        if((curWrite<0) && (errno==EAGAIN))
                        {
                                perror("\n-----send error-----");
                                for(i=0; i<10; i++)
                                {
                                        curWrite = send(fdConn, tagAddr, packLen-sentLen, 0);
                                        usleep(10000);
                                        if(curWrite > 0)
                                        {
                                                break;
                                        }
                                }
                        }

                } while (((curWrite<0) && (errno==EINTR)));// || ((curWrite<0) && (errno==EAGAIN) && (sttParm->fdSock >0 ) && (sttParm->loginLegal >0)));
                if (curWrite <= 0)
                {
                        return -1;
                }
                sentLen += curWrite;
                tagAddr += curWrite;
        }
        return 0;
}


static void HeartbeatInit(int frequency, int alarmcnts)
{
        if ((gheartbeat_fre = frequency) < 1)
                gheartbeat_fre = 1;
        if ((gmax_nalarms=alarmcnts) < 1)
                gmax_nalarms = frequency;
        signal(SIGALRM, sigAlarm);
        alarm(gheartbeat_fre);

        //屏蔽SIGPIPE信号
        //signal(SIGPIPE, sigPipe);
        struct sigaction sa;
        sa.sa_handler = SIG_IGN;
        sigaction(SIGPIPE, &sa, 0);			//收到SIGPIPE信号则去执行SIG_IGN函数
}

static int HandleNewConn(int fdListen)
{
        int fdConn, i, clientLen;

#ifdef DEBUG
        DebugPrintf("\n----HandleNewConn--start to handle new socket!");
#endif

        clientLen = sizeof(struct sockaddr_in);
        if ((fdConn = accept(fdListen, (struct sockaddr*)&gclient_addr, &clientLen)) == -1)		//成功则返回新创建的套接字，失败返回-1
        {
                perror("\naccept error");
                DebugPrintf("\n---accept err happen here  注意出现accept错误！！----------");
                gIP_change = 1;
                return 0;
        }

#ifdef DEBUG
        DebugPrintf("\n----HandleNewConn--allocate memory buffer for candidate Connection accepted socket: FD = %d", fdConn);
#endif

        for(i=0; i<MAX_LINK_SOCK; i++)
        {
#ifdef DEBUG
                DebugPrintf("\n----HandleNewConn--check if candidate Connection can be accepted sockfd = %d", sttConnSock[i].fdSock);
#endif
                if (sttConnSock[i].fdSock == 0)
                {
#ifdef DEBUG
                        DebugPrintf("\n-----=*=-----Well allocateed:Connection accepted: FD = %d; Slot = %d", fdConn, i);
#endif

                        if ((pthread_mutex_init(&sttConnSock[i].lockBuffIn, NULL)!=0) || (pthread_mutex_init(&sttConnSock[i].lockBuffOut, NULL)!=0))
                        {
                                perror("\nmutex lock failed!");
                                SockPackSend((unsigned char)SERVER_ERR, fdConn, NULL, NULL, 0);    //服务器出错
                                close(fdConn);
                                return 0;
                        }
                        SetNonBlock(fdConn);
                        sttConnSock[i].fdSock = fdConn;
                        sttConnSock[i].loginLegal = 1;				//登录合法
                        sttConnSock[i].clientAddr = gclient_addr.sin_addr;
                        sttConnSock[i].remainPos = 0;
                        sttConnSock[i].curReadth = 0;
                        sttConnSock[i].packBuffIn = (unsigned char *)malloc(sizeof(char)*RECV_BUFF_SIZE);
                        sttConnSock[i].packBuffOut = (unsigned char *)malloc(sizeof(char)*SEND_BUFF_SIZE);
                        sttConnSock[i].isPid = 0;
                        sttConnSock[i].noProbes = 0;

#ifdef DEBUG
                        DebugPrintf("\n-----=*=-----Connected client IP: %s", inet_ntoa(sttConnSock[i].clientAddr));
#endif
                        return fdConn;
                }
        }

#ifdef DEBUG
        DebugPrintf("\n----HandleNewConn--sorry no room left for new client");
        fflush(stdout);
#endif

        SockPackSend((unsigned char)SERVER_BUSY, fdConn, NULL, NULL, 0);     //服务器忙
        close(fdConn);
        return 0;
}

 void Get_Netaddr(char *netaddr,unsigned char *temp_addr)
   {
        int b [16];
        int Count=0;
        int Pointdex[3];
        int temp=-1;
        int i;
        for( i=0;i<16;++i)
        {
                if(netaddr[i]=='.')
                {
                        b[i]=-1;
                        Count++;
                }
                else if(netaddr[i]=='\0')
                {
                        b[i]=-1;
                        Count++;
                        i=16;
                }
                else
                {
                        b[i]=netaddr[i]-'0';
                        Count++;
                }
        }

        int j=-1;
        int Count1=0;
        for(i=0;i!=Count;++i)
        {
                if(b[i]==-1)
                {
                        int num=i-j-1;
                        switch(num)
                        {
                                case 1:
                                        temp_addr[Count1]=b[i-1];
                                        Count1 ++;
                                        break;
                                case 2:
                                    temp_addr[Count1]=b[i-1]+b[i-2]*10;
                                        Count1 ++;
                                        break;
                                case 3:
                                        temp_addr[Count1]=b[i-1]+b[i-2]*10+b[i-3]*100;
                                        Count1 ++;
                                        break;
                        }
                        j=i;
                }
        }
}

extern struct err_check Err_Check;

char *query_Para(const char *dataBuffer,int dataLenth,unsigned int *length)
{
        int re = 1;
        char *ansData = NULL;
        *length = 0;
        int len = 0;
        int i = 0;

#if DEBUG_CONN
    DebugPrintf("\n----------------dataBuffer[0] = %d---------", dataBuffer[0]);
    PrintScreen("\n---dataBuffer[0] = %d---",dataBuffer[0]);
#endif
        switch(dataBuffer[0])                             	//查询字
        {

        /********************************用户名密码校验 **************************************************************/
        case 0:
                DebugPrintf("\n----- query snrnum -----");
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x00;
                beginsendsnrnum = 1;
                return ansData;

                break;

        /*********************************查询台站参数*************************************************************/
        case 1:    //card information
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x01;
                //beginsendcard = 1;
                return ansData;
                break;

        /**********************************************************************************************/
        case 2:          //terminal states                        				 //查询仪器响应参数
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x02;
                beginsendtersta = 0;
                return ansData;
                break;

        /***************************************查询采集参数*******************************************************/
        case 3:        //device information
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x03;
                beginsenddevice = 1;
                return ansData;
                break;
        /************************************查询GPS信息**********************************************************/
        case 4:
                *length=13;
                ansData = malloc(13);
                //re = Get_GPS_Data(temp_gpsdata);
                if(re == 0)
                {
                        ansData[0]  =0x00;
                        ansData[1] = 0x01;
                        ansData[2]  =0x04;
                }
                else
                {
                        ansData[0]  =0xff;
                        ansData[1] = 0x01;
                        ansData[2]  =0x04;
                        ansData[3] = 0;
                        ansData[4] = 0;
                        ansData[5] = 0;
                        ansData[6] = 0;
                        ansData[7] = 0;
                        ansData[8] = 0;
                        ansData[9] = 0;
                        ansData[10] = 0;
                        ansData[11] = 0;
                        ansData[12] = 0;
                }
                return ansData;
                break;
        /**********************************************************************************************/
        case 5:                //collect image       采集使能           					 //查询标定参数
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x05;
                if(dataBuffer[1] == 0x00)
                {
                        //freqsendbmp = dataBuffer[2];
                        #if NDEBUG
                                DebugPrintf("\n-------------beginsendbmp = 1--------------");
                        #endif
                        beginsendbmp = 1;
                }
                else if(dataBuffer[1] == 0xFF)
                {
                        #if NDEBUG
                                DebugPrintf("\n-------------beginsendbmp = 0--------------");
                        #endif
                        beginsendbmp = 0;
                }
                return ansData;
                break;
        /**********************************************************************************************/
        case 6:               //sync image                    					//查询触发参数
                *length=0;
                ansData = malloc(3);
                ansData[0]  =0x00;
                ansData[1] = 0x01;
                ansData[2]  =0x06;
                if(dataBuffer[1] == 0x00)
                {
                    sprintf(syncbeginFname, "%02d%02d%02d%02d%02d%02d00.jpg", dataBuffer[2],dataBuffer[3],dataBuffer[4],dataBuffer[5],dataBuffer[6],dataBuffer[7]);
                    sprintf(syncendFname, "%02d%02d%02d%02d%02d%02d00.jpg", dataBuffer[8], dataBuffer[9],dataBuffer[10],dataBuffer[11],dataBuffer[12],dataBuffer[13]);
#if DEBUG_CONN
                    DebugPrintf("\n-- query case 6 received-syncbeginFname = %s  syncendFname = %s---", syncbeginFname, syncendFname);
#endif
                    beginsyncbmp = 1;
                }
                else if(dataBuffer[1] == 0xFF)
                {
                    beginsyncbmp = 0;
                }
                return ansData;
                break;
        /**********************************************************************************************/
        case 7: 			//采集模式  参数查询						//查询环境参数，具体为查询当前的电源电压和环境温度
                if (catch_mode)
                {
                        *length = 7;
                        ansData = malloc(7);
                        ansData[0] = 0x00;
                        ansData[1] = 0x01;
                        ansData[2] = 0x07;
                        ansData[3] = catch_mode;
                        ansData[4] = catch_sen;
                        ansData[5] = (catch_freq >> 8) & 0xFF;
                        ansData[6] = catch_freq & 0xFF;

                }
                else
                {
                        *length = 6;
                        ansData = malloc(6);
                        ansData[0] = 0x00;
                        ansData[1] = 0x01;
                        ansData[2] = 0x07;
                        ansData[3] = catch_mode;
                        //ansData[4] = catch_sen;
                        ansData[4] = (catch_freq >> 8) & 0xFF;
                        ansData[5] = catch_freq & 0xFF;
                }



                return ansData;
                break;
        /**********************************************************************************************/
        case 8:
            if (dataLenth < 3) {
                        *length=4;
                        ansData=malloc(4);
                        //re=userAls(dataBuffer+1,dataLenth-1);

                        if(re>=0)
                        {
                                ansData[0] = 0x00;
                                ansData[1] = 0x01;
                                ansData[2] = 0x08;
                                ansData[3] = 0x01;                     		//1超级用户，0普通用户
                                //DebugPrintf("\n---log in by super user----\n");
                        }
                        else
                        {
                                ansData[0] = 0xff;				//应答状态
                                ansData[1] = 0x01;				//命令字，01表示为查询，02为设置
                                ansData[2] = 0x08;				//查询子，即对应的是哪条查询命令
                                ansData[3] = QUERY_NONE;              //查询未找到
                        }

                        return ansData;
                }

                *length = 0;
                ansData=malloc(3);
                ansData[0] = 0x00;
                ansData[1] = 0x01;
                ansData[2] = 0x08;

        Err_Check.year_high = dataBuffer[1];
        Err_Check.yead_low = dataBuffer[2];
        Err_Check.month = dataBuffer[3];
        Err_Check.day = dataBuffer[4];
        Err_Check.hour = dataBuffer[5];
        Err_Check.min = dataBuffer[6];
        Err_Check.sec = dataBuffer[7];
        getstime(&(Err_Check.time_now));
                Err_Check.begincheck = 1;
                return ansData;
                break;

    /*************************************用户端版本号*********************************************************/
    case 9:
                if (trans_user)
                        *length = 0;
        else
                        *length=7;

                DebugPrintf("\n-------recv query command----");
        ansData = malloc(7);
        ansData[0] = 0x00;
        ansData[1] = 0x01;
        ansData[2] = 0x09;
        ansData[3] = (user_version >> 24) & 0xFF;			//高字节在前，低字节在后
        ansData[4] = (user_version >> 16) & 0xFF;
        ansData[5] = (user_version >> 8) & 0xFF;
        ansData[6] = (user_version) & 0xFF;
                return ansData;
                break;
    /*************************************程序版本号*********************************************************/
        case 0x11:

                *length = 7;
        ansData = malloc(7);
        ansData[0] = 0x00;
        ansData[1] = 0x01;
        ansData[2] = 0x11;
        ansData[3] = read_at24c02b(85);
        ansData[4] = read_at24c02b(86);
        ansData[5] = read_at24c02b(87);
        ansData[6] = read_at24c02b(88);
 #if DEBUG_CONN
                DebugPrintf("\nsearch for software version!!!!");
        software_version = (read_at24c02b(85) << 24)& 0xFF000000;
        software_version +=  (read_at24c02b(86) << 16)& 0xFF0000;
        software_version +=  (read_at24c02b(87) << 8)& 0xFF00;
        software_version +=  (read_at24c02b(88))& 0xFF;
        DebugPrintf("\nSoftware Version = %d",software_version);
 #endif
                return ansData;
                break;

        case 0xa:                                				 //查询主动传送
                *length = 0;
                ansData = malloc(3);
                ansData[0] = 0x00;
        ansData[1] = 0x01;
        ansData[2] = 0x10;
        //obtain_card = dataBuffer[1];
                return ansData;
                break;
        case 0xb:                                 				//查询串口
                break;
        case 0xc:                                 				//查询U盘挂载状态
            *length=4;
                ansData=malloc(4);
                //if(mount_re==0)
                //{DebugPrintf("\n-----------U盘在设备上-------------\n");
                        ansData[0] = 0x00;
                        ansData[1] = 0x01;
            ansData[2] = 0x0c;
            ansData[3] = 0xff;
                /*}
                else
                {DebugPrintf("\n-----------U盘不在设备上-----------\n");
                        ansData[0] = 0x00;
                        ansData[1] = 0x01;
                        ansData[2] = 0x0c;
                        ansData[3] = 0x00;
                }*/
            return ansData;
                break;
        case 0xd:                                 				//查询记录参数
                break;
        case 0xe:                                 				//查询数据备份
                break;
        case 0xf:                                				 //查询系统时区
                break;
    case 0x10:                               				 //查询物理卡卡号
        *length = 3;
        ansData = malloc(3);
        begin_query = 1;									//查询物理卡号标志位置1
        ansData[0] = 0x00;
        ansData[1] = 0x01;
        ansData[2] = 0x10;
        return ansData;
                break;
        default:
                break;
    }
    return ansData;
}


int IpAls(const char *tmp,int length)
{
        int i,j;
        strcpy(ipdz, tmp);
        i = strlen(ipdz);
        ipdz[i]='\0';
        DebugPrintf("\nipdz=%s",ipdz);
        fflush(stdout);
        strcpy(zwym, tmp+i+1);
        j= strlen(zwym);
        zwym[j]='\0';
        DebugPrintf("\nzwym=%s",zwym);
        fflush(stdout);
        strncpy(mrwg, tmp+i+j+2, length-i-j-2);
        mrwg[length-i-j-2]='\0';
        DebugPrintf("\nmrwg=%s",mrwg);
        fflush(stdout);
        reconfig();
        return(net_configure());
}
static GDBM_FILE gdbm_usrbak = NULL;
static GDBM_FILE gdbm_ordertimebak = NULL;

static int count_users;

#if DEBUG_CONN
        static int file_fp;
#endif

static int strncmp0(const char *s1, const char *s2, int n)
{
        int i;

        for (i = 0; i < n; i++) {
                if (s1[i] != s2[i])
                        return -1;
        }
        return 0;
}

char *set_Para(const char *dataBuffer,int dataLenth,unsigned int *length)
{
        int re = 0;
        char *ansData;

        *length = 0;
        char TempDatabuf[80] = {0};
    char TempBuffer[80] = {0};
    //char TempData[40] = {0};
        int temp = 0;
        int i = 0;
    datum data;
    datum key;
    static int software_seq=0,software_seqtmp=0;		//software_seqtmp stores the sequency of this record,software_seq stores the sequency of last record
    static int byte_count=0;
    static unsigned int byte_all=0;

   // char data_buffer[50];
   // char key_buffer[50];
#if NDEBUG
    DebugPrintf("\n--------------receive set command databuffer[0] = %d------", dataBuffer[0]);
    PrintScreen("\n--------------receive set command databuffer[0] = %d------", dataBuffer[0]);
#endif
        switch(dataBuffer[0])                              	 //设置字
        {
        /***************************************设置检测仪的ip地址和默认网关*****************************/
        case 0x0b:
                *length=4;                                     		//设置条件为ip地址和默认网关中间以0x00隔开
                ansData = malloc(4);
                re = IpAls(dataBuffer+1,dataLenth-1);
                if(re==0)
                {
                        ansData[0]=0x00;
                        ansData[1]=0x02;
                        ansData[2]=0x0b;
                        ansData[3]=re;                     		//ip 设置成功 返回有效数据0
                }
                else
                {
                        ansData[0]=0xff;
                        ansData[1]=0x02;
                        ansData[2]=0x0b;
                        ansData[3]=SERVER_ERR;              //设置错误
                }
                break;

        /***************************************设置终端时间***********************************************/
        case 0x00:      //set time
        *length = 3;
                ansData=malloc(3);
                memcpy(TempDatabuf,dataBuffer+1,7);
                DebugPrintf("\n-----set time %02d%02d%02d%02d%02d%02d.%02d-----", TempDatabuf[0],TempDatabuf[1],TempDatabuf[2],TempDatabuf[3],TempDatabuf[4],TempDatabuf[5],TempDatabuf[6]);
                re = Set_time(TempDatabuf);
                time_change = 1;
                if(re==0)
                {
                        ansData[0]=0x00;
                        ansData[1]=0x02;
                        ansData[2]=0x00;
                }
                else
                {
                        ansData[0]=0xff;
                        ansData[1]=0x02;
                        ansData[2]=0x00;
                }
                break;

        /*************************************** 设置台站参数********************************************/
        case 0x01:
                *length=4;
                ansData=malloc(4);
                memcpy(TempDatabuf,dataBuffer+1,dataLenth-1);
                //re =Set_Device_Info(TempDatabuf,dataLenth-1);
                if (re == 0)
                {
                        ansData[0]=0x00;
                        ansData[1]=0x02;
                        ansData[2]=0x01;
                        ansData[3]=0x01;
                }
                else
                {
                        ansData[0]=0xff;
                        ansData[1]=0x02;
                        ansData[2]=0x01;
                        ansData[3]=SERVER_ERR;
                }
                break;

        case 0x02:     //reboot                          			//设置地震计参数
                *length = 3;
                ansData=malloc(3);
                if(re==0)
                {
                        ansData[0]=0x00;
                        ansData[1]=0x02;
                        ansData[2]=0x02;
                }
                else
                {
                        ansData[0]=0xff;
                        ansData[1]=0x02;
                        ansData[2]=0x02;
                }
                break;

        /*************************************** 设置采集参数**********************************************/
        case 0x03:                        //控制电源状态
                *length = 0;
                ansData=malloc(3);
                ansData[0]=0x00;
                ansData[1]=0x02;
                ansData[2]=0x03;

                if (dataLenth < strlen(card_record_check) + 1) {
                        DebugPrintf("\n----set case 3 datalength err-----");
                        break;
                }

        DebugPrintf("\n---------dataBuffer = %s------", dataBuffer + 1);
 #if   DEBUG_RECV
        DebugPrintf("\n---set case 3---");
#endif
        if (!strncmp0(dataBuffer + 1, card_record_check, strlen(card_record_check))) {
            DebugPrintf("\n------receive cardsend_cond reply-----");
            if (pthread_cond_broadcast(&cardsend_cond) != 0)
                DebugPrintf("\n---pthread_cond_broadcast cardsend_cond err----------");
        }
                break;

        case 0x04:                               //设置标定参数
                if (dataBuffer[1])    //动态检测
                {
                        *length = 3;
                        ansData = malloc(3);

                        catch_mode = dataBuffer[1];
                        catch_sen = dataBuffer[2];
                        catch_freq = ((dataBuffer[3] << 8) & 0xFF00) + (dataBuffer[4] & 0xFF);

                        write_at24c02b(226, catch_mode);
                        write_at24c02b(227, catch_sen);
                        write_at24c02b(228, dataBuffer[3]);
                        write_at24c02b(229, dataBuffer[4]);
                        //mode_reset = 1;			//改变模式
#if DEBUG_CONN
            DebugPrintf("\n=======motion detect mode begin---------");
#endif

                        ansData[0] = 0x00;
                        ansData[1] = 0x02;
                        ansData[2] = 0x04;
                }
                else                 //定时采样
                {
                        *length = 3;
                        ansData = malloc(3);
#if DEBUG_CONN
            DebugPrintf("\n---time mode begin----");
#endif
                        catch_mode = dataBuffer[1];
                        catch_freq = ((dataBuffer[2] << 8) & 0xFF00) + (dataBuffer[3] & 0xFF);
                        //mode_reset = 1;			//改变模式
                        write_at24c02b(226, catch_mode);
                        write_at24c02b(228, dataBuffer[2]);
                        write_at24c02b(229, dataBuffer[3]);

                        ansData[0] = 0x00;
                        ansData[1] = 0x02;
                        ansData[2] = 0x04;
                }
                break;
    case 0x05:                               //存储器初始化
                        *length = 3;
                        ansData = malloc(3);
            system("rm -rf /mnt/work/*");
            system("rm -rf /mnt/safe/*");
                        system("rm -rf /tmp/*.jpg");
            system("mkdir /mnt/work");
            system("mkdir /mnt/safe");
                        ansData[0] = 0x00;
                        ansData[1] = 0x02;
                        ansData[2] = 0x05;
                break;
    case 0x06:                               //清空用户数据

                        //break;
            DebugPrintf("\n------------set case 0x06-----");
            PrintScreen("\n------------set case 0x06-----");

                        count_users = 0;
                        trans_user = 1;
            *length = 3;
            ansData = malloc(3);
            ansData[0] = 0x00;
            ansData[1] = 0x02;
            ansData[2] = 0x06;
            user_version = 0;
            user_count = 0;

            db_close(gdbm_usrbak);
            DelFile("/tmp/user_bak.xml");
            gdbm_usrbak = db_open("/tmp/user_bak.xml");
            if (gdbm_usrbak == NULL)
                ansData[0] = 0xFF;
            else {
                key.dptr = "user_version";
                key.dsize = strlen("user_version") + 1;
                data = key;
                if (db_store(gdbm_usrbak, key, data) < 0) {
                     DebugPrintf("\n---------user data deleted failed------");
                                         PrintScreen("\n---------user data deleted failed------");
                                         ansData[0] = 0xFF;
                }
#if DEBUG_CONN
                                DebugPrintf("\n---------user data deleted------");
                                PrintScreen("\n---------user data deleted------");
#endif
            }
                break;

        /***************************************标定*******************************************************/
    case 0x07:                               //增加物理卡(用户)
#if DEBUG_CONN
            DebugPrintf("\n------------set case 0x07-----");
#endif
        *length = 5;
        ansData=malloc(5);
        ansData[0] = 0x00;
        ansData[1] = 0x02;
        ansData[2] = 0x07;
        ansData[3] = dataBuffer[1];
        ansData[4] = dataBuffer[2];
        if (gdbm_usrbak == NULL) {
            ansData[0] = 0xFF;
            DebugPrintf("\n------------add user fail!! %d-----------", dataBuffer[1]);
            break;
        }
        for (temp = 3; temp < dataLenth;) {
                        /******************超级用户***********************/
                        if (dataBuffer[temp] == 'S') {					//判断是否为操作员，注意服务器同步超级用户的时候是'S'+卡号!!!
                                 memcpy(TempBuffer, dataBuffer+temp, 9);	//将需要添加用户卡号放在TempBuffer中
                                 TempBuffer[9] = 0;							//用户间以0x00隔开
                                 key.dptr = TempBuffer;
                                 key.dsize = 10;
                                 data = key;
                                 user_count++;
#if DEBUG_CONN
                DebugPrintf("\n--------TempBuffer = %s--user_count = %d ", TempBuffer, user_count);
#endif
                                 if (db_store(gdbm_usrbak, key, data) < 0) {			//保存超级用户到数据库
                                        DebugPrintf("\n------------add user fail!! %d-----------", dataBuffer[1]);
                                        ansData[0]=0xFF;
                                        break;
                                }
                                //else
                                        //gdbm_sync(gdbm_usrbak);

#if NDEBUG
                        if (gdbm_exists(gdbm_usrbak, key) != 0 )				//数据库已保存
                                DebugPrintf("\n------------really exists here--------------");
#endif
                                temp += 10;
                        }
                        /******************非机组用户***********************/
                        else if(dataBuffer[temp] == 'N'){					//判断是否为非机组操作员
                                memcpy(TempBuffer, dataBuffer+temp, 9);	//将需要添加用户卡号放在TempBuffer中
                                TempBuffer[9] = 0;
                                key.dptr = TempBuffer;
                                key.dsize = 10;
                                data = key;
                                user_count++;
#if DEBUG_CONN
                DebugPrintf("\n--------TempBuffer = %s--user_count = %d \n", TempBuffer, user_count);
#endif
                                 if (db_store(gdbm_usrbak, key, data) < 0) {			//保存超级用户到数据库
                                        DebugPrintf("\n------------add user fail!! %d-----------", dataBuffer[1]);
                                        ansData[0]=0xFF;
                                        break;
                                }
                                //else
                                        //gdbm_sync(gdbm_usrbak);

#if NDEBUG
                        if (gdbm_exists(gdbm_usrbak, key) != 0 )				//数据库已保存
                                DebugPrintf("\n------------really exists here--------------");
#endif
                                temp += 10;
                                }
                        /******************普通用户***********************/
                        else {													//普通用户
                                 memcpy(TempBuffer, dataBuffer+temp, 8);
                                 TempBuffer[8] = 0;
                                 key.dptr = TempBuffer;
                                 key.dsize = 9;
                                 data = key;

#if DEBUG_CONN
                                 DebugPrintf("\n--------TempBuffer = %s--user_count = %d ", TempBuffer, user_count);
#endif

                                 if (db_store(gdbm_usrbak, key, data) < 0) {		//保存普通用户
                                        DebugPrintf("\n------------add user fail!!-----------");
                                        DebugPrintf("\n------------add user fail!! %d-----------", dataBuffer[1]);
                                        ansData[0]=0xFF;
                                        break;
                                }
                                //else
                                        //gdbm_sync(gdbm_usrbak);
                                user_count++;
                                temp += 9;
#if NDEBUG
                                if (gdbm_exists(gdbm_usrbak, key) != 0 )
                                        DebugPrintf("\n------------really exists here--------------");
#endif
                        }
                }
                break;

    case 0x08:                               //更新用户版本号
#if DEBUG_CONN
            DebugPrintf("\n------------set case 0x08-----");
            PrintScreen("\n------------set case 0x08-----");
#endif
                trans_user = 0;
        *length = 3;
        ansData = malloc(3);
        ansData[0] = 0x00;
        ansData[1] = 0x02;
        ansData[2] = 0x08;

                DebugPrintf("\n-----------data version before user_version = %d-----");
                DebugPrintf("\n----------count_users = %d------------", count_users);

        user_version = 0;
        user_version += (dataBuffer[1]<<24)&0xFF000000;
        user_version += (dataBuffer[2]<<16)&0xFF0000;
        user_version += (dataBuffer[3]<<8)&0xFF00;
        user_version += (dataBuffer[4])&0xFF;
#if DEBUG_CONN
            DebugPrintf("\n------------user_version = %d-----", user_version);
#endif
        if (gdbm_usrbak == NULL) {
            ansData[0] = 0xFF;
#if DEBUG_CONN
            DebugPrintf("\n------------update user data fail = %d-----");
            PrintScreen("\n------------update user data fail = %d-----");
#endif
            user_version = 0;
            break;
        }
        memset(TempBuffer, 0, KEY_SIZE_MAX);
        memcpy(TempBuffer, "user_version", strlen("user_version"));
        //data.dsize = sizeof(user_version);
        //data.dptr = (char*)(&user_version);

        key.dptr = "user_version";
        key.dsize =strlen("user_version") + 1;
        data = key;
        if (db_store(gdbm_usrbak, key, data) < 0) {
            ansData[0] = 0xFF;
            db_close(gdbm_usrbak);
#if NDEBUG
            DebugPrintf("\n-------update user data err--------");
#endif
            break;
        }
        db_close(gdbm_usrbak);
        gdbm_usrbak = NULL;
        system("cp /tmp/user_bak.xml /tmp/user_cur.xml");
        update_user_xml = 1;
                break;

    case 0x09:                               //设置最短刷卡时间间隔
#if NDEBUG
            DebugPrintf("\n------------set case 0x09-----");
            PrintScreen("\n------------set case 0x09-----");
#endif
        *length = 4;
                ansData=malloc(4);
        ansData[0]=0x00;
        ansData[1]=0x02;
        ansData[2]=0x09;
        card_tlimit = 0;
        card_tlimit += (dataBuffer[1]<<8)&0xFF00;
        card_tlimit += (dataBuffer[2])&0xFF;
        if (card_tlimit < 30)
            card_tlimit = 30;
        write_at24c02b(220, dataBuffer[1]);
        write_at24c02b(221, dataBuffer[2]);
#if NDEBUG
            DebugPrintf("\n------------card_tlimit = %d-----", card_tlimit);
#endif
                break;

        case 0x10:                               //验证刷卡信息
                *length = 0;						 //终端发送，所以将包长度定位0
                ansData = malloc(4);
                ansData[0] = 0x00;
                ansData[0] = 0x02;
                ansData[1] = 0x0a;

                if (dataLenth != 11)
                        break;

                strncpy(TempBuffer, dataBuffer+1, 8);		//将收到去包头的服务器卡号的8个字节数据放在暂存缓冲里

                TempBuffer[8] = 0;

        if (!strcmp(TempBuffer, card_to_check)) {
                //arrive_card = 1;
                //pthread_mutex_lock(&cardsnd_lock);
                //pthread_mutex_unlock(&cardsnd_lock);
            if (dataBuffer[9] == 0x00)		//判断服务器状态，机组卡
                                arrive_flag = 1;
                        else if (dataBuffer[9] == 0x01)		//非机组卡
                                arrive_flag = 2;
                        else  								//验证失败
                                arrive_flag = 0;
            if  (pthread_cond_broadcast(&cardsnd_cond) != 0)		//收到服务器应答，结束阻塞
                                DebugPrintf("\n-------arrive card cond err----------");
        }
                break;

    case 0x11:				//验证物理卡号
        *length = 0;
        ansData = malloc(4);
        strncpy(TempBuffer, dataBuffer+1, 8);		//将收到去包头的服务器卡号的8个字节数据放在暂存缓冲里
        TempBuffer[8] = 0;							//第9字节改为0
        if (!strcmp(TempBuffer, card_to_send)) {	//将收到服务器发送的卡号与发送过去的卡号进行比较，相同则进入
            if (dataBuffer[9] == 0x00)				//判断服务器状态是否正确
                card_ack = 1;						//正确则应答1
            else
                card_ack = 0;						//出错应答0
            if  (pthread_cond_broadcast(&cardquery_cond) != 0)		//收到应答则对所有等待参数条件cardquery_cond的线程接触阻塞
                DebugPrintf("\n-------cardquery_cond err----------");
        }
        break;

        case 0x12:										//设置开放模式
                DebugPrintf("\n------------set case 0x12-----");
                PrintScreen("\n------------set case 0x07-----");
                *length = 3;
                ansData = malloc(3);
                ansData[0] = 0x00;
                ansData[1] = 0x02;
                ansData[2] = 0x12;

                device_mode = dataBuffer[1];			//设置设备开放模式
                if(dataBuffer[1] == 0x01) {
                        DebugPrintf("\n-----------Turn on open mode-----------");
                        PrintScreen("\n-----------Turn on open mode-----------");
                        write_at24c02b(239,0x01);			//在24C02上记录现在设备模式
                }
                else{
                        DebugPrintf("\n-----------Turn on half open mode-----------");
                        PrintScreen("\n-----------Turn on half open mode-----------");
                        write_at24c02b(239,0x00);
                }
                break;

        case 0x13:										//设置预约时间段
                DebugPrintf("\n------------set case 0x13-----");
                PrintScreen("\n------------set case 0x13-----");

                gdbm_ordertimebak = db_open("/tmp/ordertime.xml");
                DebugPrintf("\n-----------open dbm id:%d---------------",gdbm_ordertimebak);

                ansData = malloc(72);

                ansData[0] = 0x00;
                ansData[1] = 0x02;
                ansData[2] = 0x13;

                if(gdbm_ordertimebak == NULL) {
                        DebugPrintf("\n--------------open ordertime err!--------------");
                        ansData[0] = 0xFF;
                        //break;
                }
                else {
                        DebugPrintf("\n--------------open ordertime successfully!");
                }

                //temp = 1;
                //while(dataBuffer[temp]!='_')temp++;			//得到数据表ID字节数,现在dataBuffer[temp]应指向'_'
                *length = 35;							//应答字长为应答状态+0x02+0x13+数据表ID,数据ID长度为temp-1个字节

                for(i=3;i<35;i++){						//应答数据表ID
                        ansData[i] = dataBuffer[i-2];
                }
#if NDEBUG
                        DebugPrintf("\n---data ID is %s---",dataBuffer+1);
#endif

                for(i=34;i<72;i++)					//TempBuffer得到卡号
                        TempBuffer[i-34] = dataBuffer[i];
                TempBuffer[38] = 0;
                DebugPrintf("\n----------key:%s---------------",TempBuffer);

                for(i=43;i<72;i++)
                        TempDatabuf[i-43] = dataBuffer[i];		//TempDatabuf得到预约时间
                TempDatabuf[29] = 0;
                DebugPrintf("\n----------ordertime :%s---------------",TempDatabuf);

                key.dptr = TempBuffer;						//将卡号作为检索的关键字
                key.dsize = strlen(TempBuffer)+1;

                data.dptr = TempDatabuf;					//将预约时间作为内容
                data.dsize = strlen(TempDatabuf)+1;

        //pthread_mutex_lock(&ordertime_lock);
                if (db_store(gdbm_ordertimebak, key, data) < 0) {		//保存预约记录
                        DebugPrintf("\n-------- store ordertime err ------");
                        ansData[0] = 0xFF;
                        //pthread_mutex_unlock(&cardfile_lock);
                        db_close(gdbm_ordertimebak);
                        gdbm_ordertime = NULL;
                        return ansData;
                        break;
                }
                else {
                        DebugPrintf("\n---store ordertime success!---");

                        db_close(gdbm_ordertimebak);
                        DebugPrintf("\n-----------close dbm id:%d---------------",gdbm_ordertimebak);
                        gdbm_ordertimebak = NULL;

                        //system("cp /tmp/ordertime_bak.xml /tmp/ordertimecur.xml");
                }

                updata_ordertime_xml = 1;
                return ansData;
                break;
        case 0x14:								//远程更新程序
                DebugPrintf("\n------------set case 0x14-----");
                PrintScreen("\n------------set case 0x14-----");
                beginupload = 1;
#if DEBUG_DATA
                DebugPrintf("\nremote update software!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!");
#endif

                ansData = malloc(5);
                ansData[1] = 0x02;
                ansData[2] = 0x14;
                ansData[3] = dataBuffer[5];		//序号
                ansData[4] = dataBuffer[6];
        software_seqtmp = 0;
        software_seqtmp+= (dataBuffer[5]<<8)&0xFF00;		//实际收到的序号
        software_seqtmp += (dataBuffer[6])&0xFF;

#if DEBUG_DATA
                DebugPrintf("\nsoftware_seq=%d\nsoftware_seqtmp=%d",software_seq,software_seqtmp);
#endif
                char cmdtmp[20],soft_nametmp[20];			//for saving file/cmd name
                unsigned int RetWrite;
                FILE *fp;

        if(led_state==1)				//当前有人在上机
                {
                        beginupload = 0;
#if DEBUG_DATA
                        DebugPrintf("\nThis meachine is used!!!!",software_seq,software_seqtmp);
#endif
                        ansData[0] = 0xff;
                }
        else if(software_seqtmp==0)				//the first packet
        //////////////////////在此接受程序//////////////////////////////
        {
                        system("rm /tmp/Tmp*");		//清除上次的临时文件
                        sprintf(soft_nametmp,"/tmp/Tmp_Soft");

                        fp = fopen(soft_nametmp,"ab+");		//以附加方式打开二进制文件，若文件不存在，就新建文件
                        RetWrite = fwrite(dataBuffer+7,1,dataLenth-7,fp);		//write data into the binary file

                        if(fclose(fp)!=0)
                        {
                                perror("\nFile Close Error!");
                        }
                        //system("cat /tmp/Tmp_Soft");

                        byte_count +=RetWrite;
                        byte_all = 0;
                        byte_all+= (dataBuffer[1]<<24)&0xFF000000;		//the size of the whole program
                        byte_all += (dataBuffer[2]<<16)&0xFF0000;
                        byte_all += (dataBuffer[3]<<8)&0xFF00;
                        byte_all += (dataBuffer[4])&0xFF;
#if DEBUG_CONN
                        DebugPrintf("\nWRITE:%dBYTES",RetWrite);
                        DebugPrintf("\nPROGRAM SIZE:%d BYTE",byte_all);
#endif
                        ansData[0] = 0x00;
                }
                else if((software_seqtmp-software_seq)==1)			//序号必须连续
                {
                        sprintf(soft_nametmp,"/tmp/Tmp_Soft");
                        fp = fopen(soft_nametmp,"ab+");		//以附加方式打开文件，若文件不存在，就新建文件

#if NDEBUG
                        DebugPrintf("\ndataLenth = %d",dataLenth);
#endif

                        RetWrite = fwrite(dataBuffer+7,1,dataLenth-7,fp);		//write data into the binary file

                        if(fclose(fp)!=0)
                        {
                                DebugPrintf("\n----Update File Close Error!----");
                                return 0;
                        }

#if NDEBUG
                        DebugPrintf("\nWRITE:%dBYTES",RetWrite);
#endif
                        byte_count +=RetWrite;
                        ansData[0] = 0x00;
                }
                else
        //////////////////////序列不连续//////////////////////////////
                {
#if DEBUG_CONN
                        DebugPrintf("\nsoftware_seq=%d\nsoftware_seqtmp=%d  !!!",software_seq,software_seqtmp);
                        DebugPrintf("\nThe Sequency Is Not Continuous");
#endif
                        ansData[0] = 0xfe;
                }

                software_seq = software_seqtmp;		//更新序号暂存值
                *length = 5;
                return ansData;
                break;

        case 0x15:								//更新程序版本号
                DebugPrintf("\n------------set case 0x15-----");
                *length = 3;
                ansData = malloc(3);
                ansData[1] = 0x02;
                ansData[2] = 0x15;

        software_version = 0;														//software version
        software_version += dataBuffer[1] << 24;			//high byte in the front,low byte in the back
        software_version += dataBuffer[2] << 16;
        software_version += dataBuffer[3] << 8;
        software_version += dataBuffer[4] ;

        char*  program_tmp=malloc(byte_all);							//for store tmporary program
        unsigned int CRC_server=0,CRC_local;
        sprintf(soft_nametmp,"/tmp/Tmp_Soft");
        fp=fopen(soft_nametmp,"r");        											//open the file
        fread(program_tmp,1,byte_all,fp);										//read the file to "program_tmp"
        fclose(fp);
        CRC_local = CRC_check(program_tmp,0,byte_all);		//caculate the result of CRC
        CRC_server += (dataBuffer[5]<<24)&0xFF000000;		//CRC result of the server
        CRC_server += (dataBuffer[6]<<16)&0xFF0000;
        CRC_server += (dataBuffer[7]<<8)&0xFF00;
        CRC_server += (dataBuffer[8])&0xFF;

#if DEBUG_DATA
                        //DebugPrintf("\nALL BYTE = %d\nCRC_server=%x\nCRC_local=%x",CRC_server,CRC_local);
                        //PrintScreen("\nALL BYTE = %d\nCRC_server=%x\nCRC_local=%x",CRC_server,CRC_local);
#endif

        if(CRC_server!=CRC_local)
        {
            ansData[0] = 0xff;
#if DEBUG_DATA
            DebugPrintf("\nCRC check failed!");
            PrintScreen("\nCRC check failed!");
#endif
            return ansData;
            break;
        }
        else
        {
            ansData[0] = 0x00;
#if DEBUG_DATA
            DebugPrintf("\nCRC check success!");
            PrintScreen("\nCRC check success!");
#endif
        }
#if DEBUG_DATA
            DebugPrintf("\nMAKE AN COPY TO NANDFLASH!");
#endif
        sprintf(cmdtmp,"cp /tmp/Tmp_Soft /mnt");		//save a copy of the new program in the RAM to the nand flash
        system(cmdtmp);

#if DEBUG_DATA
        DebugPrintf("\nERASE NORFLASH!");
#endif
        write_at24c02b(236,0);		//set update bit,shows the program has not been updated
        sprintf(cmdtmp,"/usb/./mtd_debug erase /dev/mtd0 0x02a0000 0x150000");		//eraze nor flash:1.5M
        system(cmdtmp);

#if DEBUG_DATA
        DebugPrintf("\nWRITE PROGRAM TO NORFLASH!");
#endif

        sprintf(cmdtmp,"/usb/mtd_debug write /dev/mtd0 0x02a0000 %d /tmp/Tmp_Soft",byte_all);		//write the program to nor flash
        system(cmdtmp);
                                                                                //wait
        write_at24c02b(236,1);		//reset update bit,shows the program has been updated

        write_at24c02b(85,dataBuffer[1]);		//将版本号存至E2PROM
        write_at24c02b(86,dataBuffer[2]);
        write_at24c02b(87,dataBuffer[3]);
        write_at24c02b(88,dataBuffer[4]);

        beginupload = 0;
#if DEBUG_DATA
            DebugPrintf("\nGOING TO REBOOT!");
            PrintScreen("\nGOING TO REBOOT!");
#endif
        reboot_flag = 1;
        return ansData;
        break;

        case 0x0c:                               //挂载、卸载U盘
             *length = 4;
                 ansData=malloc(4);
             if(dataBuffer[1]==0xff)
              {
             /*   mount_re=u_mount();
                if(mount_re==0)
                    {DebugPrintf("\n-------------挂载U盘成功-----------\n");
                                ansData[0]=0x00;
                                ansData[1]=0x02;
                                ansData[2]=0x0c;
                                ansData[3]=0x01;
                    }
                    else
                    {DebugPrintf("\n-------------挂载U盘失败-----------\n");
                                ansData[0]=0xff;
                                ansData[1]=0x02;
                                ansData[2]=0x0c;
                                ansData[3]=0;
                    }*/

              }
             if(dataBuffer[1]==0x00)
               {
             /*    umount_re=u_umount();
                 if(umount_re==0)
                    {DebugPrintf("\n-------------卸载U盘成功-----------\n");
                                ansData[0]=0x00;
                                ansData[1]=0x02;
                                ansData[2]=0x0c;
                                ansData[3]=0x01;
                                mount_re=1;
                    }
                    else
                    {DebugPrintf("\n-------------卸载U盘失败-----------\n");
                                ansData[0]=0xff;
                                ansData[1]=0x02;
                                ansData[2]=0x0c;
                                ansData[3]=0;
                    }*/
               }
        }
        return ansData;
}

static void* SyncParketExec(void *arg)
{
        stuConnSock *conn = (stuConnSock *)arg;
        unsigned int leftLen, headFlag, packLen, ackLen=0;
        unsigned char cmdWord, *buffer, *oriAddr, aCmdBuff[2048];
        unsigned char *tmpBuffer;

        int count = 0;
        while (1)
        {
                pthread_testcancel();  //线程取消点
                //DebugPrintf("\n-----------------------------------\n");
        if( conn->packBuffIn==NULL)continue;
                pthread_mutex_lock(&conn->lockBuffIn);



                if ((conn->fdSock>0) && (conn->curReadth>0))                //扫描可用可读写socket
                {
                        //DebugPrintf("\n-----SyncParketExec running-------\n");
                        leftLen = conn->curReadth + conn->remainPos;
                        conn->curReadth = 0;                                    //接收缓冲区当前未处理字节数归零
                        oriAddr = conn->packBuffIn;
                        buffer = oriAddr;

#if DEBUG_DATA
                        DebugPrintf("\n----SyncParketExec--parse Packet Total Length = %d", leftLen);
#endif

                        while (leftLen > 7)
                        {
                                while ((*buffer==36) && (leftLen>0))                               //跳过带外数据:$
                                {
                                        --leftLen;
                                        ++buffer;
                                }
                                /***********************************************
                                while ((headFlag = *((int *)buffer)) != 0x101B101B ) {
                                                if (leftLen <= 7) break;
                                                --leftLen;
                                                ++buffer;

                                }
                                ************************************************/

                                if (leftLen <= 7) break;

                                if ((headFlag = *((int *)buffer)) == 0x101B101B)                  	 //确认是包头
                                {
                                        cmdWord = buffer[4];                                           		//命令字
                                        packLen = (int)((buffer[5]<<8) + buffer[6]);                  	 //获取有效数据长度
                                        if (packLen > 2048) {
                                                leftLen = 0;
                                                DebugPrintf("\n-------packet too large packLen = 0x%x------", packLen);
                                                pthread_mutex_unlock(&conn->lockBuffIn);

                                                ////////////////添加锁
                                                pthread_mutex_lock(&conn->lockBuffOut);
                                                SockPackSend((unsigned char)PACKET_ERR, conn->fdSock, NULL, NULL, 0);       //数据包处理出错
                                                pthread_mutex_unlock(&conn->lockBuffOut);


                                                pthread_testcancel();  //线程取消点
                                                pthread_mutex_lock(&conn->lockBuffIn);
                                                break;
                                        }

#if DEBUG_DATA
                                        DebugPrintf("\n----SyncParketExec--parse Packet Length = %d", packLen);
                                        DebugPrintf("\n--buffer[7] = %d   buffer[8] = %d--", buffer[7], buffer[8]);
                                        fflush(stdout);
#endif

                                        if (packLen > leftLen)                                         //包不完整
                                        {
                                                break;
                                        }

                                        memmove(aCmdBuff, buffer+7, packLen-7);                        //缓存一次命令 不保留固定头
                                        leftLen -= packLen;                                            			//解析下一个包
                                        buffer += packLen;
                                        memmove(oriAddr, buffer, leftLen);                             //清空已处理命令
                                        buffer = oriAddr;
                                        conn->remainPos = leftLen;                                     //此次通信不完整包遗留字节数

                                        pthread_mutex_unlock(&conn->lockBuffIn);

                                        unsigned char *cmdAck;

                                        int i;
#ifdef DEBUG_CON
                                        for (i=0; i<packLen-7; i++)
                                        {
                                            DebugPrintf("-0x%X", aCmdBuff[i]);
                                            fflush(stdout);
                                        }
#endif

                                        switch (cmdWord)                                               //解析出一个完整包 执行
                                        {
                                        case CMD_QUERY:                                                //查询命令

#if NDEBUG
                        DebugPrintf("\n-*---***---*-received QUERY command packetlen = %d", packLen);
#endif
                                                cmdAck = query_Para(aCmdBuff, packLen-7, &ackLen);
                                                break;
                                        case CMD_STDSET:                                               //设置命令
#if NDEBUG
                        DebugPrintf("\n-*---***---*-received SETTINGS packetlen = %d", packLen);
#endif
                        cmdAck = set_Para(aCmdBuff, packLen-7, &ackLen);			//返回值为数据段的首地址
                        //DebugPrintf("\ncmdAck[0] = %x",*cmdAck);
                                                break;
                                        case 3: break;
                                        default:
                                                DebugPrintf("\n-*---***---*-received command unknown");
                                                break;
                                        }
                                        if(ackLen != 0)
                                        {
                                                ////////////添加锁/////////////
                                                pthread_mutex_lock(&conn->lockBuffOut);
                                                SockPackSend((unsigned char)CMD_ACK, conn->fdSock, conn, cmdAck, ackLen);
                                                pthread_mutex_unlock(&conn->lockBuffOut);
                                                if(reboot_flag==1)
                                                {
                                                        system("reboot");
                                                }
                                        }
                                        else
                                        {
#if NDEBUG
                                                for (i=0;  i<ackLen;  i++)
                                                {
                                                        DebugPrintf(" %x-", cmdAck[i]);
                                                        fflush(stdout);
                                                }
#endif
                                        }
                                        if(cmdAck[0] == 0x00 && cmdAck[1] == 0x02 && cmdAck[2] == 0x02)
                                        {
                                                sleep(1);
                                                system("reboot");
                                        }
                                        else if(cmdAck[0] == 0x00 && cmdAck[1] == 0x02 && cmdAck[2] == 0x0b)
                                        {
                                                gIP_change = 1;
                                        }
                                        if (cmdAck != NULL) {

                                                free(cmdAck);

                                        }
                                        pthread_testcancel();  //线程取消点
                                        pthread_mutex_lock(&conn->lockBuffIn);
                                }
                                else                                                              //向客户端发出错命令
                                {
#if NDEBUG
                                        DebugPrintf("\n----SyncParketExec--Packet header error!headFlag = %x", headFlag);
                                        for (leftLen = 0; leftLen < 100; leftLen++) {
                                                DebugPrintf("\n----------buffer[%d] = %x -------",leftLen, buffer[leftLen]);
                                        }
#endif
                                        leftLen = 0;

                                        pthread_mutex_unlock(&conn->lockBuffIn);

                                        ////////////////添加锁
                                        pthread_mutex_lock(&conn->lockBuffOut);
                                        SockPackSend((unsigned char)PACKET_ERR, conn->fdSock, NULL, NULL, 0);       //数据包处理出错
                                        pthread_mutex_unlock(&conn->lockBuffOut);


                                        pthread_testcancel();  //线程取消点
                                        pthread_mutex_lock(&conn->lockBuffIn);
                                        break;
                                }
                        }
                        conn->remainPos = leftLen;                                      //此次通信不完整包遗留字节数
                }
                pthread_mutex_unlock(&conn->lockBuffIn);
        }
}

static void SockRelease(stuConnSock *sttParm)
{
    sttParm->fdSock = 0;
        sttParm->loginLegal = 0;

        pthread_mutex_lock(&sttParm->lockBuffIn);
        pthread_mutex_lock(&sttParm->lockBuffOut);
        trans_user = 0;
        shutdown(sttParm->fdSock, SHUT_RDWR);
        FD_CLR(sttParm->fdSock, &exEvents);
        FD_CLR(sttParm->fdSock, &rdEvents);
        close(sttParm->fdSock);
        //sttParm->fdSock = 0;
        //sttParm->loginLegal = 0;
        beginsendbmp = 1;
        beginsyncbmp = 0;
        if (sttParm->isPid > 0)
        {
                pthread_cancel(sttParm->isPid);    //取消该socket通信线程
        }
        sttParm->isPid = 0;                    //pid归零

        memset((unsigned char *)&sttParm->clientAddr, 0, sizeof(struct in_addr));
        sttParm->remainPos = 0;
        sttParm->curReadth = 0;
        free(sttParm->packBuffIn);
        free(sttParm->packBuffOut);
        sttParm->packBuffIn = NULL;
        sttParm->packBuffOut = NULL;
        sttParm->noProbes = 0;

#if DEBUG_DATA
        DebugPrintf("\n------ & ------SockRelease--current thread delete");
        PrintScreen("\n------ & ------SockRelease--current thread delete");
        fflush(stdout);
#endif

        pthread_mutex_unlock(&sttParm->lockBuffOut);
        pthread_mutex_unlock(&sttParm->lockBuffIn);
        pthread_mutex_destroy(&sttParm->lockBuffIn);
        pthread_mutex_destroy(&sttParm->lockBuffOut);
}


static int SockThreadOpen(stuConnSock *sttParm)
{
        pthread_t pid;
        pthread_attr_t attr;
        int err, policy;

        err = pthread_attr_init(&attr);
        if (err != 0)
        {
                perror("\n----SockThreadOpen--pthread_attr_init err");
                SockPackSend((unsigned char)SERVER_ERR, sttParm->fdSock, NULL, NULL, 0);    //服务器出错
                SockRelease(sttParm);
                return err;
        }

        policy = SCHED_RR;                                                   //设置线程调度策略
        pthread_attr_setschedpolicy(&attr, policy);

        err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
        if (err == 0)
        {
                //DebugPrintf("\n----SockThreadOpen--pthread_attr_setdetachstate ok\n");
                err = pthread_create(&pid, &attr, SyncParketExec, (void*)sttParm/*(void*)&sttConnSock[i]*/);
                if (err != 0)
                {
                        perror("\n----SockThreadOpen--pthread_create err");
                        SockPackSend((unsigned char)SERVER_ERR, sttParm->fdSock, NULL, NULL, 0);    //服务器出错
                        SockRelease(sttParm);
                        return err;
                }
                #if DEBUG_DATA
                DebugPrintf("\n----------------SockThreadOpen--pthread tid = %d", pid);
                fflush(stdout);
                #endif
                sttParm->isPid = pid;        //记录当前socket线程id
        }

        err = pthread_attr_destroy(&attr);
        if (err != 0)
        {
                perror("\n----SockThreadOpen--pthread_attr_destroy err");
                SockPackSend((unsigned char)SERVER_ERR, sttParm->fdSock, NULL, NULL, 0);        //服务器出错
                SockRelease(sttParm);
                return err;
        }
        return 0;
}


static int SockBuffRecv(stuConnSock *sttParm)
{
        size_t recvLen = 0;
        int curRead;
        unsigned char *tagAddr;

        pthread_mutex_lock(&sttParm->lockBuffIn);

        recvLen = sttParm->curReadth + sttParm->remainPos;     //记录当前用户缓冲区上次未处理数据长度
        tagAddr = sttParm->packBuffIn + recvLen;
        //memset(tagAddr, 0, RECV_BUFF_SIZE-recvLen);            //当前用户缓冲区余下空间清零

        do
        {
                curRead = recv(sttParm->fdSock, tagAddr, RECV_BUFF_SIZE-recvLen, 0);        //读系统缓冲区 最大到当前用户缓冲区满
                if (curRead < 0)
                {
                        if (errno == EINTR)
                        {
                                continue;
                        }
                        if (errno != EAGAIN)    //POSIX.1 对于一非阻塞描述符若无数据可读 read返回-1 errno置EAGAIN
                        {
#if DEBUG_DATA
                                DebugPrintf("\n----SockBuffRecv--current receive bytes = %d, error reason: %d", curRead, errno);
                                fflush(stdout);
#endif

                                pthread_mutex_unlock(&sttParm->lockBuffIn);
                                return -1;
                        }
                        curRead = 0;
                }

#if DEBUG_DATA
                DebugPrintf("\n----SockBuffRecv--receive now all bytes = %d", curRead);
                fflush(stdout);
#endif

                recvLen += curRead;
                tagAddr += curRead;

        } while (curRead > 0);

        pthread_mutex_unlock(&sttParm->lockBuffIn);

#if NDEBUG
        DebugPrintf("\n----SockBuffRecv--over, total recvbuff bytes: %d", recvLen);
        int i;
        for (i=0; i<recvLen; i++)
        {
                DebugPrintf("  %x", sttParm->packBuffIn[i]);
                fflush(stdout);
        }
        DebugPrintf("\n");
#endif

        return (recvLen-sttParm->curReadth-sttParm->remainPos);       //返回此次读取长度(非当前用户缓冲区数据长度)
}

static int DispatchPacket(stuConnSock *sttParm)
{
        int fdConn, thisRead;
        unsigned long rp;

        fdConn = sttParm->fdSock;
        thisRead = SockBuffRecv(sttParm);
#if NDEBUG
        DebugPrintf("\n---thisRead = %d---------", thisRead);
#endif
        if (thisRead <= 0)                //判断socket关闭
        {
                #if DEBUG_DATA
                DebugPrintf("\n----DispatchPacket--Connection lost: FD = %d", sttParm->fdSock/*, curSlot*/);
                #endif
                SockRelease(sttParm);
                return fdConn;
        }

        //数据包解析处理

        pthread_mutex_lock(&sttParm->lockBuffIn);

        sttParm->curReadth += thisRead;    //接收缓冲区当前未处理字节数累加 一旦当前缓冲区数据进入线程处理函数 该值归零

#if NDEBUG
        DebugPrintf("\n----DispatchPacket--%d bytes received, readey for parse Packet", thisRead);
#endif

        pthread_mutex_unlock(&sttParm->lockBuffIn);

        return 0;
}

void LedTwinkle()
{
        static int i = 0;
        if (i == 16) i=0;
        if(ledtwinklebegin == 1)
        {
                lederrcount++;
                if(lederrcount == 100)
                {
                        TurnLedOn();
                        //Ch450Write(BCD_decode_tab[i],BCD_decode_tab[i],BCD_decode_tab[i]);
                        //i++;
                        //DebugPrintf("arg is %x \n", ReadVol());
                }
                if(lederrcount == 200)
                {
                        TurnLedOff();
                        lederrcount = 0;
                }
        }
        else
        {
                TurnLedOn();
                lederrcount = 0;
        }

}


int _ConnLoop()
{
        fd_set catchRdEvents, carchExEvents;
        int rtn, i;
        int fdListen = 0, fdConn, fdClose, highConn;
        struct timeval tmout;
        unsigned char oobytes;

        while(1)                                      		//网络检测
        {
                if (gIP_change == 1)                      //IP重设
                {

                        if (fdListen > 0)
                        {
                                close(fdListen);                  //关闭原先IP监听套接字
                                FreeMemForEx();                   //释放原先连接的资源
                        }
                        fdListen = SockServerInit();
                        if (fdListen == -1)
                        {
#ifdef DEBUG
                                DebugPrintf("\n----_ConnLoop--Server init listen error!");
#endif
                                return -1;
                        }

#ifdef DEBUG
                        DebugPrintf("\n----_ConnLoop--start to listen socket = %d", fdListen);
#endif

                        FD_ZERO(&exEvents);
                        FD_ZERO(&rdEvents);
                        FD_SET(fdListen, &rdEvents);
                        highConn = fdListen;

                        memset((unsigned char *)&sttConnSock, 0, sizeof(sttConnSock));
                        HeartbeatInit(60, 2);
                        gIP_change = 0;
                        trans_user = 0;
                }
                //LedTwinkle();
                for (i=0; i<MAX_LINK_SOCK; i++)
                {
                        if ((sttConnSock[i].noProbes>gmax_nalarms) || (sttConnSock[i].loginLegal==-1))    //带外数据接收超时 或登录非法
                        {
                                DebugPrintf("\n---长时间无法接受查询命令，断开连接---");
                                SockRelease(&sttConnSock[i]);
                                //return -1;
                        }
                }

                if(sttConnSock[0].fdSock > 0 && sockreleasebegin == 1)
                {
                        SockRelease(&sttConnSock[0]);
                        sockreleasebegin = 0;
                }

                tmout.tv_sec = 0;
                tmout.tv_usec = 10;

                //pthread_mutex_lock(&gspecset_lock);
                carchExEvents = exEvents;
                catchRdEvents = rdEvents;
                //pthread_mutex_unlock(&gspecset_lock);
        // system("ifconfig eth0 up");
                rtn = select(highConn+1, &catchRdEvents, NULL, &carchExEvents, &tmout);
                //pthread_mutex_unlock(&gspecset_lock);

                if ((rtn<0) && (errno==EINTR))continue;
                if (rtn < 0)
                {
#ifdef DEBUG
                        DebugPrintf("\n----_ConnLoop--select error = %d", errno);
#endif
                        close(fdListen);
                        return -1;
                }
                if (rtn == 0)                             //当前无可读写套结字描述符
                {
                        //DebugPrintf("nothing detected\n");
                        //fflush(stdout);
                        continue;
                }

                if (FD_ISSET(fdListen, &catchRdEvents))   //确认监听套结字可通信
                        //如果有新的客户连接请求，经过select函数后进入此处，并不是每次循环都进入。
                {
                        DebugPrintf("\n-----FD_ISSET(fdListen, &catchRdEvents)-----");
                        if(sttConnSock[0].fdSock > 0)
                        {
                                DebugPrintf("\n-------SockRelease sock for new conn-------");
                                SockRelease(&sttConnSock[0]);
                        }
                        fdConn = HandleNewConn(fdListen);		//从处于fdListen监听状态的流套接字的客户连接请求队列中取出排在最前面的一个客户请求，返回新创建的套接字


                        if (fdConn == 0)continue;             //服务器忙或未响应

                        FD_SET(fdConn, &exEvents);
                        FD_SET(fdConn, &rdEvents);
                        if (fdConn > highConn)
                        {
                                highConn = fdConn;
                        }

                }

                for (i=0; i<MAX_LINK_SOCK; i++)           //在建立的socket集中查找请求读写的socket
                {
#ifdef DEBUG
                        DebugPrintf("\n----_ConnLoop--check to read slot: %d", i);
#endif
                        if (FD_ISSET(sttConnSock[i].fdSock, &carchExEvents))		//检测carchExEvents集是否变化
                        {
                                if (recv(sttConnSock[i].fdSock, &oobytes, 1, MSG_OOB) < 0)
                                {
                                //DebugPrintf("\n-----MSG_OOB < 0-----\n");
                                }
                                sttConnSock[i].noProbes = 0;      //心搏函数监测值归零
                        }
                        else
                        {
                                if (sttConnSock[i].fdSock > 0)
                                {
                                        FD_SET(sttConnSock[i].fdSock, &exEvents);               //恢复监听该socket连接带外数据
                                }
                        }

                        if (FD_ISSET(sttConnSock[i].fdSock, &catchRdEvents))  //接收数据和客户关闭连接时都进入此处
                        {
                                DebugPrintf("\n-----if (FD_ISSET(sttConnSock[%d].fdSock, &catchRdEvents))-----",i);
                                sttConnSock[i].noProbes = 0;      //心搏函数监测值归零
                                fdClose = DispatchPacket(&sttConnSock[i]);      //通信流读到用户缓冲区

                                if (fdClose)                                    //无数据可读写 说明socket关闭 || 进入此FD_ISSET，而rec<0,则说明socket关闭
                                {
                                        //FD_CLR(fdClose, &exEvents);
                                        //FD_CLR(fdClose, &rdEvents);

                                        if (fdClose == highConn)                    //刷新最大socket句柄
                                        {
                                                highConn = fdListen;
                                                for (i=0; i<MAX_LINK_SOCK; i++)
                                                {
                                                        if (sttConnSock[i].fdSock > highConn)
                                                        {
                                                                highConn = sttConnSock[i].fdSock;
                                                        }
                                                }
                                        }
                                        continue;
                                }

                                ////////////////////socket thread create/////////////////////
                                if (sttConnSock[i].isPid == 0)          //线程未创建
                                {
                                        SockThreadOpen(&sttConnSock[i]);
                                }
                                ////////////////////socket thread running////////////////////

#ifdef DEBUG
                                DebugPrintf("\n------------socket end------------#####################");
#endif
                        }
                        else
                        {
                                if (sttConnSock[i].fdSock > 0)
                                {
#ifdef DEBUG
                                        DebugPrintf("\n----_ConnLoop--restore link sock: %d", i);
#endif
                                        FD_SET(sttConnSock[i].fdSock, &rdEvents);    //恢复可用socket监听
                                }
                        }
                }

#ifdef DEBUG
                DebugPrintf("\n--------------------------------socket over--------------------------------");
#endif

        }
}

int readNlSock(int sockFd, char *bufPtr, int seqNum, int pId)
{
        struct nlmsghdr *nlHdr;
        int readLen = 0, msgLen = 0;
        do{
                //收到内核的应答
                if((readLen = recv(sockFd, bufPtr, BUFSIZE - msgLen, 0)) < 0)
                {
                        perror("\nSOCK READ: ");
                        return -1;
                }

                nlHdr = (struct nlmsghdr *)bufPtr;
                //检查header是否有效
                if((NLMSG_OK(nlHdr, readLen) == 0) || (nlHdr->nlmsg_type == NLMSG_ERROR))
                {
                        perror("\nError in recieved packet");
                        return -1;
                }

                /* Check if the its the last message */
                if(nlHdr->nlmsg_type == NLMSG_DONE)
                {
                        break;
                }
                else
                {
                        /* Else move the pointer to buffer appropriately */
                        bufPtr += readLen;
                        msgLen += readLen;
                }

                /* Check if its a multi part message */
                if((nlHdr->nlmsg_flags & NLM_F_MULTI) == 0)
                {
                        /* return if its not */
                        break;
                }
        } while((nlHdr->nlmsg_seq != seqNum) || (nlHdr->nlmsg_pid != pId));
        return msgLen;
}

//分析返回的路由信息
void parseRoutes(struct nlmsghdr *nlHdr, struct route_info *rtInfo, char *gateway)
{
        struct rtmsg *rtMsg;
        struct rtattr *rtAttr;
        int rtLen;
        char *tempBuf = NULL;
        //2007-12-10
        struct in_addr dst;
        struct in_addr gate;

        tempBuf = (char *)malloc(100);
        rtMsg = (struct rtmsg *)NLMSG_DATA(nlHdr);
        // If the route is not for AF_INET or does not belong to main routing table
        //then return.
        if((rtMsg->rtm_family != AF_INET) || (rtMsg->rtm_table != RT_TABLE_MAIN))
                return;
        /* get the rtattr field */
        rtAttr = (struct rtattr *)RTM_RTA(rtMsg);
        rtLen = RTM_PAYLOAD(nlHdr);
        for(;RTA_OK(rtAttr,rtLen);rtAttr = RTA_NEXT(rtAttr,rtLen)){
                switch(rtAttr->rta_type) {
        case RTA_OIF:
                if_indextoname(*(int *)RTA_DATA(rtAttr), rtInfo->ifName);
                break;
        case RTA_GATEWAY:
                rtInfo->gateWay = *(u_int *)RTA_DATA(rtAttr);
                break;
        case RTA_PREFSRC:
                rtInfo->srcAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
        case RTA_DST:
                rtInfo->dstAddr = *(u_int *)RTA_DATA(rtAttr);
                break;
                }
        }
        dst.s_addr = rtInfo->dstAddr;
        if (strstr((char *)inet_ntoa(dst), "0.0.0.0"))
        {
                gate.s_addr = rtInfo->gateWay;
                sprintf(gateway, (char *)inet_ntoa(gate));
        }
        free(tempBuf);
        return;
}


/********************************************************************
* 函数名： get_gateway
* 参数名： gateway(out)   网关
* 返回值： 0              成功
*          -1             失败
* 功  能：获取本地机的网关
********************************************************************/
int get_gateway(char *gateway)
{
        struct nlmsghdr *nlMsg;
        struct rtmsg *rtMsg;
        struct route_info *rtInfo;
        char msgBuf[BUFSIZE];

        int sock, len, msgSeq = 0;
        //创建 Socket
        if((sock = socket(PF_NETLINK, SOCK_DGRAM, NETLINK_ROUTE)) < 0)
        {
                perror("\nSocket Creation: ");
                return -1;
        }

        /* Initialize the buffer */
        memset(msgBuf, 0, BUFSIZE);

        /* point the header and the msg structure pointers into the buffer */
        nlMsg = (struct nlmsghdr *)msgBuf;
        rtMsg = (struct rtmsg *)NLMSG_DATA(nlMsg);

        /* Fill in the nlmsg header*/
        nlMsg->nlmsg_len = NLMSG_LENGTH(sizeof(struct rtmsg)); // Length of message.
        nlMsg->nlmsg_type = RTM_GETROUTE; // Get the routes from kernel routing table .

        nlMsg->nlmsg_flags = NLM_F_DUMP | NLM_F_REQUEST; // The message is a request for dump.
        nlMsg->nlmsg_seq = msgSeq++; // Sequence of the message packet.
        nlMsg->nlmsg_pid = getpid(); // PID of process sending the request.

        /* Send the request */
        if(send(sock, nlMsg, nlMsg->nlmsg_len, 0) < 0){
                DebugPrintf("\nWrite To Socket Failed...");
                return -1;
        }

        /* Read the response */
        if((len = readNlSock(sock, msgBuf, msgSeq, getpid())) < 0) {
                DebugPrintf("\nRead From Socket Failed...");
                return -1;
        }
        /* Parse and print the response */
        rtInfo = (struct route_info *)malloc(sizeof(struct route_info));
        for(;NLMSG_OK(nlMsg,len);nlMsg = NLMSG_NEXT(nlMsg,len)){
                memset(rtInfo, 0, sizeof(struct route_info));
                parseRoutes(nlMsg, rtInfo,gateway);
        }
        free(rtInfo);
        close(sock);
        return 0;
}

int netcheckagain(char *tem_ipdz,char *tem_zwym,char *tem_mrwg)
{
        struct sockaddr_in *my_ip;
        struct sockaddr_in *addr;
        struct sockaddr_in myip;
        my_ip = &myip;
        struct ifreq ifr;
        int sock;

        if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
                DebugPrintf("\nsock error ");
                return -1;
        }
        strcpy(ifr.ifr_name, "eth0");

        //取本机IP地址
        if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
        {
                DebugPrintf("\nioctl SIOCGIFADDR \n");
                return -1;
        }
        my_ip->sin_addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr;
        strcpy(tem_ipdz,inet_ntoa(my_ip->sin_addr));
        //DebugPrintf("\n%s\n",tem_ipdz);

        //取本机掩码
        if(ioctl( sock, SIOCGIFNETMASK, &ifr) == -1)
        {
                perror("\n[-] ioctl");
                return -1;
        }
        addr = (struct sockaddr_in *) & (ifr.ifr_addr);
        strcpy(tem_zwym,inet_ntoa(addr->sin_addr));
        get_gateway(tem_mrwg);

        close(sock);
        return 0;
}

int DelFile(char *filename)
{
        char ml[50]="rm -rf ";
        strcat(ml,filename);
        system(ml);
}

/*******************************************************************************************
**函数名称：	 wrconfile(const char *str,const char *filename)
**函数功能：	 向配置文件中写入信息
**入口参数：	const char *str		-- 需要写入的信息
**			const char *filename--需要写入信息的文件
**返 回 值：
**说    明：	注意这里是以文本的形式读写的
*******************************************************************************************/
int wrconfile(const char *str,const char *filename)
{
        FILE *fp;
        fp=fopen(filename,"a+");
        if(fp==NULL)
        {
                DebugPrintf("\nFine doesn't exit!");
                exit(1);
        }
        fputs(str,fp);
        fputc('\n',fp);							//每次均换行
        fclose(fp);
}

char *Trim(char *ptr)
{
        //去空格
        char temp[30];
        int i,j,count;
        j=0;
        count=strlen(ptr);
        for(i=0;i<count;i++)
        {
                if(*(ptr+i)==' ')
                        continue;
                else
                        temp[j++]=*(ptr+i);
                temp[j]='\0';
        }
        strcpy(ptr, temp);
        return ptr;
}

/*******************************************************************************************
**函数名称：	GetKey(char *dst,char *outptr,char *filename)
**函数功能：	从文件中查找目标字符串
**入口参数：	char *dst 					目标字符串的标识字符串
 **         		char *outptr           			目标字符串
**          		*filename    				文件名
**返 回 值：	返回-1表明未能找到目标字符串，否则返回目标字符串的长度
**说    明：
*******************************************************************************************/
int GetKey(char *dst,char *outptr,char *filename)
{
        char buf1[30];
        FILE *fp;
        fp=fopen(filename,"r");
        if(fp==NULL)					//如果文件不存在，就创建一个默认的文件，并且写入默认的值
        {
                wrconfile("[ip]",IPConfigfile_Path);
                wrconfile(ipdz,IPConfigfile_Path);
                wrconfile("[zwym]",IPConfigfile_Path);
                wrconfile(zwym,IPConfigfile_Path);
                wrconfile("[mrwg]",IPConfigfile_Path);
                wrconfile(mrwg,IPConfigfile_Path);

                fp=fopen(filename,"r");
                 if(fp==NULL)
                {
                        DebugPrintf("\nopen configfile error");
                        fclose(fp);
                        exit(0);
                  }
        }
        rewind(fp);
        while(!feof(fp))					//判断文件是否结束
        {
                fscanf(fp,"%s",buf1);//%s 读入一个字符串，遇空格、制表符或换行符结束
                if(!strcmp(Trim(buf1),dst))	//extern int strcmp(char *s1,char * s2);
                                                                // 当s1<s2时，返回值<0;当s1=s2时，返回值=0;当s1>s2时，返回值>0
                {
                        fscanf(fp,"%s",outptr);
                        fclose(fp);
                        return strlen(outptr);
                }
        }
        fclose(fp);
        return -1;
}
/********************************************************************************************
*********************************************************************************************/

/*******************************************************************************************
**函数名称：	 read_optfile()
**函数功能：	读取网络配置文件中的网络信息
**入口参数：
**返 回 值：
**说    明：
*******************************************************************************************/
int read_optfile()
{
        if(read_at24c02b(30) == 11)
        {
                DebugPrintf("\n--- read eeprom init ip ---\n");
                sprintf(ipdz,"%d.%d.%d.%d",read_at24c02b(31),read_at24c02b(32),read_at24c02b(33),read_at24c02b(34));
                sprintf(zwym,"%d.%d.%d.%d",read_at24c02b(35),read_at24c02b(36),read_at24c02b(37),read_at24c02b(38));
                sprintf(mrwg,"%d.%d.%d.%d",read_at24c02b(39),read_at24c02b(40),read_at24c02b(41),read_at24c02b(42));
        }
}

/*******************************************************************************************
**函数名称：	 reconfig()
**函数功能：	重新写配置文件后
**入口参数：
**返 回 值：
**说    明：
*******************************************************************************************/
int reconfig()
{
        unsigned char t_ipdz[4] = {0};
        unsigned char t_zwym[4]  ={0};
        unsigned char t_mrwg[4] = {0};
        DebugPrintf("\n--- write eeprom save ip");
        fflush(stdout);
        Get_Netaddr(ipdz,t_ipdz);
        Get_Netaddr(zwym,t_zwym);
        Get_Netaddr(mrwg,t_mrwg);
        write_at24c02b(30,11);
        write_at24c02b(31,t_ipdz[0]);
        write_at24c02b(32,t_ipdz[1]);
        write_at24c02b(33,t_ipdz[2]);
        write_at24c02b(34,t_ipdz[3]);
        write_at24c02b(35,t_zwym[0]);
        write_at24c02b(36,t_zwym[1]);
        write_at24c02b(37,t_zwym[2]);
        write_at24c02b(38,t_zwym[3]);
        write_at24c02b(39,t_mrwg[0]);
        write_at24c02b(40,t_mrwg[1]);
        write_at24c02b(41,t_mrwg[2]);
        write_at24c02b(42,t_mrwg[3]);

}


int net_configure(void)   //返回0网络配置成功，返回-1，网络配置失败
{
        char buff[100] = {0};
        char tem_ipdz[16]={0};
        char tem_zwym[16]={0};
        char tem_mrwg[16]={0};
        int i;

        read_optfile();								//获取网络配置文件中的IP地址，子网掩码以及默认网关

        DebugPrintf("\n----%s\n    %s\n    %s\n", ipdz, zwym, mrwg);
        sprintf(buff,"ifconfig eth0 %s netmask %s",ipdz,zwym);
        if(system(buff) != 0)							//调用linux系统命令配置网络
                DebugPrintf("\nsystem(1) error");

        if(system("ifconfig eth0 down")!=0)				//禁用设备
                DebugPrintf("\nsystem(2) error");

        if(system("ifconfig eth0 up")!=0)				//激活设备
                DebugPrintf("\nsystem(3) error");

        sprintf(buff,"route add default gw %s",mrwg);		//添加默认网关
        if(system(buff)!=0)
                DebugPrintf("\nsystem(4) error");
        sleep(3);

        netcheckagain(tem_ipdz,tem_zwym,tem_mrwg);

        if((strcmp(ipdz,tem_ipdz)==0) && (strcmp(zwym,tem_zwym)==0) && (strcmp(mrwg,tem_mrwg)==0))
        {

                DebugPrintf("\n检测到网络,configure success!\n");
                for(i=0;i<5000000;i++);
                return 0;
        }
        else
        {
                DebugPrintf("\n请检查网络物理连接和网络设置,configure fail!\n");
                for(i=0;i<5000000;i++);
                return -1;
        }
}

void BmpFileSend(char * bmpfilename)
{
        int i;
        unsigned char transBuffer[WAVE_BUFF_LEN];
        unsigned char sendfilename[20];
        FILE *output = NULL;
        int freadcount = 0;
        #if NDEBUG
                DebugPrintf("\n------beginsendbmp = %d bmpfilename = %s", beginsendbmp, bmpfilename);
        #endif
        if(beginsendbmp)
        {
        if (sttConnSock[0].fdSock <= 0)
            return;
                transBuffer[0] = 0x00;
                transBuffer[1] = 0x01;
                transBuffer[2] = 0x05;
                /*memcpy(sendfilename,bmpfilename,17);
                memcpy(sendfilename,bmpfilename,18);
                transBuffer[3] = (sendfilename[5] - 48)*10 + sendfilename[6] - 48;
                transBuffer[4] = (sendfilename[7] - 48)*10 + sendfilename[8] - 48;
                transBuffer[5] = (sendfilename[9] - 48)*10 + sendfilename[10] - 48;
                transBuffer[6] = (sendfilename[11] - 48)*10 + sendfilename[12] - 48;
                transBuffer[7] = (sendfilename[13] - 48)*10 + sendfilename[14] - 48;
                transBuffer[8] = (sendfilename[15] - 48)*10 + sendfilename[16] - 48;
                */
                memcpy(sendfilename,bmpfilename,19);
                transBuffer[8] = (sendfilename[17] - 48)*10 + sendfilename[18] - 48;
                transBuffer[3] = (sendfilename[7] - 48)*10 + sendfilename[8] - 48;
                transBuffer[4] = (sendfilename[9] - 48)*10 + sendfilename[10] - 48;
                transBuffer[5] = (sendfilename[11] - 48)*10 + sendfilename[12] - 48;
                transBuffer[6] = (sendfilename[13] - 48)*10 + sendfilename[14] - 48;
                transBuffer[7] = (sendfilename[15] - 48)*10 + sendfilename[16] - 48;

                output = fopen (bmpfilename, "ab+");
                pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
                //DebugPrintf("\n--- begin send bmpfilename =  %s ---\n",bmpfilename);
                while(!feof(output))
                {
                        freadcount = fread(transBuffer+9, 1,1000, output);
                        //DebugPrintf("\n--- freadcount =  %d ---\n",freadcount);
                        for (i=0; i<MAX_LINK_SOCK; i++)
                        {

                                if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                                {
                                        //pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                                        SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, freadcount+9);
                                        //pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);

                                }
                        }
                }
                fclose (output);
                DebugPrintf("\n--- end send bmpfilename =  %s ---",bmpfilename);
                pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
                DelFile(bmpfilename);
        }

}

#define 	BASIC_DISP		800

extern int BASIC_LEVEL_;
extern int is_action;
extern unsigned char sys_Time[15];
int is_redict = 1; //0 需要重定向

void self_check(int fd_video1)
{
        int catch_sen_back = 0;
        int curmin_time = 0;
        int curhou_time = 0;
        unsigned char read_sys_Time[15];

        static int packet_count;
        static int check_ok;


        if (fd_video1 < 0)				//若fd_video<0说明打开video1不成功
                return;

        if (!check_ok)
                IsSetaction(fd_video1);


        memcpy(read_sys_Time, sys_Time, 15);
        curmin_time = (read_sys_Time[10] - 48)*10+(read_sys_Time[11] - 48);			//现在分钟数
        curhou_time = (read_sys_Time[8] - 48)*10+(read_sys_Time[9] - 48);			//现在小时数

        if (curhou_time == 2 && curmin_time == 20) {
                is_redict = 0;
                BASIC_LEVEL_ = BASIC_VALUE;
                return;
        }
                ////////////////////////

        if (is_action && !is_redict){			//已捕捉到动作并且需要重定向
                packet_count = 0;
                catch_sen_back = 0;
                is_action = 0;
                check_ok = 1;

                BASIC_LEVEL_ += BASIC_DISP;			//BASIC_DISP=800
                set_action(fd_video1, &catch_sen_back);
                DebugPrintf ("\n\n\n   MOTION LEVEL  %d \n\n\n", BASIC_LEVEL_);
        }
        else if (!is_redict) {					//未捕捉到动作，需要重定向
                packet_count++;
                DebugPrintf ("\n----- times = %d----------\n", packet_count);

                if (packet_count > 4) {				//当收到4个以上包的时候，需要降低基准灵敏度(相当于自调节)
                        if (BASIC_LEVEL_ > 2000)
                                BASIC_LEVEL_ -= 2000;
                        else
                                packet_count = 0;			//基准灵敏度已经最低
                        set_action(fd_video1, &catch_sen_back);
                }

                if (packet_count == 2)
                        set_action(fd_video1, &catch_sen);
                else if (packet_count == 4) {
                        is_redict = 1;
                        check_ok = 0;
                        //packet_count = 0;
                        DebugPrintf ("\n\n\n   FINAL  MOTION LEVEL  %d \n\n\n", BASIC_LEVEL_);
                        write_at24c02b(230, (BASIC_LEVEL_>>8)&0xFF);
                        write_at24c02b(231, BASIC_LEVEL_&0xFF);
                }

        }
}

#define		CHECK_ADDR		238
static void check_eeprom(void)
{
        write_at24c02b(CHECK_ADDR, 0x2D);
        if (read_at24c02b(CHECK_ADDR) != 0x2D)
                Err_Check.eeprom = 0xFF;
        else
                Err_Check.eeprom = 0x00;
    write_at24c02b(CHECK_ADDR, 0xB2);
}

static void check_flash(void)
{
        int fp = 0;
        int ch = 0x2F;

        if (islink() != 0)
        {
                Err_Check.flash = 0xFF;
                DebugPrintf("\n-------NAND FLASH ERR----------\n");
                return;
        }
        if ((fp = open("/mnt/err_check.txt", O_RDWR | O_CREAT, S_IWOTH)) < 0 ||
                                                                        (fp = open("/mnt/err_check.txt", O_RDWR | O_CREAT, S_IWOTH)) < 0) {
                Err_Check.flash = 0xFF;
                //DebugPrintf("\n-------1 NAND FLASH ERR----------\n");
        }

        else if ((write(fp, &ch, 1)) < 1 || (write(fp, &ch, 1)) < 1) {
                Err_Check.flash = 0xFF;
                //DebugPrintf("\n-------2 NAND FLASH ERR----------\n");
        }
        else if (!lseek(fp, 0, 0) && (read(fp, &ch, 1) < 1 || read(fp, &ch, 1) < 1)) {
                Err_Check.flash = 0xFF;
                //DebugPrintf("\n-------3 NAND FLASH ERR----------\n");
        }

        else if (ch != 0x2F)
                Err_Check.flash = 0xFF;
        else
                Err_Check.flash = 0x00;

        if (Err_Check.flash == 0xFF)
                DebugPrintf("\n-------NAND FLASH ERR----------\n");
        else
                DebugPrintf("\n-------NAND FLASH NO ERR----------\n");
        close(fp);
        system("rm /mnt/err_check.txt");
}

#define		TIME_DISP		60

static void check_rtc(void)
{
    char fmt[] = "%Y-%m-%d-%H-%M-%S";
        char strtime[30];
        struct tm time_now, time_pre;
        time_t curtime = 0;
        time_t pretime = 0;

        memset(strtime, '-', 30);
        strtime[0] = Err_Check.year_high/10 + 48;
        strtime[1] = Err_Check.year_high%10 + 48;
        strtime[2] = Err_Check.yead_low/10 + 48;
        strtime[3] = Err_Check.yead_low%10 + 48;

        strtime[5] = Err_Check.month/10 + 48;
        strtime[6] = Err_Check.month%10 + 48;

        strtime[8] = Err_Check.day/10 + 48;
        strtime[9] = Err_Check.day%10 + 48;

        strtime[11] = Err_Check.hour/10 + 48;
        strtime[12] = Err_Check.hour%10 + 48;

        strtime[14] = Err_Check.min/10 + 48;
        strtime[15] = Err_Check.min%10 + 48;

        strtime[17] = Err_Check.sec/10 + 48;
        strtime[18] = Err_Check.sec%10 + 48;
        strtime[19] = 0;
        DebugPrintf("\n--------time intend to check is %s------", strtime);
        strptime(strtime, fmt, &time_now);
        curtime = mktime(&time_now);

        memset(strtime, '-', 30);
        strtime[0] = Err_Check.time_now.tm_year/1000 + 48;
        strtime[1] = (Err_Check.time_now.tm_year%1000)/100 + 48;
        strtime[2] = (Err_Check.time_now.tm_year%100)/10 + 48;
        strtime[3] = Err_Check.time_now.tm_year%10 + 48;

        strtime[5] = Err_Check.time_now.tm_mon/10 + 48;
        strtime[6] = Err_Check.time_now.tm_mon%10 + 48;

        strtime[8] = Err_Check.time_now.tm_mday/10 + 48;
        strtime[9] = Err_Check.time_now.tm_mday%10 + 48;

        strtime[11] = Err_Check.time_now.tm_hour/10 + 48;
        strtime[12] = Err_Check.time_now.tm_hour%10 + 48;

        strtime[14] = Err_Check.time_now.tm_min/10 + 48;
        strtime[15] = Err_Check.time_now.tm_min%10 + 48;

        strtime[17] = Err_Check.time_now.tm_sec/10 + 48;
        strtime[18] = Err_Check.time_now.tm_sec%10 + 48;
        strtime[19] = 0;


        //DebugPrintf("\n--------time intend to check is %s------\n", strtime);
        strptime(strtime, fmt, &time_pre);
        pretime = mktime(&time_pre);
#if NDEBUG
        DebugPrintf("---\ncurtime = %ld,  pretime = % ld pretime -curtime = %d-----", curtime, pretime, pretime -curtime);
        DebugPrintf("\n\ndifftime(curtime, pretime) = %d", difftime(curtime, pretime));
#endif
        Err_Check.rtc = 0x00;
        if (pretime -curtime < TIME_DISP && pretime -curtime > -TIME_DISP){
            DebugPrintf("\n--------------RTC OK----------");
            return;
        }
        Err_Check.rtc = 0xFF;
        DebugPrintf("\n--------------RTC ERR----------");
}



static void board_check(unsigned char *transBuffer)
{
        int i;
        //DebugPrintf("\n------Err_Check.begincheck = %d-----\n", Err_Check.begincheck);
    if (Err_Check.begincheck && Err_Check.card_checked && (Err_Check.photo_checked || Err_Check.issavvideo || beginsendbmp == 0)) {
                        DebugPrintf("\n------begin to selfcheck----");
                        check_eeprom();
                        check_flash();
                        check_rtc();

                        DebugPrintf("\n---------begin send check information-------------");
                        transBuffer[0] = 0x00;
                        transBuffer[1] = 0x01;
                        transBuffer[2] = 0x08;
                        transBuffer[3] = Err_Check.flash;
                        transBuffer[4] = Err_Check.eeprom;
                        transBuffer[5] = Err_Check.rtc;
                        transBuffer[6] = Err_Check.card;
                        transBuffer[7] = Err_Check.photo;

                        for (i=0; i<MAX_LINK_SOCK; i++)
                                if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0) {
                                                pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                                                SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, 8);
                                                pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);
                        break;
                                        }
                        Err_Check.photo_checked = 0;
                        Err_Check.card_checked = 0;
                        Err_Check.begincheck = 0;
                        //DebugPrintf("\n----------check information sent ok----------------\n");
        }
}

unsigned int CRC_check(char bs[],int off,int len)
{
        int i = off,CRC_result=0;
        for(;i < off + len;i++)
        {
                CRC_result = (CRC_result<<8)^CRC_table[(((unsigned int)CRC_result>>8)^bs[i])&0xff];
        }
        return CRC_result;
}

static void card_sent(unsigned char *transBuffer)
{
    //unsigned char transBuffer[1000];
    int  i;
    datum key;
    char * cardrecordre = NULL;
    struct timespec tsp;
    long seconds;
    static long time_towait = 10;
    int ret = -10;
    static int resend_count = 0;
    seconds = time_towait;
    do {
            if(beginsendcard)
            {

                transBuffer[0] = 0x00;
                transBuffer[1] = 0x01;
                transBuffer[2] = 0x01;
                i = 0;
                                PrintScreen("\n----------start to send beginsendcar----\n");

                do
                {
                    pthread_mutex_lock(&cardfile_lock);
                    key = gdbm_firstkey(gdbm_card);					//get a record
                    pthread_mutex_unlock(&cardfile_lock);

                    cardrecordre = key.dptr;
                    //cardsendcount++;
                    if (cardrecordre == NULL) {
                        break;
                    }
                    else if(cardrecordre != NULL)
                    {
                                                if(strlen(cardrecordre)<38)		//record at least 38 bytes,delete illegal records
                                                {
#if DEBUG_DATA
                                                        DebugPrintf("\nthis record is illegal!");
#endif
                                                        pthread_mutex_lock(&cardfile_lock);
                                                        gdbm_delete(gdbm_card, key);
                                                        pthread_mutex_unlock(&cardfile_lock);
                                                        break;
                                                }
                        memcpy(transBuffer+3, cardrecordre, strlen(cardrecordre));
                        DebugPrintf("\n-----cardrecordre = %s -----",cardrecordre);

                        if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                        {
                            DebugPrintf("\n-----cardsnr_send-----");
                            memset(card_record_check, 0, 50);
                            strncpy(card_record_check, cardrecordre, strlen(cardrecordre));
                            //DebugPrintf("\n----card_record_check = %s----\n", card_record_check);
                            pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                            SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, strlen(cardrecordre)+3);
                            pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);

                            maketimeout(&tsp, seconds);

                            pthread_mutex_lock(&cardsend_lock);
                            ret = pthread_cond_timedwait(&cardsend_cond, &cardsend_lock, &tsp);
                            pthread_mutex_unlock(&cardsend_lock);

                            if (0 == ret) {
                                pthread_mutex_lock(&cardfile_lock);
                                gdbm_delete(gdbm_card, key);
                                //system("cp /tmp/cards.xml /mnt/cards.xml");
                                pthread_mutex_unlock(&cardfile_lock);
                                free(cardrecordre);
                                resend_count = 0;
                            }
                            else {
                                                                if (ret == ETIMEDOUT) {
                                                                        if (time_towait < 20)
                                                                                //time_towait++;
                                                                                time_towait = 20;
                                                                        DebugPrintf("\n-------------card send repley timeout time wait = %d-------", time_towait);
                                                                        resend_count++;
                                                                }
                                if (resend_count > 20) {			//if the time of sending this record has passed 20,then delete it
                                                                        pthread_mutex_lock(&cardfile_lock);
                                                                        gdbm_delete(gdbm_card, key);
                                                                        //system("cp /tmp/cards.xml /mnt/cards.xml");
                                                                        pthread_mutex_unlock(&cardfile_lock);
                                                                        resend_count = 0;
                                                                }
                                beginsendcard = 0;
                                free(cardrecordre);
                                return;
                            }
                        }
                        else
                        {
                            //pthread_mutex_lock(&cardfile_lock);
                            //system("cp /tmp/cards.xml /mnt/cards.xml");
                            //pthread_mutex_unlock(&cardfile_lock);
                            DebugPrintf("\n-----net_error-----");
                            free(cardrecordre);
                            beginsendcard = 0;
                            resend_count = 0;
                            break;
                        }
                    }
                }while (1);
                beginsendcard = 0;
            }

       } while (0);
}

static void check_ordertime(unsigned long cur_cardsnr,unsigned char *cardrecordwr,unsigned char *card_time,unsigned char *read_sys_Time)
{
        datum key,key_order,data_order,data;
        unsigned char user_temp[50];
        unsigned char order_starttime[15] = {0};		//预约起始时间
        unsigned char order_endtime[15] = {0};		//预约结束时间
        unsigned long int order_start = 0;
        unsigned long int order_end = 0;
        unsigned long int sys_iTime = atoll(sys_Time);

        ///////////////////////////////////
        //遍历预约时间数据库///////////////
        //pthread_mutex_lock(&ordertime_lock);
        DebugPrintf("\n----------------half open mode!!--------------------\n");
        PrintScreen("\n----------------half open mode!!--------------------\n");
        if(gdbm_ordertime==NULL) {
                DebugPrintf("\n---------gdbm_ordertime = %d------------\n",gdbm_ordertime);
                gdbm_ordertime = db_open("/tmp/ordertime.xml");
        }

        for(key_order = gdbm_firstkey(gdbm_ordertime);key_order.dptr;)	//遍历数据库所有的key并与给定卡号比较
        {
                unsigned char tmp_cmp[9],count = 0;

                sprintf(user_temp, "%08lX",cur_cardsnr);		//将当前刷卡号保存到user_temp中

                memcpy(tmp_cmp,key_order.dptr,8);
                tmp_cmp[8] = 0;
                DebugPrintf("\n-------tmp_cmp = %s--------\n",tmp_cmp);
                DebugPrintf("\n-------user_temp = %s--------\n",user_temp);
                if(strcmp(tmp_cmp,user_temp)){					//如果用户和数据库不一致则继续循环
                        DebugPrintf("\n---------It's other's ordertime!----------------");
                        PrintScreen("\n---------It's other's ordertime!----------------");
                        key_order = gdbm_nextkey(gdbm_ordertime,key_order);
                        continue;
                }
                else{
                        DebugPrintf("\n----------Led_on = %d---------------",Led_on);
                        if( read_at24c02b(46) == 1 )				//如果用户想下机一定让他下
                        {
                                Led_delay = 1;

                                DebugPrintf("\n------------normal user is to get off!---------------");
                                sprintf(cardrecordwr, "%.14s_%.14s_%08lX", card_time, read_sys_Time,cur_cardsnr);		//记录刷卡信息
                                DebugPrintf("\n%.14s_%.14s_%08lX\n", card_time, read_sys_Time,cur_cardsnr);
#if NDEBUG
                                DebugPrintf("\n-----normal card cardrecordwr = %s-------", cardrecordwr);
#endif
                                key.dptr = cardrecordwr;
                                key.dsize = strlen(cardrecordwr) + 1;
                                data = key;

                                //pthread_mutex_lock(&cardfile_lock);
                                if (db_store(gdbm_card, key, data) < 0) {		//保存读卡记录
                                        DebugPrintf("\n--------normal store cardrecordwe err ------");
                                }
                                else{
                                        DebugPrintf("\n-----normal user forced data store success-----");
                                        beginsendcard = 1;
                                        db_close(gdbm_ordertime);
                                        gdbm_ordertime = NULL;
                                        break;
                                }
                        }
                        DebugPrintf("\n---------get order time!-------------");
                        data = gdbm_fetch(gdbm_ordertime,key_order);		//得到key的内容，也就预约时间

                        DebugPrintf("\n-----------data.dptr = %s----------------",data_order.dptr);

                        strncpy(order_starttime,data.dptr+3,11);
                        DebugPrintf("\n-----------order starttime = %s----------------",order_starttime);
                        order_start = atoll(order_starttime);		//得到预约起始时间
                        DebugPrintf("\n-----------order start = %ld----------------",order_start);
                        strncpy(order_endtime,data.dptr+18,11);
                        DebugPrintf("\n-----------order endtime = %s----------------",order_endtime);
                        order_end = atoll(order_endtime);			//得到预约结束时间
                        DebugPrintf("\n-----------order end = %ld----------------",order_end);

                        DebugPrintf("\n-----------sys_Time = %s------------",sys_Time);

                        sys_iTime = atoll(sys_Time+3);					//得到当前时间
                        DebugPrintf("\n----------now is %ld----------------",sys_iTime);
                        if((sys_iTime>=order_start)&&(sys_iTime<=order_end)){		//在预约时间内
                                if(arrive_card == 2){									//若未验证后的非机组卡
                                }
                                else
                                Led_delay = 1;

                                sprintf(cardrecordwr, "%.14s_%.14s_%08lX", card_time, read_sys_Time,cur_cardsnr);		//记录刷卡信息
                                DebugPrintf("\n%.14s_%.14s_%08lX", card_time, read_sys_Time,cur_cardsnr);
#if NDEBUG
                                DebugPrintf("\n-----normal card cardrecordwr = %s-------", cardrecordwr);
#endif
                                key.dptr = cardrecordwr;
                                key.dsize = strlen(cardrecordwr) + 1;
                                data = key;

                                //pthread_mutex_lock(&cardfile_lock);
                                if (db_store(gdbm_card, key, data) < 0) {		//保存读卡记录
                                        DebugPrintf("\n--------normal store cardrecordwe err ------");
                                }
                                else{
                                        DebugPrintf("\n-----normal user forced data store success-----");
                                        beginsendcard = 1;
                                        db_close(gdbm_ordertime);
                                        gdbm_ordertime = NULL;
                                        break;
                                }
                                //pthread_mutex_unlock(&cardfile_lock);
                                //pthread_mutex_unlock(&ordertime_lock);
                                break;											//跳出检索数据库循环
                        }
                        else if(sys_iTime>order_end){
                                DebugPrintf("\n--------It's out of order time-----------------------");
                                PrintScreen("\n--------It's out of order time-----------------------");
                                //pthread_mutex_lock(&cardfile_lock);
                                DebugPrintf("\n---------------key.dptr = %s------------",key.dptr);
                                gdbm_delete(gdbm_ordertime,key_order);
                                //system("cp /tmp/ordertime.xml /tmp/ordertime_bak.xml");

                                system("cp /tmp/ordertime.xml /mnt/ordertime.xml");						//备份更新过的数据库

                                key_order = gdbm_firstkey(gdbm_ordertime);
                                DebugPrintf("\n------------next key.dptr = %s----------",key_order.dptr);
                                continue;
                                //pthread_mutex_unlock(&cardfile_lock);
                        }
                        else {
                                DebugPrintf("\n--------It's not order time!----------------------");
                                PrintScreen("\n--------It's not order time!----------------------");
                        }
                }
                key_order = gdbm_nextkey(gdbm_ordertime,key_order);
                DebugPrintf("\n------------next key.dptr = %s----------",key_order.dptr);
        }
        if(gdbm_ordertime != NULL) {
                db_close(gdbm_ordertime);
                gdbm_ordertime = NULL;
        }
        //pthread_mutex_unlock(&ordertime_lock);
}


#define		RANDOM_MAX		1000

 void* WavePacketSend(void *arg)
{
        int i, waveLen;
        int rand_value = 0;
        unsigned char transBuffer[WAVE_BUFF_LEN];
        unsigned char sendfilename[30];
        unsigned char openfilename[30];
    //unsigned char cur_htime[20];
   // unsigned char pre_htime[20];
    //int curhour = 0;
   // int prehour = 0;
    unsigned char file_del[50] = "rm /mnt/work/";
        FILE *output = NULL;
        int freadcount = 0;
        DIR   *   dir;
        struct   dirent   *   ptr;
    beginsyncbmp = 1;
    sleep(5);

    //startsyncbmp = 1;

        //srand((unsigned) time(NULL));

        while (1)
        {
        /////////////////////////////////////////////////

                self_check(action_fd);
        if (!is_redict) {					//启动线程时先得设定灵敏度
                        sleep(1);
            continue;
        }
                /*********************** err check send *******************************/
                board_check(transBuffer);
                /************************************************************************/

        /************************* card send ********************************/
        card_sent(transBuffer);
        /***********************************************************************/

                while(0)
        //while(startsyncbmp) // 同步图片使能
                {
            if (sttConnSock[0].fdSock <= 0)
                break;
            else
                DebugPrintf("\n-------begin to syncbmp--------");

                        //rand_value = rand() % (RANDOM_MAX + 1);
            //DebugPrintf( "\n-----begin  d_name:   %s-----\n ",   syncbeginFname);
            //DebugPrintf( "\n-----end  d_name:   %s-----\n ",   syncendFname);
                        dir   =opendir( "/mnt/work/");
            while((ptr = readdir(dir)) != NULL && sttConnSock[0].fdSock > 0)
                        {
                //if(strcmp(ptr-> d_name,syncbeginFname) >= 0 && strcmp(ptr-> d_name,syncendFname) <= 0)
                if (strstr(ptr-> d_name,".jpg") != NULL)
                {
#if DEBUG_DATA
                    DebugPrintf( "\nd_name:   %s\n ",   ptr-> d_name);
#endif
                                        transBuffer[0] = 0x00;
                                        transBuffer[1] = 0x01;
                                        transBuffer[2] = 0x06;
                                        //memcpy(sendfilename,ptr-> d_name,12);  //去掉".bmp"
                                        memcpy(sendfilename,ptr-> d_name,14);
                                        /*
                                        transBuffer[3] = (sendfilename[0] - 48)*10 + sendfilename[1] - 48;//字符串文件名201011130953转化为字节发送20 10 11 13 09 53
                                        transBuffer[4] = (sendfilename[2] - 48)*10 + sendfilename[3] - 48;
                                        transBuffer[5] = (sendfilename[4] - 48)*10 + sendfilename[5] - 48;
                                        transBuffer[6] = (sendfilename[6] - 48)*10 + sendfilename[7] - 48;
                                        transBuffer[7] = (sendfilename[8] - 48)*10 + sendfilename[9] - 48;
                                        transBuffer[8] = (sendfilename[10] - 48)*10 + sendfilename[11] - 48;
                                        */
                                        transBuffer[8] = (sendfilename[12] - 48)*10 + sendfilename[13] - 48;//字符串文件名20101113095327转化为字节发送 10 11 13 09 53 27
                                        transBuffer[3] = (sendfilename[2] - 48)*10 + sendfilename[3] - 48;
                                        transBuffer[4] = (sendfilename[4] - 48)*10 + sendfilename[5] - 48;
                                        transBuffer[5] = (sendfilename[6] - 48)*10 + sendfilename[7] - 48;
                                        transBuffer[6] = (sendfilename[8] - 48)*10 + sendfilename[9] - 48;
                                        transBuffer[7] = (sendfilename[10] - 48)*10 + sendfilename[11] - 48;

                                        sprintf(openfilename, "/mnt/work/%s",ptr-> d_name);
                                        output = fopen (openfilename, "ab+");
                                        pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
                                        DebugPrintf("\n--- begin send bmpfilename =  %s ---",ptr-> d_name);
                                        while(!feof(output))
                                        {
                                                freadcount = fread(transBuffer+9, 1,1000, output);
                                                //DebugPrintf("\n--- freadcount =  %d ---\n",freadcount);
                                                for (i=0; i<MAX_LINK_SOCK; i++)
                                                {

                                                        if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                                                        {
                                                                //pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                                                                SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, freadcount+9);
                                                                //pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);

                                                        }
                                                }
                                        }
                                        fclose (output);
                                        DebugPrintf("\n--- end send bmpfilename =  %s ---",ptr-> d_name);
                                        pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
                    strcat(file_del, ptr-> d_name);
                    system(file_del);
                    memcpy(file_del, "rm /mnt/work/", strlen("rm /mnt/work/") + 1);
                                        //usleep(rand_value*1000);
                                }
                        }
                        closedir(dir);
            startsyncbmp = 0;
                }

        sleep(1);




        ////////////////////////////////////////////////////////////////////////////

        }
}

static void check_card(int cur_cardsnr)
{
        int count;

        if (Err_Check.begincheck && Err_Check.card_checked == 0)
                for (count = 0; count < 1; count++) {
                        Err_Check.card = 0x00;
                        cur_cardsnr = CardRead();
                        if(cur_cardsnr < 0 ) {
                                if (cur_cardsnr == -2) {
                                        close_card_uart();
                                        init_card_uart();
                                        #if NDEBUG
                                                DebugPrintf("\n-------card err------\n");
                                        #endif
                                        Err_Check.card = 0xFF;
                                }
                        }
                        Err_Check.card_checked = 1;
        }

}

static void maketimeout(struct timespec *tsp, long seconds)
{
    struct timeval now;
    gettimeofday(&now);
    tsp->tv_sec = now.tv_sec;
    tsp->tv_nsec = now.tv_usec * 1000;

    tsp->tv_sec += seconds;
}

static void check_unknown(unsigned char *cardrecordre)
{
        unsigned char transBuffer[50];
        struct timespec tsp;
        long time_out;
        static long time_towait = 10;
        int ret = -10;
                if (sttConnSock[0].fdSock <= 0)
                        return;

                transBuffer[0] = 0x00;
                transBuffer[1] = 0x02;
                transBuffer[2] = 0x10;

                //memcpy(card_to_check, cardrecordre, strlen(cardrecordre));
                strncpy(card_to_check, cardrecordre, 9);

#if DEBUG_DATA
                DebugPrintf("\n--------card_to_check = %s--------", card_to_check);
#endif

                time_out = time_towait;
                if(cardrecordre != NULL) {
                        memcpy(transBuffer+3, cardrecordre, strlen(cardrecordre));
                        DebugPrintf("\n-----cardrecordre = %s -----",cardrecordre);
                        if (sttConnSock[0].fdSock>0 && sttConnSock[0].loginLegal>0) {
                pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
                                SockPackSend(CMD_ACK, sttConnSock[0].fdSock, &sttConnSock[0], transBuffer, strlen(cardrecordre)+3);
                pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
                maketimeout(&tsp, time_out);

                pthread_mutex_lock(&cardsnd_lock);
                ret = pthread_cond_timedwait(&cardsnd_cond, &cardsnd_lock, &tsp);		//等待服务器响应，返回0则成功响应
                pthread_mutex_unlock(&cardsnd_lock);

                                DebugPrintf("\n-------------ret= %d--------------",ret);
                if (0 == ret) {
                        DebugPrintf("\n-----arrive_flag = %d----", arrive_flag);
                        arrive_card = arrive_flag;
                }
                else if (ret == ETIMEDOUT) {			//超时
                                        if (time_towait < 12)
                                                time_towait++;
                                        DebugPrintf("\n---unknown card check timeout = %d-----", time_towait);
                }

                DebugPrintf("\n-----unknown card send-----");
                        }
                }

}

//static char card_to_send[50];
//static int begin_query;
//static int cardquery_lock = PTHREAD_MUTEX_INITIALIZER;
//static int cardquery_cond = PTHREAD_COND_INITIALIZER;

/**********************发送卡号给服务器(作读卡器使用)***************************************************************/
static int send_check(unsigned char *cardrecordre)
{

    unsigned char transBuffer[50];
    struct timespec tsp;
    long time_out = 15;
    int ret = -10;
    if (sttConnSock[0].fdSock <= 0)
        return -1;

    transBuffer[0] = 0x00;		//包头
    transBuffer[1] = 0x02;
    transBuffer[2] = 0x11;		//查询字0x11

    strncpy(card_to_send, cardrecordre, 9);		//将刷卡记录保存在card_to_send中

    if(cardrecordre != NULL) {					//若cardrecordre有数据
        memcpy(transBuffer+3, cardrecordre, strlen(cardrecordre));		//则将cardrecordre保存接在发送
        DebugPrintf("\n-----cardrecordre = %s -----",cardrecordre);
        if (sttConnSock[0].fdSock>0 && sttConnSock[0].loginLegal>0) {	//网络通且登录合法
            pthread_mutex_lock(&sttConnSock[0].lockBuffOut);			//发送缓冲区锁定
            SockPackSend(CMD_ACK, sttConnSock[0].fdSock, &sttConnSock[0], transBuffer, strlen(cardrecordre)+3);		//发送卡号给服务器
            pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);			//解锁锁定
            maketimeout(&tsp, time_out);				//超时时间为15秒

            pthread_mutex_lock(&cardquery_lock);
            ret = pthread_cond_timedwait(&cardquery_cond, &cardquery_lock, &tsp);	//等待条件cardquery_cond，15秒不响应则超时
            pthread_mutex_unlock(&cardquery_lock);									//即等待服务器发送设置字0x11

            if (0 == ret) {
                return 0;
            }
            else if (ret == ETIMEDOUT) {

                return -1;
            }
        }
    }
    return -1;
}

#define     TIME_FIRST_WAIT     375			//刷卡前等待时间
#define     TIME_WAIT           150			//刷卡后等待时间
#define     SLEEP_TIME          800000		//休眠时间，等待时间的基数

static  void sync_card()
{
    unsigned long cur_cardsnr = 0;
    int card_sendcount, time_wait = TIME_FIRST_WAIT;
    char card[50];

    int time_count = 0;
    datum data;
    datum key;

    card_beep(10);			//进入读卡状态，蜂鸣器叫4下
    card_beep(10);
    card_beep(10);
    card_beep(10);

    memset(card, 0, 50);	//清空暂存卡号

    while (time_count < time_wait) {
        usleep(SLEEP_TIME);			//休眠0.8s
        time_count++;
        cur_cardsnr = CardRead();	//读卡

        if (sttConnSock[0].fdSock <= 0)		//如果网络未连接则返回!!!!
            return;
        if (cur_cardsnr > 1 && cur_cardsnr != -1 && cur_cardsnr != -2) {	//读到卡号
            sprintf(card, "%08lX", cur_cardsnr);							//将卡号保存到card
            time_wait = TIME_WAIT;
            card_sendcount = 0;		//发送次数
            time_count = 0;

            do {							//让终端一直处于发送状态，直到服务器有应答
                if (0 == send_check(card)) {								//发送卡号，返回0表示服务器有响应，返回-1表示超时
                    if (!card_ack)			//判断服务器状态，card_ack为1:服务器状态正确，为0:服务器出错(出错需要服务器再次发送命令，让终端进入状态)
                        return;
                    card_beep(50);			//响两声表示服务器读到卡号，状态正确
                    card_beep(50);
                    key.dptr = card;
                    key.dsize = strlen(card) + 1;
                    data = key;
                    db_store(gdbm_user, key, data);		//将卡号保存到数据库内
                    sleep(2);
                    return;
                }
               else
                    card_sendcount++;
                if (card_sendcount == 3)	//发送3次服务器都没响应的话则返回
                    return;
            }while (1);
        }
    }


}

#define FREQ_HIGH 400000
#define FREQ_LOW  100000
#define CARD_LIMIT	5
#define ADDR_BEGIN      60





void* CardPacketSend(void *arg)         //查询参数
{
    static unsigned long pre_cardsnr = 0;		//存放之前一次所刷8位卡号
    static unsigned long cur_cardsnr = 0;		//存放8位卡号的数值
    static time_t pre_ctime = 0;
    static time_t cur_ctime = 0;
    int pre_devicestate = 0;
    int cur_devicestate = 0;
    unsigned char read_sys_Time[15];		//保存系统时间
    unsigned char cardrecordwr[50];			//读卡记录
    unsigned char super_card[50];			//保存9字节超级卡号
    unsigned char normal_card[50];			//保存9字节非机组卡信息
    unsigned char devicerecordwr[50];		//设备记录
    unsigned char user_temp[50];
    unsigned char cur_card[50] = {0};		//现在使用设备的卡号
    unsigned char card_time[50] = {0};		//刷卡时间
    unsigned char this_card[50] = {0};		//当前8字节卡号
    unsigned char *cardrecordre;
    unsigned char *devicerecordre;
    unsigned char *snrnumrecordre;
    unsigned char *terminalstatesre;
    int devicedelfile = 0;
    int terminalstatesdelfile = 0;
    long int cardcount = 0;
    long int devicecount = 0;
    int next1;
    int i;
    unsigned char transBuffer[WAVE_BUFF_LEN];
    int need_delay = 0;
        datum key_order,data_order;
    datum key;
    datum data;

    //Init_Webserver_Par();
    int CardPacketSend_count = 0;
    //system("rm -rf /tmp/*.xml");
    //system("cp -rf /mnt/*.xml /tmp/");

    key.dptr = "user_version";
    key.dsize = strlen("user_version") + 1;
    data = key;

    gdbm_user = db_open("/tmp/user.xml");				//打开用户数据库
    if (gdbm_user == NULL) {							//如果返回NULL则打开失败
                user_version = 0;
        system("rm /tmp/user.xml");						//删除用户数据库后为什么还要打开(将之前的数据删除，创建新的数据库，用户更新)
        gdbm_user = db_open("/tmp/user.xml");
        if (gdbm_user == NULL)
                DebugPrintf("\n----err---");
        if (db_store(gdbm_user, key, data) < 0) {
            DebugPrintf("\n-----there is no user.xml open err store-----\n");
        }
        DebugPrintf("\n-----user.xml open err-----\n");
    }
    else {
        data = gdbm_fetch(gdbm_user, key);				//取队列第一个key
        if (data.dptr == NULL) {						//第一个key为空
                        db_close(gdbm_user);						//则删除数据库，新建数据库
            system("rm /tmp/user.xml");
            gdbm_user = db_open("/tmp/user.xml");
                        user_version = 0;
                        data = key;
            if (db_store(gdbm_user, key, data) < 0) {	//初始化数据库头
                DebugPrintf("\n-----there is no user.xml open err-----\n");
            }
            else
                                DebugPrintf("\n----user xml now store success-----\n");
           // system("cp /tmp/user.xml /mnt/user.xml");
        }
        else {
            free(data.dptr);
#if NDEBUG
            DebugPrintf("\n-----open user.xml successful------\n");
#endif
        }
    }
    gdbm_device = db_open("/tmp/devices.xml");			//打开设备数据库

    if (gdbm_device == NULL) {
        system("rm /tmp/devices.xml");
        gdbm_user = db_open("/tmp/devices.xml");
        DebugPrintf("\n-----device.xml open err-----\n");
    }

    gdbm_card = db_open("/tmp/cards.xml");			//打开读卡数据库
    if (gdbm_card == NULL) {
        system("rm /tmp/devices.xml");
        gdbm_card = db_open("/tmp/cards.xml");
        DebugPrintf("\n-----cards.xml open err-----\n");
    }

        gdbm_ordertime = db_open("/tmp/ordertime.xml");		//打开预约时间数据库
        if (gdbm_ordertime == NULL) {
                system("rm /tmp/ordertime.xml");
                gdbm_ordertime = db_open("/tmp/ordertime.xml");
                DebugPrintf("\n-----ordertime.xml open err-----\n");
        }

    for (i = 0; i <  8; i++) {						//读取当前卡号
        cur_card[i] = read_at24c02b(i+ADDR_BEGIN);
    }

    for (i = 0; i < 14; i++) {						//读取当前刷卡时间
        card_time[i] =  read_at24c02b(ADDR_BEGIN+10+i);
    }
    beginsendcard = 1;								//刚启动的时候将保存的刷卡信息发送给服务器

#if DEBUG_DATA
    for (i = 0; i <  8; i++) {
        DebugPrintf("\ncur_card=%s", cur_card);
    }
    for (i = 0; i < 14; i++) {
        DebugPrintf("\ncard_time=%s", card_time);
    }
#endif
    //cur_card[0] = 0;
    cur_card[i] = 0;							//卡号补0，但是i=14,卡号只有8位，在哪补零????

    while(1)
    {
        cardcount++;

           // DebugPrintf("\n---------cardcount = %ld-----\n", cardcount);
        /******************************服务器增加卡*************************/
        if (begin_query) {
            sync_card();
            begin_query = 0;
        }
        /********************************************************************/
        if(cardcount == FREQ_HIGH) // 读卡频率
        {
            cardcount = 0;
            if(!beginupload)
            cur_cardsnr = CardRead();		//读取卡号

                        if ((cur_cardsnr == -2)&&!beginupload) {		//需要重启读卡器
                                        close_card_uart();
                                        init_card_uart();
                        }

            if(cur_cardsnr == 1)			//什么时候会为1(老版本的dc_reset失败时返回1，目前版本没用)
            {
                devicecount += 5; 			//避免读卡器故障时电流检测周期过长
            }
            check_card(cur_cardsnr);		//检测读卡器工作是否正常

            if(cur_cardsnr == 0)			//寻卡失败，则将前一次卡号置0
            {
                pre_cardsnr = 0;
            }
            do {
                if(pre_cardsnr == 0 && cur_cardsnr > 1 && cur_cardsnr != -1 && cur_cardsnr != -2)	//get a new card
                {
                    cur_ctime = time(NULL);
                    ReadSysTime();
                    memcpy(read_sys_Time, sys_Time, 15);

                                        if (cur_ctime < pre_ctime - 5)							//如果当前时间比之前刷卡时间还早5秒以上，说明之前刷卡时间有问题
                                                pre_ctime = cur_ctime - card_tlimit + 1;			//将之前刷卡时间更新为当前时间-card_tlimit+1

                    if (need_delay) {			//刷卡延时(就是说刷了一张卡后延时30s，在这段时间都不可刷卡，直接跳出循环)
                                                if (cur_ctime - pre_ctime < card_tlimit) {
                                                        PrintScreen("---limit = %ds  ----------------\n", card_tlimit);
                                                        break;
                                                }
                                        }
                                        else {						//后一个人和前一个刷卡时间大于5s
                                                if (cur_ctime - pre_ctime < CARD_LIMIT) {
#if NDEBUG
                                                        DebugPrintf("\n---limit = %ds  ----------------", CARD_LIMIT);
#endif
                                                        break;
                                                }
                                        }
                    sprintf(this_card, "%08lX", cur_cardsnr);		//写入卡号到this_card
                    DebugPrintf("\n-------this_card = %s----", this_card);

/******************超级卡********************************************/
                    sprintf(super_card, "S%08lX", cur_cardsnr);		//在超级卡前加S标示，假设为超级卡
                        key.dptr = super_card;
                                        key.dsize = strlen(super_card) + 1;
                                        data = key;

                                        if (gdbm_exists(gdbm_user, key) != 0) {			//判断用户是否在本地数据库,存在用户则进入
#if NDEBUG
                    DebugPrintf("\n----obtain a super card super_card = %s, cardrecordwr = %s-------", super_card, cardrecordwr);
                    PrintScreen("\n----obtain a super card super_card = %s, cardrecordwr = %s-------", super_card, cardrecordwr);
#endif

                                                if (0 != cur_card[0]) {						//判断首位是否为0检测当前是否有人使用设备，有人使用进入if语句内让他下机，没人使用则跳过
                                                        Led_delay = 1;							//第一次读到物理卡信息需延时30s,置延时标记位为1
                            sprintf(cardrecordwr, "%.14s_%.14s_", card_time, read_sys_Time);		//将系统时间写入cardrecordwr
#if NDEBUG
                                                        DebugPrintf("\n----obtain a super card super_card = %s, cardrecordwr = %s-------", super_card, cardrecordwr);
                                                        PrintScreen("\n----obtain a super card super_card = %s, cardrecordwr = %s-------", super_card, cardrecordwr);
#endif
                                                        strncat(cardrecordwr, cur_card, 8);		//将当前登录卡号添加到结尾处

#if NDEBUG
                                                        DebugPrintf("\n-----super card cardrecordwr = %s-------", cardrecordwr);
                                                        PrintScreen("\n-----super card cardrecordwr = %s-------", cardrecordwr);
#endif
                                                        key.dptr = cardrecordwr;
                                                        key.dsize = strlen(cardrecordwr) + 1;

                                                        data = key;
                            pthread_mutex_lock(&cardfile_lock);
                                                        if (db_store(gdbm_card, key, data) < 0) {		//保存读卡记录
                                                                DebugPrintf("\n--------super store cardrecordwe err ------");
                                                                pthread_mutex_unlock(&cardfile_lock);
                                                                break;
                                                        }
                                                        else {
#if NDEBUG
                                                        DebugPrintf("\n---super user forced data store success ---");
#endif
                                                        }
                            pthread_mutex_unlock(&cardfile_lock);
                                                        beginsendcard = 1;
                                                        pre_cardsnr = cur_cardsnr;
                            need_delay = 0;
                            pre_ctime = cur_ctime;
                                                        break;
                                                }
                    }
/***************非机组卡**********************************************/
                    sprintf(normal_card, "N%08lX", cur_cardsnr);		//在卡号前加N标示，检测是否为非机组卡
                        key.dptr = normal_card;
                                        key.dsize = strlen(normal_card)+1;
                                        data = key;

                                        system("cp /tmp/cards.xml /mnt");
                                        if (gdbm_exists(gdbm_user, key) != 0) {			//判断用户是否在非机组数据库,存在用户则进入
#if NDEBUG
                            DebugPrintf("\n----obtain a normal card normal_card = %s, cardrecordwr = %s-------", normal_card, cardrecordwr);
                            PrintScreen("\n----obtain a normal card normal_card = %s, cardrecordwr = %s-------", normal_card, cardrecordwr);
#endif

                                                ////////////////////////////////////////
                                                //记录刷卡信息//////////////////////////
                            if (cur_card[0] == 0) {						//如果当前没有人使用设备
                                sprintf(cardrecordwr, "%.14s_%.14s_%08lX", read_sys_Time, read_sys_Time, cur_cardsnr);		//更新刷卡信息
                                DebugPrintf("\n--------cardrecordwr when turn on = %s------------",cardrecordwr);
                                sprintf(card_time, "%.14s_", read_sys_Time);
                                for (i = 0; i < 14; i++)				//写入读卡时间到24c02b
                                    write_at24c02b(ADDR_BEGIN+10+i, card_time[i]);
                            }
                            else {										//当前设备有人使用
                                sprintf(cardrecordwr, "%.14s_%.14s_", card_time, read_sys_Time);
                                                        strncat(cardrecordwr,cur_card,8);		//将当前使用卡添加到记录中，因为要返回与上机者相同的卡号
                            }
                                                if(cur_card[0]!=0){						//判断是否有人上机
                                                        if(strcmp(cur_card,this_card))		//对于非机组用户来说，如果正在上机的不是自己的话则跳出
                                                        {
                                                                break;
                                                        }
                                                }

                                                if(device_mode == 0x01){					//设备为全开放模式
                                                        ////////////////////////////////////
                                                        //没有人上机或上机的人是自己/////////////
                                                        DebugPrintf("\n----------------open mode!!--------------------");
                                                        Led_delay = 1;
                                sprintf(cardrecordwr, "%.14s_%.14s_%08lX", card_time, read_sys_Time,cur_cardsnr);		//记录刷卡信息
#if NDEBUG
                                                        DebugPrintf("\n-----normal card cardrecordwr = %s-------", cardrecordwr);
#endif
                                                        key.dptr = cardrecordwr;
                                                        key.dsize = strlen(cardrecordwr) + 1;
                                                        data = key;

                               // pthread_mutex_lock(&cardfile_lock);
                                                        if (db_store(gdbm_card, key, data) < 0) {		//保存读卡记录
                                                                DebugPrintf("\n--------normal store cardrecordwe err ------");
                                                                //pthread_mutex_unlock(&cardfile_lock);
                                                                break;
                                                        }
                                                        else{
                                                                DebugPrintf("\n-----normal user forced data store success-----");
                                                                key = gdbm_firstkey(gdbm_card);
                                                                DebugPrintf("\n---------key.dptr = %s-----------",key.dptr);
                                                        }
                                                        //pthread_mutex_unlock(&cardfile_lock);
                                                        beginsendcard = 1;
                                                }
                                                else{										//设备为半开放模式
                                                        check_ordertime(cur_cardsnr,cardrecordwr,card_time,read_sys_Time);
                                                }
                                                need_delay = 0;
                                                pre_cardsnr = cur_cardsnr;
                        pre_ctime = cur_ctime;
                                                break;										//只要判断是非机组用户必然需要跳出
                            }
/***************机组卡**********************************************/
                        key.dptr = this_card;
                                        key.dsize = sizeof(this_card);
                                        data = key;
#if NDEBUG
                    DebugPrintf("\n---limit = %d  cur_card = %s   this_card = %s---", card_tlimit, cur_card, this_card);
                    PrintScreen("\n---limit = %d  cur_card = %s   this_card = %s---", card_tlimit, cur_card, this_card);
#endif
                    DebugPrintf("\n-----card beginning work-----\n");

                                        /////////////////////////////////////////////
                                        //记录刷卡信息////////////////////////////
                    if (cur_card[0] == 0) {						//如果当前没有人使用设备
                        sprintf(cardrecordwr, "%.14s_%.14s_%08lX", read_sys_Time, read_sys_Time, cur_cardsnr);		//更新刷卡信息
                        sprintf(card_time, "%.14s_", read_sys_Time);
                        for (i = 0; i < 14; i++)				//写入读卡时间到24c02b
                            write_at24c02b(ADDR_BEGIN+10+i, card_time[i]);
                    }
                    else {										//当前设备有人使用
                        sprintf(cardrecordwr, "%.14s_%.14s_", card_time, read_sys_Time);
                                                strncat(cardrecordwr,cur_card,8);		//将当前使用卡添加到记录中，因为要返回与上机者相同的卡号
                    }

                    memset(user_temp, 0, 50);
                    sprintf(user_temp, "%08lX", cur_cardsnr);		//将卡号保存到user_temp
                    key.dptr = user_temp;
                    key.dsize = strlen(user_temp)+1;
                    data = key;

                    pthread_mutex_lock(&cardfile_lock);
                    if (gdbm_exists(gdbm_user, key) != 0) {			//返回1则用户存在，可以使用(user_temp因为没有加'S'，'N'，所以即使他为超级用户非机组用户不能检测到)
                                                DebugPrintf("\n--------got an admin card:%s-------------------",this_card);
                                                PrintScreen("\n--------got an admin card:%s-------------------",this_card);
                                                pthread_mutex_unlock(&cardfile_lock);
                        Led_delay = 1;								//表示现在电源状态需要变化(可以是上机也可以是下机)
                    }
                    else { 											//返回0则用户不在
                        pthread_mutex_unlock(&cardfile_lock);
                        DebugPrintf("\n------------ admin user not exists ----------------");
                        PrintScreen("\n------------ admin user not exists ----------------");
                        //need_delay = 1;
                        check_unknown(user_temp);					//检测未知卡
                        break;
                                        }
                    key.dptr = cardrecordwr;
                    key.dsize = strlen(cardrecordwr) + 1;

                    data = key;
                    pthread_mutex_lock(&cardfile_lock);
                    if (db_store(gdbm_card, key, data) < 0) {		//保存读卡记录
                        DebugPrintf("\n--------store cardrecordwe err ------");
                       // break;
                    }
                    else {
#if NDEBUG
                        DebugPrintf("\n---data store success ---");
#endif
                                        }
                    pthread_mutex_unlock(&cardfile_lock);
                    pre_cardsnr = cur_cardsnr;						//更新卡号
                    beginsendcard = 1;								//准备发送cardrecordwr
                    need_delay = 0;
                    pre_ctime = cur_ctime;
                                }
            } while (0);

/**********************检测设备是否上电***************************************************************/
            devicecount++;
            if(devicecount >= 50) // 检测电源频率
            {
                                beginsendcard = 1;
                PrintScreen("\n-----device detecting-----");
                devicecount = 0;
                cur_devicestate = ReadVol();
#if NDEBUG
                                DebugPrintf("\n---------cur_devicestate = %d pre_devicestate = %d-----------------", cur_devicestate, pre_devicestate);
#endif
                if(cur_devicestate == -1)
                {
                    close_gpio_e();
                    init_gpio_e();
                }
                if(pre_devicestate == 0 && cur_devicestate == 1)		//设备上电
                {
                    DebugPrintf("\n-----device beginning work-----");
                    ReadSysTime();
                    memcpy(read_sys_Time, sys_Time, 15);
                    sprintf(devicerecordwr, "%.14s_device_beginning", read_sys_Time);
                    key.dptr = devicerecordwr;
                    key.dsize = strlen(devicerecordwr) + 1;
                    data = key;
                    if (db_store(gdbm_device, key, data) < 0)
                        DebugPrintf("\n----------save device state err--------");
                    pre_devicestate = cur_devicestate;
                }
                if(pre_devicestate == 1 && cur_devicestate == 0)		//设备断电
                {
                    DebugPrintf("\n-----device finishing work-----");
                    ReadSysTime();
                    memcpy(read_sys_Time, sys_Time, 15);
                    sprintf(devicerecordwr, "%.14s_device_finishing", read_sys_Time);

                    key.dptr = devicerecordwr;
                    key.dsize = strlen(devicerecordwr) + 1;
                    data = key;
                    if (db_store(gdbm_device, key, data) < 0)			//保存断电记录
                        DebugPrintf("\n----------save device state err--------");
                    pre_devicestate = cur_devicestate;
                }
                GetNetStat();											//显示运行的网络服务???
            }
            CardPacketSend_count++;
            if(CardPacketSend_count == 500)								//定时打印线程运行
            {
                CardPacketSend_count = 0;
                DebugPrintf("\n----- CardPacketSend_thread running -----");
            }
        }

                /**************卡不存在 增加卡***************/
                if(arrive_card == 1) {
                        key.dptr = cur_card;
                        key.dsize = strlen(cur_card) + 1;
                        data = key;

                        DebugPrintf("\n-------arrive card make it on----\n");
                        if (gdbm_exists(gdbm_user, key) == 0) {
                                DebugPrintf("\n---------no user here add now----\n");
                                PrintScreen("\n---------no user here add now----\n");
                                if (db_store(gdbm_user, key, data) < 0) {
                                                DebugPrintf("\n--------add user data err ------");
                                }
                                else
                                        system("cp /tmp/user.xml /mnt/user.xml");
                        }
                        DebugPrintf("\n----- arrive an unknow to admin card------");
                        PrintScreen("\n----- arrive an unknow to admin card------");

                        if (led_state)			//根据LED状态将对应位置1
                                Led_off = 1;
                        else
                                Led_on = 1;

                        key.dptr = cardrecordwr;							//保存刷卡信息
            key.dsize = strlen(cardrecordwr) + 1;

            data = key;
            pthread_mutex_lock(&cardfile_lock);
            if (db_store(gdbm_card, key, data) < 0) {
                                DebugPrintf("\n--------store cardrecordwr err ------");
            }
            else {
                                DebugPrintf("\n--------store cardrecordwr successfully!---------\n");
                        }
            pthread_mutex_unlock(&cardfile_lock);
            beginsendcard = 1;

                        arrive_card = 0;
                }
                else if(arrive_card == 2) {
                        sprintf(user_temp,"N%s",this_card);
                        key.dptr = user_temp;
                        key.dsize = strlen(user_temp) + 1;
                        data = key;

                        DebugPrintf("\n\n\n-------arrive card make it on----");
                        if (gdbm_exists(gdbm_user, key) == 0) {
                                DebugPrintf("\n---------no user here add now----");
                                if (db_store(gdbm_user, key, data) < 0) {
                                                DebugPrintf("\n--------add user data err ------");
                                                // break;
                                }
                                else
                                        system("cp /tmp/user.xml /mnt/user.xml");
                        }

                        DebugPrintf("\n----- arrive an unknow to normal card------\n");
                        PrintScreen("\n----- arrive an unknow to normal card------\n");
                        check_ordertime(cur_cardsnr,cardrecordwr,card_time,read_sys_Time);

                        arrive_card = 0;
                }
                /*****************************/

/*		if (arrive_card == 1) {			//arrive_card为1表示服务器已验证此未知卡为机组卡，可保存卡号
                }
                else if(arrive_card == 2) {		//arrive_card为2表示服务器验证此未知卡为非机组卡
                }
*/
        if(Led_on == 1)					//电源开，在终端上上下机没有明确的判断，由电源状态来判断
        {								//如果在上机的时候另一个人刷卡上机
            card_beep(50);
            card_beep(50);
            card_beep(50);
            TurnLedOn();
            write_at24c02b(46, 1);		//记录电源状态
            led_state = 1;
            Led_on = 0;
            need_delay = 1;				//只要在电源开启的状态下need_delay都置1
            memcpy(cur_card, this_card, strlen(this_card) + 1);

            DebugPrintf("\n-------cur_card = %s------\n");
            for (i = 0; i <  8; i++) {			//更新现在24C02B暂存的用户卡号
                write_at24c02b(i+ADDR_BEGIN, cur_card[i]);
            }
#if NDEBUG
           // DebugPrintf("\n-------cur_card time begin from %ld----\n", time_last);
#endif
        }

        if(Led_off == 1)					//如果电源没开，则不需要延时
        {
            card_beep(50);
            TurnLedOff();
            write_at24c02b(46, 0);
            Led_off = 0;
            led_state = 0;
            cur_card[0] = 0;				//电源关闭后cur_card也就没有了
            write_at24c02b(ADDR_BEGIN, 0);		//将60位写0，表示现在没有卡号
            need_delay = 0;
        }

        if(Led_delay == 1)					//需要延迟则将Led_delay置1
        {
            if(read_at24c02b(46) == 1)		//判断设备现在状态，如果是通电的话则断电，反之亦然
            {
                 Led_off = 1;

            }
            else
            {
                Led_on = 1;
            }
             Led_delay = 0;
        }

        if (update_user_xml) {				//需要在24C02上更新用户版本号
            db_close(gdbm_user);
            //gdbm_user = db_open("/tmp/user.xml");
            system("rm /tmp/user.xml");
            system("cp /tmp/user_cur.xml /tmp/user.xml");
            system("cp /tmp/user.xml /mnt/user.xml");

            if ((gdbm_user = db_open("/tmp/user.xml")) != NULL)
                DebugPrintf("\n--------!!!!!!!!!!!!!!update user successful------\n");

            write_at24c02b(232, (user_version >> 24) & 0xFF);
            write_at24c02b(233, (user_version >> 16) & 0xFF);
            write_at24c02b(234, (user_version >> 8) & 0xFF);
            write_at24c02b(235, (user_version ) & 0xFF);
            update_user_xml = 0;
        }
        if(updata_ordertime_xml){						//更新备份的预约时间数据库
                        DebugPrintf("\n-------------!!!!!!!!!updata_ordertime.xml-------------\n");
                        updata_ordertime_xml = 0;
                        //system("cp /tmp/ordertime.xml /tmp/ordertime.xml");

                        system("cp /tmp/ordertime.xml /mnt/ordertime.xml");

                        system("./gread ../ordertime.xml");

                }

/**********************发送设备信息***************************************************************/
                do {
                        if(beginsenddevice)
                        {
                                transBuffer[0] = 0x00;
                                transBuffer[1] = 0x01;
                                transBuffer[2] = 0x03;
                                next1 = 0;
                                i = 0;
                                pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
                                do
                                {
                                        //devicerecordre = XmlRead("/tmp/device.xml", 1, next1, 0, 0, "record");
        #if NDEBUG
                                        DebugPrintf("\n-----------begin send device--------");
        #endif
                                        key = gdbm_firstkey(gdbm_device);
                                        devicerecordre = key.dptr;
                                        if (key.dptr == NULL) {

        #if NDEBUG
                                        DebugPrintf("\n-----------no data has been fetched--------");
        #endif
                                                break;
                                        }
        #if NDEBUG
                                        else
                                                DebugPrintf("\n-----------fetch a data--------");
        #endif
                                        if(devicerecordre != NULL)
                                        {
                                                devicedelfile = 1;
                                                memcpy(transBuffer+3, devicerecordre, strlen(devicerecordre));
                                                DebugPrintf("\n-----devicerecordre = %s -----\n",devicerecordre);

                                           // for (i=0; i<MAX_LINK_SOCK; i++)
                                                {

                                                        if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                                                        {
                                                                //pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                                                                SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, strlen(devicerecordre)+3);
                                                                //pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);
                                                                DebugPrintf ("\n-----device state send-----");
                                                                gdbm_delete(gdbm_device, key);
                                                                //system("cp /tmp/devices.xml /mnt/devices.xml");
                                                        }
                                                        else {
                                                                //system("cp /tmp/devices.xml /mnt/devices.xml");
                                                                DebugPrintf ("\n-----device state send err-----");
                                                                free(devicerecordre);
                                                                break;
                                                        }
                                                }
                                                free(devicerecordre);
                                                next1++;
                                        }
                                        //DebugPrintf("\n-----hello -----\n");
                                }while(1);
                                pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
                                if(devicedelfile == 1)
                                {
                                        DelFile("/tmp/device.xml");
                                        XmlCreat("/tmp/device.xml");
                                        devicedelfile = 0;
                                }
                                beginsenddevice = 0;
                        }
                } while (0);

/**********************发送串号***************************************************************/
        if(beginsendsnrnum)
        {
            transBuffer[0] = 0x00;
            transBuffer[1] = 0x01;
            transBuffer[2] = 0x00;
            pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
            snrnumrecordre = snrnum;;//XmlRead("net_smtp.xml", 2, 8, 0, 0, "Receiver");
            if(snrnumrecordre != NULL)
            {
                memcpy(transBuffer+3, snrnum, strlen(snrnum));
                for (i=0; i<MAX_LINK_SOCK; i++)
                {

                    if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                    {
                        //pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                        SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, strlen(snrnumrecordre)+3);
                        //pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);

                    }
                }

            }
            pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
            beginsendsnrnum = 0;
        }


/**********************发送终端开关信息***************************************************************/
        if(beginsendtersta)
        {
            transBuffer[0] = 0x00;
            transBuffer[1] = 0x01;
            transBuffer[2] = 0x02;
            next1 = 0;
            pthread_mutex_lock(&sttConnSock[0].lockBuffOut);
            do
            {
                terminalstatesre = XmlRead("terminalstates.xml", 1, next1, 0, 0, "record");
                if(terminalstatesre != NULL)
                {
                    terminalstatesdelfile = 1;
                    memcpy(transBuffer+3, terminalstatesre, strlen(terminalstatesre));
                    DebugPrintf("\n-----terminalstatesre = %s -----",terminalstatesre);

                    for (i=0; i<MAX_LINK_SOCK; i++)
                    {

                        if (sttConnSock[i].fdSock>0 && sttConnSock[i].loginLegal>0)
                        {
                            //pthread_mutex_lock(&sttConnSock[i].lockBuffOut);
                            SockPackSend(CMD_ACK, sttConnSock[i].fdSock, &sttConnSock[i], transBuffer, strlen(terminalstatesre)+3);
                            //pthread_mutex_unlock(&sttConnSock[i].lockBuffOut);

                        }
                    }

                    next1++;
                }
                //DebugPrintf("\n-----hello -----\n");
            }while(terminalstatesre != NULL);
            pthread_mutex_unlock(&sttConnSock[0].lockBuffOut);
            if(terminalstatesdelfile == 1)
            {
                DelFile("terminalstates.xml");
                XmlCreat("terminalstates.xml");
                terminalstatesdelfile = 0;
            }

            beginsendtersta = 0;
        }

        if(beginsavewebpar)
        {
            Save_Webserver_Par();
            beginsavewebpar = 0;
        }
    }
    DebugPrintf("\n-----CardPacketSend Thread exit-----");
    sleep(1);
    system("reboot");
}


 int WorkThreadCreate(ptexec threadexec, int prio) // 创建线程
{
        pthread_t pid;
        pthread_attr_t attr;
        //int policy;
        int err;
                                                                                                                //线程默认的属性为非绑定、非分离、缺省1M的堆栈、与父进程同样级别的优先级。
        err = pthread_attr_init(&attr);								//线程属性值不能直接设置，须使用相关函数进行操作，
                                                                                                                //初始化的函数为pthread_attr_init，这个函数必须在pthread_create函数之前调用
        if (err != 0)
        {
                perror("\n----WorkThreadCreate--pthread_attr_init err\n");
                return err;
        }

        err = pthread_attr_setschedpolicy(&attr, SCHED_RR);			//SCHED_FIFO --先进先出；SCHED_RR--轮转法；SCHED_OTHER--其他
        if (err != 0)
        {
                perror("\n----pthread_attr_setschedpolicy err\n");
                return err;
        }

        err = pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);			//设置线程的分离属性
                                                                                                                                        //PTHREAD_CREATE_DETACHED -- 分离线程
                                                                                                                                        //PTHREAD _CREATE_JOINABLE -- 非分离线程
        if (err == 0)
        {
                err = pthread_create(&pid, &attr, threadexec, NULL); //(void*)&sttConnSock[i]);;
                if (err != 0)
                {
                        perror("\n----WorkThreadCreate--pthread_create err\n");
                        return err;
                }
        }
        err = pthread_attr_destroy(&attr);
        if (err != 0)
        {
                perror("\n----WorkThreadCreate--pthread_attr_destroy err\n");
                return err;
        }
        return 0;
}

int GetIpaddr() // 获取本地IP
{
        char tem_ipdz[20];
        struct sockaddr_in *my_ip;
        struct sockaddr_in *addr;
        struct sockaddr_in myip;
        my_ip = &myip;
        struct ifreq ifr;
        int sock;

        if((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
        {
                DebugPrintf("\nsock error \n");
                return -1;
        }
        strcpy(ifr.ifr_name, "eth0");
        //取本机IP地址
        if(ioctl(sock, SIOCGIFADDR, &ifr) < 0)
        {
                DebugPrintf("\nioctl SIOCGIFADDR \n");
                return -1;
        }
        my_ip->sin_addr = ((struct sockaddr_in *)(&ifr.ifr_addr))->sin_addr;
        strcpy(tem_ipdz,inet_ntoa(my_ip->sin_addr));
        DebugPrintf("\n-----ipaddr = %s-----\n",tem_ipdz);
        close(sock);
        return 0;
}

int islink(void) // 判断SD卡是否挂载
{
    FILE *fd_usb;
    int fd_ext3;
        fd_usb = fopen("/mnt/islink","r+");
        if(fd_usb == NULL)
        {
                DebugPrintf("\n------islink has lost------------");
                fd_ext3 = access("/mnt/lost+found/",F_OK);		//判断/lost+found/是否存在
                if (fd_ext3 == -1)								//目录不存在返回-1
                {
                        DebugPrintf("\n----- sd card error -----");
                        return -1;
                }
                else{
                        system("touch /mnt/islink");
                }
        }
        else
        fclose(fd_usb);
        return 0;
}
