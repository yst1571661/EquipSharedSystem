/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  asf_display.c
 *       Compiler:  gcc
 *
 *    Description:  implement read asf file and decode to display
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  10/15/10 10:17:54
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */




#include <SDL.h>
#include <SDL_thread.h>

#ifdef __MINGW32__
#undef main /* we don't want SDL to override our main()*/
#endif

#include <unistd.h>
#include <assert.h>

#include "spct_type.h"
// use streaming client lib
#include "spct_file_reader.h"

// use spct decoder lib
#include "spct_decoder.h"

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

const char program_name[] = "file_display";
const int program_birth_year = 2010;
int error;

typedef struct TransStates
{
        int videoframecount;
        int audioframecount;
        int videototalbytes;
        int audiototalbytes;
        int time;
} TransStates;


typedef struct DispStates {
        char * url;
        Spct_File_Reader reader; // streaming clent obj
        Spct_File_Info fileinfo;
        TransStates trans;

        SDL_Surface * screen;
        SDL_Overlay * bmp;
        int width, height;	/* source height & width */

        SDL_Thread * dispthread;
        int stoped;
} DispStates;

int dump_fileinfo(Spct_File_Info * fileinfo)
{
        if(fileinfo->has_video)
        {
                printf("stream video:\n");
                printf("       width %d \n", fileinfo->video_width);
                printf("      height %d \n", fileinfo->video_height);
                printf("       codec %d \n", fileinfo->video_codec);
                printf("  pix format %d \n", fileinfo->video_pix_fmt);

        }
        else
                printf("no video stream\n");
        if(fileinfo->has_audio)
        {
                printf("stream audio:\n");
                printf("       codec %d\n", fileinfo->audio_codec);
                printf("  samplerate %d\n", fileinfo->audio_samplerate);
        }
        else
                printf("no audio stream\n");
}

static display_audio(DispStates * vp, unsigned char * buf)
{

}

static display_video(DispStates * vp, unsigned char * buf)
{
        int size;
        unsigned char * data;
        SDL_Rect rect;
        SDL_Overlay * overlay = vp->bmp;
        size = vp->width * vp->height;
        data = buf;

        SDL_LockSurface(vp->screen);
        SDL_LockYUVOverlay(overlay);
        if (overlay->hw_overlay) {
       memcpy (overlay->pixels[0], data, size);
       memcpy (overlay->pixels[1], data + size, size/4);
       memcpy (overlay->pixels[2], data + (size*5/4), size/4);
    } else {
       overlay->pixels[0] = data;
       overlay->pixels[1] = data + size;
       overlay->pixels[2] = data + (size*5/4);
    }
        SDL_UnlockYUVOverlay(overlay);
        SDL_UnlockSurface(vp->screen);

        rect.w = vp->width;
        rect.h = vp->height;
        rect.x = rect.y = 0;

        SDL_DisplayYUVOverlay(overlay, &rect);
}

static int disp_thread(void * arg)
{
        DispStates * sta = (DispStates *) arg;
        Spct_File_Reader * reader = &sta->reader;
        int ret;
        SDL_Event event;
        struct timeval timeold;
        struct timeval timepre;
        struct timeval timecur;
        struct timeval timenew;
        unsigned char * gotbuf;
        int  gotsize;
        int passecond;
        int passusec;
        int res_time;
        struct timeval frame_time;
        int has_stream;

        Decode_Context videodecoder;
        Decode_Context audiodecoder;
        Spct_Data  data;
        int begindecode = 0;
        has_stream = 0;

        event.type = FF_QUIT_EVENT;



        if(sta->fileinfo.has_video && sta->fileinfo.video_codec == SPCT_CODEC_TYPE_H264)
        {
                // init video codec decoder
                ret = spct_decode_open(SPCT_CODEC_H264, &videodecoder, 0);
                if(ret)
                {
                        fprintf(stderr, " Open h264 decoder fail\n");
                        sta->stoped = 1;
                }
                has_stream = 1;
        }

        if(sta->fileinfo.has_audio && sta->fileinfo.audio_codec == SPCT_CODEC_TYPE_G723)
        {
                // init audio codec decoder
                ret = spct_decode_open(SPCT_CODEC_G723, &audiodecoder, 0);
                if(ret)
                {
                        fprintf(stderr, " Open g723 decoder fail\n");
                        sta->stoped = 1;
                }
                has_stream = 1;
        }

        gettimeofday(&timeold, NULL);
        while(sta->stoped == 0 && has_stream == 1)
        {
                ret = file_reader_get_packet(reader, &data);
                if(ret != 0)
                {
                        // get data fail, or end of file
                        printf("file end\n");
                        break;
                }

                // do some thing to process data
                if(data.type == DATA_VIDEO)
                {
                        // stat.
                        sta->trans.videoframecount++;
                        sta->trans.videototalbytes += data.size;
                        // calculate frame rate
                        if(data.flags == DATA_VIDEO_I)
                        {
                                begindecode = 1; // decode first frame must i frame
                        }
                        // got i frme decode video frame
                        if(begindecode)
                        {
                                ret = spct_decode(&videodecoder, data.data,
                                                data.size, &gotbuf, &gotsize);
                                if(ret < 0) // decode error
                                {
                                        break;
                                }
                                if(gotsize == 0) // decoder need more data
                                {
                                        printf("video decoder need more data, data size = %d, ret = %d\n", data.size,ret);
                                        continue;
                                }

                                if(sta->trans.videoframecount == 1)
                                {
                                        gettimeofday(&timeold, NULL);
                                        passecond = 0;
                                        passusec = 0;
                                        res_time = 0;
                                        frame_time = data.timestamp;
                                        gettimeofday(&timepre, NULL);
                                }
                                else if(sta->trans.videoframecount %1000 == 0)
                                {
                                        gettimeofday(&timenew, NULL);
                                        passecond = timenew.tv_sec - timeold.tv_sec;
                                        printf("get frame %d, pass time %d, frame rate %d\n", sta->trans.videoframecount, passecond, 1000/passecond);
                                        timeold = timenew;
                                }

                                gettimeofday(&timecur, NULL);

                                passusec = (timecur.tv_sec * 1000000 + timecur.tv_usec) - (timepre.tv_sec * 1000000 + timepre.tv_usec);
                                res_time = (data.timestamp.tv_sec * 1000000 + data.timestamp.tv_usec) - (frame_time.tv_sec * 1000000 + frame_time.tv_usec);
                                frame_time = data.timestamp;
                                timepre = timecur;
                                printf("passusec =%d, rest time = %d\n", passusec, res_time);
                                if(passecond < res_time)
                                        usleep(res_time - passecond);
                                // display one frame
                                display_video(sta, gotbuf);
                        }
                }
                else if(data.type == DATA_AUDIO)
                {
                        // stat.
                        sta->trans.audioframecount++;
                        sta->trans.audiototalbytes += data.size;

                        // decode audio
                        ret = spct_decode(&audiodecoder, data.data,
                                        data.size, &gotbuf, &gotsize);
                        if(ret < 0) // decode audio error
                        {
                                break;
                        }
                        if(gotsize == 0)
                        {
                                printf("audio decoder need more data\n");
                                continue;
                        }
                        // decode ok play it
                        display_audio(sta, gotbuf);
                }
                else
                {
                        // some error occur break
                        break;
                }
        }

        spct_decode_close(&videodecoder);
        spct_decode_close(&audiodecoder);

        // notice mainthread
        SDL_PushEvent(&event);
}

static void event_loop(DispStates * vp)
{
        SDL_Event event;
        for(;vp->stoped == 0;)
        {
                SDL_WaitEvent(&event);
                switch(event.type)
                {
                        case SDL_QUIT:
                        case FF_QUIT_EVENT:
                                vp->stoped = 1;
                            break;
                        default:
                                break;
                }
        }
}

int main(int argc, char * argv[])
{
        int ret;
        int flags;
        DispStates vp;

        memset(&vp, 0, sizeof(vp));

        if(argc < 2)
        {
                fprintf(stderr, "Need file name\n");
                printf("\n");
                printf("Usage:\n");
                printf("	%s  filename\n", argv[0]);
                exit(1);
        }
        vp.url = argv[1];

        // init init file reader
        ret =file_reader_construct(&vp.reader, vp.url);// open input file
        if(ret)
        {
                fprintf(stderr, "Open input %s file fail\n", vp.url);
                error = 1;
                goto exit;
        }

        file_reader_get_fileinfo(&vp.reader, &vp.fileinfo);
        dump_fileinfo(&vp.fileinfo);

        flags = SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_TIMER;
#if !defined(__MINGW32__)
        flags |= SDL_INIT_EVENTTHREAD;
#endif
        if(SDL_Init(flags))
        {
                fprintf(stderr, "Could not initialize SDL - %s\n", SDL_GetError());
                exit(1);
        }

        SDL_EventState(SDL_ACTIVEEVENT, SDL_IGNORE);
        SDL_EventState(SDL_SYSWMEVENT, SDL_IGNORE);
        SDL_EventState(SDL_USEREVENT, SDL_IGNORE);


        vp.width = vp.fileinfo.video_width; 	// video width
        vp.height = vp.fileinfo.video_height;	// vide height
        // create surface
        vp.screen = SDL_SetVideoMode(vp.width, vp.height, 0, 0);
        if(!vp.screen)
        {
                fprintf(stderr, "SDL: could not set video mode - exiting\n");
                error = 1;
                goto exit;
        }

        // create overlay
        vp.bmp = SDL_CreateYUVOverlay(vp.width, vp.height, SDL_IYUV_OVERLAY, vp.screen);

        vp.dispthread = SDL_CreateThread(disp_thread, &vp);
        if(!vp.dispthread)
        {
                fprintf(stderr, "SDL: create display thread fail - %s\n",SDL_GetError());
                error = 1;
                goto exit;
        }

        // SDL event loop
        event_loop(&vp);

exit:
        file_reader_destruct(&vp.reader);
        SDL_FreeYUVOverlay(vp.bmp);
        SDL_FreeSurface(vp.screen);
        SDL_Quit();
        return error;
}
