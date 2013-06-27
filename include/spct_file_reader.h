/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_file_reader.h
 *       Compiler:  gcc
 *
 *    Description:  this file define file reader function
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  10/13/10 10:02:56
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */




#ifndef  SPCT_FILE_READER_INC
#define  SPCT_FILE_READER_INC


#include "spct_type.h"

// define codec type
#define SPCT_CODEC_TYPE_H264  0x10
#define SPCT_CODEC_TYPE_G723  0x20
#define SPCT_CODEC_TYPE_UNKNOW  0xff

//	define pix format
#define SPCT_PIX_FORMAT_YUV420P 0x10 
#define SPCT_PIX_FORMAT_UNKNOW  0xffff 

typedef struct spct_file_reader
{
	void * context;
} Spct_File_Reader;

typedef struct spct_file_info
{
	int video_codec;
	int audio_codec;
	int video_width;
	int video_height;
	int video_pix_fmt;
	int has_video;
	int has_audio;
	int audio_samplerate;
	int duration;
} Spct_File_Info;

/***************************************************************************
Function:  file_reader_construct
Description:init file reader struct 
Input:		reader, the file reader struct
			filename, the file name
Output:		null
Return:		0 success, others fail
Others:		null
 ***************************************************************************/

int file_reader_construct(Spct_File_Reader * reader, char * filename);


/***************************************************************************
Function:  file_reader_destruct
Description:destory file reader 
Input:		reader, destruct file reader struct
Output:		null
Return:		0 success, othters fail
Others:		null
 ***************************************************************************/

int file_reader_destruct(Spct_File_Reader * reader);


/***************************************************************************
Function:  file_reader_get_packet
Description:get packet from
Input:		reader, the file reader struct
			data, the data read from file
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int file_reader_get_packet(Spct_File_Reader * reader, Spct_Data * data);


/***************************************************************************
Function:  file_reader_get_fileinof
Description:get the file information
Input:		reader, the reader struct
			info, the file information
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int file_reader_get_fileinof(Spct_File_Reader * reader, Spct_File_Info * info);

#endif   /* ----- #ifndef SPCT_FILE_READER_INC  ----- */


