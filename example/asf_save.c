
#include "streaming_client.h"
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>

#include "file_opt_asf.h"

typedef struct test_struct
{
        Streaming_Client * client;
        C_Session * session;
        char * url;
        int datacount;
} Test_Struct;

int x = 1000;


void * getdata_thread(void * par)
{
        Test_Struct * test = (Test_Struct *) par;
        C_Session * session = test->session;
        Spct_Data * data;
        File_Property  pro;
        File_Opt fileopt;
        char  outfilename[256];
        int res;
        int filesize = 0;
        int beginwrite = 0;
        int fileoptready = 1;

        pro.audio_codec = CODEC_CT_G723;
        pro.video_codec = CODEC_H264;
        pro.width = 704; // PAL D1
        pro.height = 576; // PAL D1
        pro.audioen = 1; // not save audio
        pro.videoen = 1; // save video
        pro.channels = 0; // must 0
        pro.sample_rate = 8000; // audio sample rate
        pro.bit_rate = 200; // bitrate

        fileopt_asf_construct(&fileopt);
        sprintf(outfilename, "out_%d.asf", test->datacount);

        fileoptready = fileopt_create(&fileopt, outfilename, &pro);

    while (test->datacount != 0) { // get datacount packet from server URL
        res = client_session_get_data(session, &data);
                if(res != 0)
                {
                        // get data fail, connect error, or server teardown
                        printf("get error data\n");
                        break;
                }
                if(data == NULL)
                {
                        // if the session is not blocked the data maybe == NULL, beacusethe data not ready, read again
                        //continue;
                }else
                {
                        // do some thing to process data
                        if(data->type == DATA_VIDEO)
                        {
                                test->datacount++;
                                if(data->flags == DATA_VIDEO_I)
                                {
                                        printf("got key frame frame size = %d, datacount = %d\n", data->size, test->datacount);
                                        if(beginwrite == 0)
                                        {
                                                beginwrite = 1; // asf file first frame must i frame
                                        }
                                }
                        }
                        if(beginwrite && (fileoptready == 0))
                        {
                                fileopt_write_data(&fileopt, data);
                                filesize += data->size;
                                if(filesize >= 200*1024*1024) // file len 200M
                                {
                                        fileopt_close(&fileopt);
                                        sprintf(outfilename, "out_%d.asf", test->datacount);
                                        fileoptready = fileopt_create(&fileopt, outfilename, &pro);
                                        filesize = 0;
                                        beginwrite = 0;
                                }
                        }
                        client_session_free_data(session, data);
                }
        }
}

int main (int argc, char *argv[])
{
        Spct_Data * data;
        C_Session * session;
        Test_Struct test1;
        Streaming_Client client;
        pthread_attr_t attr;
        pthread_t threadId1;
        int res;

        if(argc < 2)
        {
                printf("Need URL\n");
                printf("Usage:\n");
                printf("	%s rtsp://xxx.xxx.xxx.xx/CHANNEL-0\n", argv[0]);
        }
        // modify the URL to your streaming server URL
        char * url = argv[1];

        // construct stream client object
    streaming_client_construct(&client, 7000);// beging port num = 7000

        // create new client session and connet to server
        session = client_session_create_new(&client,
                        0 	/* id number */,
                        url /* the url want to stream*/,
                        1   /* blocked */,
                        400*1024 /* receive buffer size */,
                        0 	/* use UDP */,
                        0   /* reserved */
                        );


        // create data process thread
        pthread_attr_init(&attr);
        pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_JOINABLE);
        if(session)
        {
                test1.client = &client;
                test1.session = session;
                test1.datacount = x;
                test1.url = url;
                res = pthread_create(&threadId1, &attr, getdata_thread, &test1);
                if(res)
                {
                        // create get data thread fail
                        streaming_client_destruct(&client);
                        exit(1);
                }
        }

        // create session ok, start client to work
        if(session == NULL)
        {
                exit(1);
        }
        if(streaming_client_start_work(&client))
        {
                // start work fail, may be not enough memory
                streaming_client_destruct(&client);
                exit(1);
        }

        pthread_join(threadId1, NULL);

        streaming_client_stop_work(&client);
    streaming_client_destruct(&client);
    return 0;
}
