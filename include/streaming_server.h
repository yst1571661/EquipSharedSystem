#ifndef STREAMING_SERVER_H
#define STREAMING_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include "spct_type.h"

#if defined(__WIN32__)
#define _MSWSOCK_
#include <winsock2.h>
#include <ws2tcpip.h>
#else
#include <arpa/inet.h>
#endif

typedef struct streaming_server {
  void *context;
} Streaming_Server;

typedef struct streaming_session {
  void *context;
} Streaming_Session;

/**************************************************************/
int Log_lever;
/**************************************************************/

/***************************************************************************
Function:  streaming_server_construct
Description:construct streming server
Input:		server, the server context
			port, the rtsp port number, if port == 0, used the default rtsp port 554
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_server_construct(Streaming_Server *server, unsigned int port);

/***************************************************************************
Function:  streaming_server_destruct
Description:destruct streaming server
Input:		server, the streaming server context
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_server_destruct(Streaming_Server *server);

/***************************************************************************
Function:  streaming_server_start
Description:rtsp stream server start work
Input:		server, the server context
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_server_start(Streaming_Server *server);


/***************************************************************************
Function:  streaming_new_session
Description:get new stream session, which stream channel data
Input:		server, the server context
			channel, the id of this session
			port, the port number of this session,
				  this is a base rtp port number, rtcp = port + 1. in uncast the real port of rtp/rtcp may large then that value
			ismulticast, use the multicast streaming, 1 used
			multicastAddr, the multi cast ip address,
							only used when ismulticast == 1, if addr == 0 ,used the system random multi cast address
			audioen, 1 stream audio, 0 not stream audio,default =1
			videoen, 1 stream video, 1 not stream video,default =1
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

Streaming_Session *streaming_new_session(Streaming_Server *server, int channel, unsigned int port,
		int ismulticast, struct in_addr * multicastAddr,
		int audioen, int videoen);

/***************************************************************************
Function:  streaming_del_session
Description:delete a stream session  from rtsp server
Input:		server, the server context
			session, the session context
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_del_session(Streaming_Server *server, Streaming_Session *session);

/***************************************************************************
Function:  streaming_del_session
Description:send data to stream session
Input:		session, the stream session
			data, the spct data
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_send_data(Streaming_Session *session, Spct_Data *data);

/***************************************************************************
Function:  streaming_get_session_url
Description:get the session url
Input:		session, the stream session
			buf, the url buf
			buflen,the url buf size
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int streaming_get_session_url(Streaming_Session * session, char* buf, int buflen);

int streaming_server_stop(Streaming_Server *server);

#ifdef __cplusplus
}
#endif

#endif /* STREAMING_SERVER_H */
