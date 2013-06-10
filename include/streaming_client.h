/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  streaming_client.h
 *       Compiler:  gcc
 *
 *    Description: this file define streaming client  function
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  09/09/10 11:05:05
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */



#ifndef  STREAMING_CLIENT_1_INC
#define  STREAMING_CLIENT_1_INC

#ifdef __cplusplus
extern "C" {
#endif

#include "spct_type.h"
#ifndef STREAM_ERROR_TEARDOWN 
#define STREAM_ERROR_CONNECT 0x1234
#define STREAM_ERROR_TEARDOWN 0x5678
#endif

typedef struct streaming_client
{
	void * context;
} Streaming_Client;

typedef struct c_session
{
	void * context;
} C_Session;



/***************************************************************************
Function:  streaming_client_construct
Description:construct a client context 
Input:		client, the client context struct pointer
			port, the wanted client port number, 0 use the default portnumber.
Output:		null
Return:		0 success, -1 fail
Others:		note:
			if the server is multicast , the port number must be 0, otherwise the connect is not stabilization
 ***************************************************************************/

int streaming_client_construct(Streaming_Client * client, int port);

/***************************************************************************
Function:  streaming_client_destruct
Description:destruct a streaming client context, this function will destory all client session create from this client context 
Input:		client, the streaming client context struct pointer
Output:		null
Return:		0 success, -1 fail
Others:		null
 ***************************************************************************/

int streaming_client_destruct(Streaming_Client * client);

/***************************************************************************
Function:  streaming_client_start_work
Description:start streaming client to work, this function must be called after all client session created. 
			after called this function, streaming client will start working, you can not create client session again. 
Input:		client, the streaming client context struct pointer
Output:		null
Return:		0 success, -1 fail
Others:		null
 ***************************************************************************/

int streaming_client_start_work(Streaming_Client * client);

/***************************************************************************
Function:  streaming_client_stop_work
Description: stop streaming client work
Input:		client, the streaming client stop work
Output:		null
Return:		0 success, -1 fail
Others:		null
 ***************************************************************************/

int streaming_client_stop_work(Streaming_Client * client);


/***************************************************************************
Function:  client_session_create_new
Description:create a connect section
Input:		client, the client context struct
			id, the session id,
			url,  the streaming server url
			isblock, get data function is block or not, 1 block, 0 unblock
			buffsize, the receive data buffer size
			useTCP, the connect is through TCP or not, 1 TCP, 0 UDP
			reserve, this param not used , must be 0
Output:		null
Return:		the client session pointer, NULL ctreate client session fail
Others:		note:
			if server is multicast can not use TCP connect.
 ***************************************************************************/

C_Session * client_session_create_new(Streaming_Client * client, 
		int id, char * url, int isblock, int buffsize, int useTCP, int reserve);


/***************************************************************************
Function:  client_session_get_data
Description:get data from client connect session, if pass isblock == 1 to client_session_create_new, 
			this function will be blocked when data not ready, otherwise this function not blocked
Input:		session, the streaming client session pointer
			data, the pointer point to  SPCT_DATA struct pointer, if the return is 0, the data will point to the receive data.
				  the data is SPCT_DATA struct defined in spct_type.h .
Output:		null
Return:		0 success, -1 fail
Others:		the data may be NULL, if client session is unblocked.
 ***************************************************************************/

int client_session_get_data(C_Session * session, Spct_Data ** data);

/***************************************************************************
Function:  client_session_free_data
Description: free received data . this function must use paired with client_session_get_data
Input:		session, the streaming client session pointer
			data, the receive data
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int client_session_free_data(C_Session * session, Spct_Data * data);

/***************************************************************************
Function:  client_session_teardown
Description:disconnect the client session 
Input:		client, the streaming client context
			session, the client session
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int client_session_teardown(Streaming_Client * client, C_Session * session);





#ifdef __cplusplus
}
#endif













#endif   /* ----- #ifndef STREAMING_CLIENT_1_INC  ----- */

