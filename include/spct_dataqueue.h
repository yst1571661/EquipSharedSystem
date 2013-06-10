/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_dataqueue.h
 *       Compiler:  gcc
 *
 *    Description:  data queue define
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  04/23/10 18:16:04
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */


#ifndef  SPCT_DATAQUEUE_INC
#define  SPCT_DATAQUEUE_INC

#include "spct_mem.h"
#include "spct_queue.h"
#include "spct_type.h"

typedef struct data_queue
{
	pthread_mutex_t mutex;
	pthread_mutex_t mutex1;
	int    status;
	char * header;
	char * tail;
	char ** np;
	int    type;
	int    bufsize;
	int    datanum;
	char * buf;
	int    datacount;
} Data_Queue;

#define DQ_TYPE_WAIT   0x1234
#define DQ_TYPE_NOWAIT 0x5678


/***************************************************************************
Function:  dq_construct
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_construct(Data_Queue * queue, int size, int type);

/***************************************************************************
Function:  dq_destroy
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_destruct(Data_Queue * queue);

/***************************************************************************
Function:  dq_set_used
Description: 
Input:		mod, the data work mode, 0 normol, 1 dq_get() will return thesaved data
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_set_used(Data_Queue * queue);


/***************************************************************************
Function:  dq_set_free
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_set_free(Data_Queue * queue);

/***************************************************************************
Function:  dq_change_size
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_change_size(Data_Queue * queue, int size);

/***************************************************************************
Function:  dq_in
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int dq_in(Data_Queue * queue, Spct_Data * data); 

/***************************************************************************
Function:  dq_get
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

Spct_Data * dq_get(Data_Queue * queue);


/***************************************************************************
Function:  dq_pop
Description: 
Input:
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

Spct_Data * dq_pop(Data_Queue * queue);

int dq_clear(Data_Queue * dq);

int dq_cancel(Data_Queue * dq);

#endif   /* ----- #ifndef SPCT_DATAQUEUE_INC  ----- */
