/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  sdl_display.c
 *       Compiler:  gcc
 *
 *    Description:  implement client display 
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  09/20/10 10:27:40
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
#include "streaming_client.h"

// use spct decoder lib
#include "spct_decoder.h"

#define FF_QUIT_EVENT    (SDL_USEREVENT + 2)

const char program_name[] = "sdl_display";
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
	Streaming_Client client; // streaming clent obj
	C_Session * session;	 // connect session
	TransStates trans; 

	SDL_Surface * screen;
	SDL_Overlay * bmp;
	int width, height;	/* source height & width */

	SDL_Thread * dispthread;
	int stoped;
} DispStates;


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
	C_Session * session = sta->session;
	int ret;
	SDL_Event event;
	struct timeval timeold;
	struct timeval timenew;
	unsigned char * gotbuf;
	int  gotsize;
	int passecond;

	Decode_Context videodecoder;
	Decode_Context audiodecoder;
	Spct_Data * data = NULL;
	int begindecode = 0;
	event.type = FF_QUIT_EVENT;

	// init video codec decoder
	ret = spct_decode_open(SPCT_CODEC_H264, &videodecoder, 0);
	if(ret)
	{
		fprintf(stderr, " Open h264 decoder fail\n");
		sta->stoped = 1;
	}

	// init audio codec decoder
	ret = spct_decode_open(SPCT_CODEC_G723, &audiodecoder, 0);
	if(ret)
	{
		fprintf(stderr, " Open g723 decoder fail\n");
		sta->stoped = 1;
	}
	gettimeofday(&timeold, NULL);
	while(sta->stoped == 0)
	{
		// must free data
		if(data != NULL)
			client_session_free_data(session, data);

		ret = client_session_get_data(session, &data);
		if(ret != 0)
		{
			// get data fail, connect error, or server teardown
			printf("get error data\n");
			break;
		}

		if(data == NULL)
		{
			// if the session is not blocked the data maybe == NULL, beacusethe data not ready, read again
			usleep(4000);
			continue;
		}
		else
		{
			// do some thing to process data
			if(data->type == DATA_VIDEO)
			{
				// stat.
				sta->trans.videoframecount++;
				sta->trans.videototalbytes += data->size;
				// calculate frame rate
				if(sta->trans.videoframecount %1000 == 0)
				{
					gettimeofday(&timenew, NULL);
					passecond = timenew.tv_sec - timeold.tv_sec;
					printf("get frame %d, pass time %d, frame rate %d\n", sta->trans.videoframecount, passecond, 1000/passecond);
				   timeold = timenew;	
				}
				if(data->flags == DATA_VIDEO_I)
				{
					begindecode = 1; // decode first frame must I frame
				}
				// got i frme decode video frame
				if(begindecode)
				{
					ret = spct_decode(&videodecoder, data->data,
							data->size, &gotbuf, &gotsize);
					if(ret < 0) // decode error
					{
						break;
					}
					if(gotsize == 0) // decoder need more data
					{
						printf("video decoder need more data, data size = %d, ret = %d\n", data->size,ret);
						continue;
					}

					// display one frame
					display_video(sta, gotbuf);
				}
			}
			else if(data->type == DATA_AUDIO)
			{
				// stat.
				sta->trans.audioframecount++;
				sta->trans.audiototalbytes += data->size;

				// decode audio
				ret = spct_decode(&audiodecoder, data->data,
						data->size, &gotbuf, &gotsize);
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


	vp.url = "rtsp://172.20.57.30/CHANNEL-0";//argv[1];


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


	vp.width = 704; 	// PAL D1
	vp.height = 576;	// PAL D1
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

	// init streaming client
	ret =streaming_client_construct(&vp.client,	8000);// port number is 0, use default port num
	if(ret)
	{
		fprintf(stderr, "Streaming client construct fail\n");
		error = 1;
		goto exit;
	}
	vp.session = client_session_create_new(&vp.client,
											0, // id
											vp.url, // the connect url
											1, // block
											400*1024, // receive buffer size
											0, // use UDP
											0  // reserved
											);
	if(vp.session == NULL)
	{
		fprintf(stderr, "Streaming connect fail\n");
		error = 1;
		goto exit;
	}


	vp.dispthread = SDL_CreateThread(disp_thread, &vp);
	if(!vp.dispthread)
	{
		fprintf(stderr, "SDL: create display thread fail - %s\n",SDL_GetError());
		error = 1;
		goto exit;
	}

	// streaming client start work
	if(streaming_client_start_work(&vp.client))
	{
		fprintf(stderr, "Streaming start streaming fail\n");
		error = 1;
		goto exit;
	}

	// SDL event loop
	event_loop(&vp);

exit:
	streaming_client_destruct(&vp.client);
	SDL_FreeYUVOverlay(vp.bmp);
	SDL_FreeSurface(vp.screen);
	SDL_Quit();
	return error;
}
