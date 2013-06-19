/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  asf_read.c
 *       Compiler:  gcc
 *
 *    Description:  implement asf file read 
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  10/13/10 16:02:45
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */


#include <stdio.h>
#include "spct_file_reader.h"
char infbuf[1024];

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

struct timeval timeold;

int main(int argc, char * argv[])
{
	int res;
	Spct_File_Reader reader;
	Spct_File_Info fileinfo;

	Spct_Data data;
	int video_packet_count;
	int i_frame_count;
	int audio_packet_count;
	char buf[256];
	FILE * videofile;
	FILE * audiofile;
	FILE * infofile;
	char * filename;
	if(argc < 2)
	{
		printf(" Need input filename\n");
		printf(" USAGE:\n");
		printf(" %s  filename\n");
		return -1;
	}
	filename = argv[1];
	video_packet_count = 0;
	audio_packet_count = 0;
	i_frame_count = 0;

	snprintf(buf, sizeof(buf), "%s_videostream", filename);
	videofile = fopen(buf, "wb+");
	if(videofile == NULL)
	{
		printf("create video stream file fail\n");
		return -1;
	}

	snprintf(buf, sizeof(buf), "%s_audiostream", filename);
	audiofile = fopen(buf, "wb+");
	if(videofile == NULL)
	{
		printf("create audio stream file fail\n");
		fclose(videofile);
		return -1;
	}

	snprintf(buf, sizeof(buf), "%s_info", filename);
	infofile = fopen(buf, "w");
	if(infofile == NULL)
	{
		printf("create audio stream file fail\n");
		fclose(videofile);
		fclose(audiofile);
		return -1;
	}

	sprintf(infbuf, "count        type        timestamp        size\n");
	fwrite(infbuf, 1, strlen(infbuf), infofile);

	// init reader struct
	res = file_reader_construct(&reader, filename);
	if(res < 0)
	{
		fclose(videofile);
		fclose(audiofile);
		return -1;
	}

	// get file information
	file_reader_get_fileinfo(&reader, &fileinfo);
	dump_fileinfo(&fileinfo);

	res = file_reader_get_packet(&reader, &data);
	while(res == 0) // res == 0 read packet ok, -1 file end
	{
		if(data.type == DATA_VIDEO)
		{
			if(video_packet_count == 0)
			{
				timeold = data.timestamp;
				sprintf(infbuf, "%d        %d        %d:%d        %d\n",
						video_packet_count,
						data.type,
						data.timestamp.tv_sec,
						data.timestamp.tv_usec,
						data.size);
			}
			else
			{
				sprintf(infbuf, "%d        %d        %d        %d\n",
						video_packet_count,
						data.type,
						(data.timestamp.tv_sec - timeold.tv_sec)* 1000000 + (data.timestamp.tv_usec -timeold.tv_usec),
						data.size);
				timeold= data.timestamp;
			}
			fwrite(infbuf, 1, strlen(infbuf), infofile);


			printf("got video size = %d\n", data.size);
			video_packet_count ++;
			if(data.flags == DATA_VIDEO_I)
				i_frame_count ++;
			// write data to file
			fwrite(data.data, 1, data.size, videofile);

			// may be you can send the data by streaming server
		}
		else if(data.type == DATA_AUDIO)
		{
			printf("got audio size = %d\n", data.size);
			audio_packet_count ++;
			// write data to file
			fwrite(data.data, 1, data.size, audiofile);

			// may be you can send the data by streaming server
		}
		res = file_reader_get_packet(&reader, &data);
	}
	fclose(videofile);
	fclose(audiofile);

	printf(" read file %s\n", filename);
	printf(" 	video paceket count: %d\n", video_packet_count);
	printf(" 		Loopi frame   count: %d\n", i_frame_count);
	printf("     audio packet count: %d\n", audio_packet_count);

	return 0;
}
