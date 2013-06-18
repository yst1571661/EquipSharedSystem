/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  stream_server.c
 *       Compiler:  gcc
 *
 *    Description:  implement IP cam stream server
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  09/20/10 17:11:54
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <stdarg.h>
///////////////////////////////////////////////////////////////////////////
#include "streaming_server.h"
#include "ip_cam_dev.h"
#include "file_opt_asf.h"
#include "spct_dataqueue.h"
#include "spct_decoder.h"
#include "YUV2JPEG.h"
#include "tcp_connect.h"
#include "rtc.h"
#include "xml.h"
#include "log.h"
///////////////////////////////////////////////////////////////////////////
//extern int freqsendbmp;

typedef struct server_context
{
        // streaming
        Streaming_Server server;
        Streaming_Session * session;
        Ip_Cam_Device ipcam;

        // file opt
        int bsavefile;
        File_Property  pro;
        File_Opt fileopt;
        int fileoptready;
        int filesize;
        char basename[256];
        char outfilename[256];
        int beginwrite;
        Data_Queue dq;
        int stop;
        struct timeval timeold;
        struct timeval timenew;
        int framecount;
        int timecount;
} Server_Context;

extern struct sockaddr_in gserver_addr;
extern struct err_check err_check;

//extern GDBM_FILE gdbm_card;

//extern int is_redict;
//extern int BASIC_LEVEL_;



static void check_photo(char *bmpfilename)
{
        FILE *fp = NULL;
        long curpos = 0;

        //DebugPrintf("\n--------picture err state check begin-----------\n");
        fp = fopen (bmpfilename, "ab+");
        if (fp == NULL) {
                DebugPrintf("\n----open %s err--------", bmpfilename);
                return;
        }
        fseek(fp,0,SEEK_END); //locate to the end of the file
        curpos = ftell(fp); //the lenth of the file

        Err_Check.photo = 0;
        if (curpos < 10240)  {
                Err_Check.photo = 0xFF;
                DebugPrintf("\n-----------picture  state err--------------");
        }
        else
                DebugPrintf("\n-----------picture  state ok--------------");

        Err_Check.photo_checked = 1;
        fclose(fp);
}

static void * save_file(void * arg)
{
        Server_Context * context;
        Spct_Data * data;
        context = (Server_Context *) arg;

        // set data queue used
        dq_set_used(&context->dq);
        ///////////////////////////////////////////////////////////////////////
        unsigned char * gotbuf;
        int  gotsize;
        Decode_Context videodecoder;
        int ret;
        char bmpfilename[50];
        char delfilename[50];
        unsigned char min_time[2];
        int premin_time = 0;
        int curmin_time = 0;
        int prehou_time = 0;
        int curhou_time = 0;
        int preday_time = 0;
        int curday_time = 0;
        int premon_time = 0;
        int curmon_time = 0;
        int preyea_time = 0;
        int curyea_time = 0;
        int cursec_time = 0;
        int presec_time = 0;
        int save_curhour = 0;
        int save_prehour = 0;
        time_t cur_seconds = 0;
        time_t pre_seconds = 0;


        unsigned char read_sys_Time[15];
        ret = spct_decode_open(SPCT_CODEC_H264, &videodecoder, 0);
        if(ret)
        {
                fprintf(stderr, " Open h264 decoder fail\n");
        }
        ReadSysTime();
        memcpy(read_sys_Time, sys_Time, 15);
        cursec_time = (read_sys_Time[12] - 48)*10 + (read_sys_Time[13] - 48);
        presec_time = cursec_time;
        curmin_time = (read_sys_Time[10] - 48)*10+(read_sys_Time[11] - 48);
        premin_time = curmin_time;
        curhou_time = (read_sys_Time[8] - 48)*10+(read_sys_Time[9] - 48);
        prehou_time = curhou_time;
        save_curhour = curhou_time;
        save_prehour = save_curhour;
        curday_time = (read_sys_Time[6] - 48)*10+(read_sys_Time[7] - 48);
        preday_time = curday_time;
        curmon_time = (read_sys_Time[4] - 48)*10+(read_sys_Time[5] - 48);
        premon_time = curmon_time;
        curyea_time = (read_sys_Time[0] - 48)*1000+(read_sys_Time[1] - 48)*100+(read_sys_Time[2] - 48)*10+(read_sys_Time[3] - 48);
        preyea_time = curyea_time;
        DebugPrintf("\n----- curmin_time = %d -----",curmin_time);

        cur_seconds = time(NULL);
        pre_seconds = cur_seconds;

        freqsendbmp = 1;
        ///////////////////////////////////////////////////////////////////////
        int save_file_count = 0;
        while(1)
        {
            save_file_count++;
            if(save_file_count == 50)
            {
                save_file_count = 0;
                DebugPrintf("\n----- save_file thread running -----");
                PrintScreen("\n----- save_file thread running -----");
            }
            /////////启动灵敏度自校准开始///////////////
            //if (curhou_time == 2) {
                    //is_redict = 0;
                    //BASIC_LEVEL_ = 24000;
            //}
            ////////////////////////
            // get data from data queue
            data = dq_get(&context->dq);
            if(data == NULL)
            {
                DebugPrintf("\n----- data == NULL -----");
                continue;
            }
            else
            {
                if(data->type == DATA_VIDEO)
                {
                    if(data->flags == DATA_VIDEO_I)
                    {
            ///////////////////////////////////////////////////////////////////
                        ReadSysTime();
                        memcpy(read_sys_Time, sys_Time, 15);
                        curmin_time = (read_sys_Time[10] - 48)*10+(read_sys_Time[11] - 48);
                        curhou_time = (read_sys_Time[8] - 48)*10+(read_sys_Time[9] - 48);
                        save_curhour = curhou_time;
                        curday_time = (read_sys_Time[6] - 48)*10+(read_sys_Time[7] - 48);
                        curmon_time = (read_sys_Time[4] - 48)*10+(read_sys_Time[5] - 48);
                        curyea_time = (read_sys_Time[0] - 48)*1000+(read_sys_Time[1] - 48)*100+(read_sys_Time[2] - 48)*10+(read_sys_Time[3] - 48);

                        if(curmon_time != premon_time)
                        {
                            if(curmon_time == 1)
                            {
                                sprintf(delfilename, "/mnt/work/%04d11*.jpg",curyea_time-1);
                                DebugPrintf("\n-----delfilename = %s -----",delfilename);
                                DelFile(delfilename);
                            }
                            else if(curmon_time == 2)
                            {
                                sprintf(delfilename, "/mnt/work/%04d12*.jpg",curyea_time-1);
                                DebugPrintf("\n-----delfilename = %s -----",delfilename);
                                DelFile(delfilename);
                            }
                            else
                            {
                                sprintf(delfilename, "/mnt/work/%04d%02d*.jpg",curyea_time, curmon_time-2);
                                DebugPrintf("\n-----delfilename = %s -----",delfilename);
                                DelFile(delfilename);
                            }
                            premon_time = curmon_time;
                            ////////////////////////////////////////////////////////////
                            DebugPrintf("\n----- curmin_time = %d -----",curmin_time);
                            ret = spct_decode(&videodecoder, data->data,
                            data->size, &gotbuf, &gotsize);
                            if(ret < 0) // decode error
                            {
                                    DebugPrintf("\n----- decode error -----");
                                    break;
                            }
                            if(gotsize != 0) // decoder need more data
                            {
                                    //display one frame
                                    //sprintf(bmpfilename, "/mnt/safe/%.12s.jpg", read_sys_Time);
                                    sprintf(bmpfilename, "/mnt/safe/%.14s.jpg", read_sys_Time);
                                    DebugPrintf("\n-----bmpfilename = %s-----", bmpfilename);
                                    YUV2JPEG(gotbuf, 704, 576, bmpfilename);
                                    DebugPrintf("\nbmp save gotsized:%d ", gotsize);
                                    //BmpFileSend(bmpfilename);
                            }
                            else
                            {
                                    DebugPrintf("\n----- decoder need more data -----");
                            }
                            //////////////////////////////////////////////////////////////
                        }
                        if(curday_time != preday_time)
                        {
                            preday_time = curday_time;
                            if(curmon_time == 1)
                            {
                                sprintf(delfilename, "/mnt/work/%04d12%02d*.jpg",curyea_time-1, curday_time);
                                DebugPrintf("\n-----delfilename = %s -----",delfilename);
                                DelFile(delfilename);
                            }
                            else
                            {
                                    sprintf(delfilename, "/mnt/work/%04d%02d%02d*.jpg",curyea_time, curmon_time-1,curday_time);
                                    DebugPrintf("\n-----delfilename = %s -----",delfilename);
                                    DelFile(delfilename);
                            }
                        }
                        cur_seconds = time(NULL);

                        switch (catch_mode)
                        {
                            case 0x01:
                            if (catchonemotion || Err_Check.begincheck)
                            {
                                //if(curmin_time  + 60 - premin_time == freqsendbmp || curmin_time  + 60 - premin_time == freqsendbmp + 60) //采集图像频率
                                #if NDEBUG
                                        DebugPrintf("\n--------catch_freq = %d--------", catch_freq);
                                #endif
                                if (cur_seconds - pre_seconds >= catch_freq || (Err_Check.begincheck && !Err_Check.photo_checked))
                                {
                                    DebugPrintf("\n-----------motion deteced-------------------");
                                    PrintScreen("\n-----------motion deteced-------------------");
                                    DebugPrintf("\n----- curmin_time = %d   cur_seconds = %ld   pre_seconds = %ld-----",curmin_time, cur_seconds, pre_seconds);
                                    pre_seconds = cur_seconds;
                                    ret = spct_decode(&videodecoder, data->data,
                                    data->size, &gotbuf, &gotsize);
                                    if(ret < 0) // decode error
                                    {
                                        DebugPrintf("\n----- decode error -----");
                                        break;
                                    }
                                    if(gotsize != 0) // decoder need more data
                                    {
                                        // display one frame
                                        //sprintf(bmpfilename, "/tmp/%.12s.jpg", read_sys_Time);
                                    sprintf(bmpfilename, "/tmp/%.14s.jpg", read_sys_Time);
                                    DebugPrintf("\n-----bmpfilename = %s-----\n", bmpfilename);
                                    YUV2JPEG(gotbuf, 704, 576, bmpfilename); // save .jpg pictures
                                    DebugPrintf("\nbmp save sized:%d ", data->size);
                                    DebugPrintf("\nbmp save gotsized:%d ", gotsize);

                                    //if (Err_Check.begincheck && (!Err_Check.photo_checked))
                                    check_photo(bmpfilename);

                                    BmpFileSend(bmpfilename);			// send pictures

                                }
                                else
                                {
                                    DebugPrintf("\n----- decoder need more data -----\n");
                                }
                                catchonemotion = 0;
                            }
                        }
                        break;

                        default:
                        if (cur_seconds - pre_seconds >= catch_freq || (Err_Check.begincheck && !Err_Check.photo_checked))
                        {
                            DebugPrintf("\n---prefix curmin_time = %d   cur_seconds = %ld   pre_seconds = %ld-----",curmin_time, cur_seconds, pre_seconds);
                            pre_seconds = cur_seconds;
                            #if NDEBUG
                                    DebugPrintf("\n--------catch_freq = %d--------", catch_freq);
                            #endif
                            ret = spct_decode(&videodecoder, data->data,
                            data->size, &gotbuf, &gotsize);
                            if(ret < 0) // decode error
                            {
                                    DebugPrintf("\n----- decode error -----");
                                    break;
                            }
                            if(gotsize != 0) // decoder need more data
                            {
                                // display one frame
                                //sprintf(bmpfilename, "/tmp/%.12s.jpg", read_sys_Time);
                                sprintf(bmpfilename, "/tmp/%.14s.jpg", read_sys_Time);
                                DebugPrintf("\n-----bmpfilename = %s-----", bmpfilename);

                                YUV2JPEG(gotbuf, 704, 576, bmpfilename); // save .jpg pictures

                                DebugPrintf("\nbmp save sized:%d ", data->size);
                                DebugPrintf("\nbmp save gotsized:%d ", gotsize);
                                //DebugPrintf("\n----Err_Check.begincheck = %d-----\n", Err_Check.begincheck);
                                //DebugPrintf("\n------beginsendbmp = %d bmpfilename = %s\n\n", beginsendbmp, bmpfilename);
                                //if (Err_Check.begincheck && (!Err_Check.photo_checked))
                                        check_photo(bmpfilename);
                                //DebugPrintf("\n------beginsendbmp = %d bmpfilename = %s\n\n", beginsendbmp, bmpfilename);
                                BmpFileSend(bmpfilename);			// send pictures
                            }
                            else
                            {
                                DebugPrintf("\n----- decoder need more data -----\n");
                            }
                        }
                        break;
                    }

                    //////////////////////////////////////
                    /////////////更新时间/////////////////
                    if(time_change == 1)
                    {
                            premin_time = curmin_time;
                            time_change = 0;
                            pre_seconds = cur_seconds;
                    }
                    //////////////////////////////////////

                    if (premin_time != curmin_time)
                    {
                            GetIpaddr();
                            DebugPrintf("\n-----snrnum = %s -----\n",snrnum);//show the macaddr every minite
                            premin_time = curmin_time;
                            //beginsendcard = 1;
                    }

                    //DebugPrintf("\n--save_curhour = %d, save_prehour = %d-----\n", save_curhour, save_prehour);

                    if(save_curhour  + 24 - save_prehour == 3 || save_curhour  + 24 - save_prehour == 3 + 24) {
                        startsyncbmp = 1;
                        save_prehour = save_curhour;
                        DebugPrintf("\n------copy card info to flash------\n");
                        pthread_mutex_lock(&cardfile_lock);				//update card.xml
                        system("cp /tmp/cards.xml /mnt/cards.xml");
                        pthread_mutex_unlock(&cardfile_lock);
                    }

                    if(curhou_time != prehou_time)
                    {
                        //startsyncbmp = 1;
                        prehou_time = curhou_time;
                        if(islink() == 0) // SD卡已挂载
                        {
                                system("cp -rf /tmp/*.jpg /mnt/work/");
                        }
                        system("rm -rf /tmp/*.jpg");
                        beginsavewebpar = 1;
                    }
                //////////////////////////////////////////////////////////////////////
                }
            }
            // free data from data queue
            dq_pop(&context->dq);
        }
    }
    ///////////////////////////////////////////////////////////////////////
    spct_decode_close(&videodecoder);
    DebugPrintf("\n-----save_file Thread exit-----\n");
    sleep(1);
    system("reboot");
    ///////////////////////////////////////////////////////////////////////
}

static void send_data(void * handler, void * arg)
{
        Server_Context * context;
        Streaming_Session * session;
        Spct_Data * data ;
        int passsecond;
        if(handler == NULL || arg == NULL)
                return;
        context = (Server_Context *) handler;
        session = context->session;
        data = (Spct_Data * ) arg;
        if(data->type == DATA_VIDEO)
        {
                // frame rate calculate
                context->framecount ++;
                if(context->framecount == 1)
                {
                        gettimeofday(&context->timeold, NULL);
                        context->timecount = 0;
                }
                else if((context->framecount %1000) == 0)
                {
                        gettimeofday(&context->timenew, NULL);
                        passsecond = context->timenew.tv_sec - context->timeold.tv_sec;
                        context->timecount += passsecond;
                        DebugPrintf("\nGet send frame %d, pass time %d, pertime %d, framerate %d\n",
                                        context->framecount,
                                        context->timecount ,
                                        passsecond,
                                        1000/passsecond);
                  context->timeold =  context->timenew;
                }
        }

        // send data to save file data queue
        if(context->bsavefile)
        {
                if(data->type == DATA_VIDEO && data->flags == DATA_VIDEO_I)
                {
                        dq_in(&context->dq, data);
                }
        }


        // streaming data
        streaming_send_data(context->session, data);
}

int showhelp(char * program)
{
        DebugPrintf("\n%s [-h] [-s] [filename]", program);
        DebugPrintf("\n         -h: show help");
        DebugPrintf("\n         -s: save asf file at the save time streaming");
        DebugPrintf("\n   filename: the file name ");
}


void pasarg(int argc, char * argv[], Server_Context * context)
{
        int i;
        if(argc == 1)
        {
                strncpy(context->basename, "test.asf", sizeof(context->basename));
                strncpy(context->outfilename,"test.asf", sizeof(context->basename));
                return ;
        }
        for(i = 0; i < argc; i++)
        {
                if(strncmp(argv[i], "-s", 2) == 0)
                {
                        context->bsavefile = 1;
                }
                else if(strncmp(argv[i], "-h", 2) == 0)
                {
                        showhelp(argv[0]);
                        exit(0);
                }else
                {
                        strncpy(context->basename, argv[i], sizeof(context->basename));
                        strncpy(context->outfilename, argv[i], sizeof(context->basename));
                }
        }
}


extern int BASIC_LEVEL_;
extern int is_redict;
extern unsigned long user_version;
extern int card_tlimit;
extern int user_count;
int main(int argc, char * argv[])
{
        Server_Context context;
        char url[256];
        char LogFilename[50],LogFilenameNext[50];
        unsigned char LogCount=0;
        int res;
        pthread_attr_t attr;
        pthread_t threadId1;

        //////////////////////////////////////////////////////////////////////
        if(init_at24c02b() == -1) // init eeprom
        {
                return -1;
        }
        system("mkdir /mnt/log");
        system("mkdir /mnt/work");
        system("mkdir /mnt/safe");

        init_log(LOGFILETMPDIR,LOG_DEBUG);

        FILE *fd_mac;
        char macaddr_cmd[50];
        char snrnum_temp[20];
        fd_mac = fopen("/tmp/macaddr","r+"); // open the macaddr file
        if(fd_mac == NULL)
        {
                DebugPrintf("\n-----error: open /tmp/macaddr failed-----");
        }
        else
        {
                fscanf(fd_mac,"%s",snrnum_temp);//the macaddr is 12bit
                fclose(fd_mac);
                strcpy(snrnum, snrnum_temp);
                snrnum[12] = '\0';
        }
        DebugPrintf("\n-----snrnum = %s-----", snrnum);
        sprintf(macaddr_cmd,"ifconfig eth0 hw ether %.2s:%.2s:%.2s:%.2s:%.2s:%.2s",snrnum_temp,snrnum_temp+2,snrnum_temp+4,snrnum_temp+6,snrnum_temp+8,snrnum_temp+10);
        DebugPrintf("\n----------%s----------",macaddr_cmd);
        DebugPrintf("\n");
#if RELEASE_MODE
        system("ifconfig eth0 down ");
        system(macaddr_cmd);
        system("ifconfig eth0 up");
        system("sleep 5");

        net_configure();
#endif
        ////////////////////////////////////////////////////////////////////
        memset(&context , 0 , sizeof(context));

        // parser argv
        pasarg(argc, argv, &context);
        //catch_mode 226    catch_sen  227  catch_freq 228 229   238 check eeprom 236 237 数卡时间间隔 239 240 用户数目
        card_tlimit = 30;							//init the limit of card interval
        device_mode = 0x01;							//default open mode
        beginsendbmp = 1;
        beginupload = 0;
        reboot_flag=0;

        if (read_at24c02b(225) == 11)
        {
            catch_mode = read_at24c02b(226);
            catch_sen = read_at24c02b(227);
            catch_freq = ((read_at24c02b(228) << 8) & 0xFF00);
            catch_freq += read_at24c02b(229);
            BASIC_LEVEL_ = ((read_at24c02b(230) << 8) & 0xFF00);
            BASIC_LEVEL_ += read_at24c02b(231);
            user_version = (read_at24c02b(232) << 24)& 0xFF000000;
            user_version +=  (read_at24c02b(233) << 16)& 0xFF0000;
            user_version +=  (read_at24c02b(234) << 8)& 0xFF00;
            user_version +=  (read_at24c02b(235))& 0xFF;

            if (user_version == 0xFFFFFFFF)
                user_version = 0;
            card_tlimit = ((read_at24c02b(220) << 8) & 0xFF00);
            card_tlimit += read_at24c02b(221);
            if (card_tlimit == 65535)
                card_tlimit = 30;
            if (read_at24c02b(239) == 1)	//set open mode on boot
                device_mode = 1;
            else
                device_mode = 0;
            //user_count = ((read_at24c02b(239) << 8) & 0xFF00) + read_at24c02b(240);
            is_redict = 0;
            BASIC_LEVEL_ = BASIC_VALUE;
        }
        else
        {
            catch_mode = 0x00;
            catch_sen = 2;
            catch_freq = 60;
            BASIC_LEVEL_ = 20000;
            write_at24c02b(225, 11);
            write_at24c02b(226, catch_mode);
            write_at24c02b(227, catch_sen);
            write_at24c02b(228, 0);
            write_at24c02b(229, catch_freq);
            write_at24c02b(230, (BASIC_LEVEL_>>8)&0xFF);
            write_at24c02b(231, BASIC_LEVEL_&0xFF);
            write_at24c02b(220, (card_tlimit>>8)&0xFF);
            write_at24c02b(221, card_tlimit&0xFF);
            write_at24c02b(239,1);		//default set open mode
            write_at24c02b(241,0);
            write_at24c02b(242,0);              //log number
            is_redict = 0;			//test motion data when boot
            write_at24c02b(60, 0); //write_at24c02b(ADDR_BEGIN, 0);make
        }

        card_tlimit = 30;
        // perpare file property
        context.pro.audio_codec = CODEC_CT_G723;
        context.pro.video_codec = CODEC_H264;
        context.pro.width = 704; 	// PAL D1
        context.pro.height = 576; 	// PAL D1
        context.pro.audioen = 1; 	// not save audio
        context.pro.videoen = 1; 	// save video
        context.pro.channels = 0; 	// must 0
        context.pro.sample_rate = 8000; // audio sample rate
        context.pro.bit_rate = 200; 	// bitrate

        // device init
        /*if(ip_cam_construct(&context.ipcam, "/dev/video1"))
        {
                fprintf(stderr, "Open device fail\n");
                return -1;
        }
        */
        // init streaming server
        /*if(streaming_server_construct(&context.server, 554)) // use default RTSP port
        {
                fprintf(stderr, "Create srever fail\n");
                return -1;
        }
        */
        // create stream session
        /*context.session = streaming_new_session(&context.server,
                                        0,	//channel 0
                                        6000, // rtp port number
                                        0,	  // is not multicast
                                        NULL, // use radom multicast address
                                        1,	  // stream aduio
                                        1	  // stream video
                                        );

        if(context.session == NULL)
        {
                fprintf(stderr, "Create streaming session fail\n");
                streaming_server_destruct(&context.server);

                return -1;
        }

        streaming_get_session_url(context.session, url, sizeof(url));
        DebugPrintf("spct streaming session URL: %s\n", url);

        // register data handle function to ip cam device
        context.ipcam.datahandler = &context;
        context.ipcam.fun = send_data;
        streaming_server_start(&context.server);

        ///////////////////////////////////////////////////////////////////////////
        cam_start_work(&context.ipcam);
        */
        init_ds3231(); // init clock chip
        struct rtc_time curtime;
        get_time(&curtime);
        DebugPrintf("\n-----current time %02d%02d%02d%02d%d.%02d-----", curtime.tm_mon,curtime.tm_mday,curtime.tm_hour,curtime.tm_min,curtime.tm_year,curtime.tm_sec);
        //init_ch450();
        init_gpio_e();
        init_card_uart();

        ////////////////////////////////////////////////////////////////////////////
        if(read_at24c02b(45) == 11)
        {
            if(read_at24c02b(46) == 1)
            {
                TurnLedOn();
                led_state = 1;
            }
            else
            {
                TurnLedOff();
                led_state = 0;
            }
        }
        else
        {
            TurnLedOff();
            write_at24c02b(45, 11);
            write_at24c02b(46, 0);
        }

        /////////////////////////////////////////////////////////////////////////
        /*if(context.bsavefile) // save file
        {
                // data queue construct
                res = dq_construct(&context.dq,
                                800*1024, // buffer size 800k , must enough to hold 3 or more data
                                DQ_TYPE_WAIT // wait queue
                                );
                if(res != 0)
                {
                        DebugPrintf("construct data queue fail\n");
                        return -1;
                }
        */
                /*// file opt construct
                res = fileopt_asf_construct(&context.fileopt);
                if(res != 0)
                {
                        DebugPrintf("construct fileopt fail\n");
                        return -1;
                }
                // create file
                context.fileoptready = fileopt_create(&context.fileopt, context.outfilename, &context.pro);
                */
                // create save file  thread
        /*	pthread_attr_init(&attr);
                pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
                if((res = pthread_create(&threadId1, &attr, save_file, &context)) != 0)
                {
                        DebugPrintf("create save data thread fail\n");
                        return -1;
                }
        }
        else */
        {
                Err_Check.issavvideo = 1;
                Err_Check.photo = 0;
        }

        // start device to work
        //cam_start_work(&context.ipcam);

        /////////////////////////////////////////////////////////////////////////////
        //net_configure();                      					//  read_optfile();

        if (pthread_mutex_init(&sttDspRoute.dsp_lock, NULL) != 0)
        {
                perror("\nmutex sttDspRoute.dsp_lock failed!");
                exit(0);
        }

        if (WorkThreadCreate(WavePacketSend, 0))        	//start the synchronization pictures thread
        {
                perror("\n------------Thread WavePacketSend create error");
                fflush(stdout);
                exit(0);
        }

        if (WorkThreadCreate(CardPacketSend, 0))        	//start the query parameter thread
        {
                perror("\n------------Thread WavePacketSend create error");
                fflush(stdout);
                exit(0);
        }
        if (_ConnLoop() == -1)	//net connect
        {
                FreeMemForEx();
        }
        /////////////////////////////////////////////////////////////////////////////


        while(context.stop == 0)
        {
                usleep(4000000);
        }

        if(context.bsavefile)
        {
                pthread_join(threadId1, NULL);
        }

        // destruct ipcam
        ip_cam_destruct(&context.ipcam);

        streaming_server_stop(&context.server);
        streaming_del_session(&context.server, context.session);
        streaming_server_destruct(&context.server);

}

