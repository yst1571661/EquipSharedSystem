/*
 * =====================================================================================
 *       Copyright (C), 2010-2020, Bridge Team
 *       Filename:  spct_queue.h
 *       Compiler:  gcc
 *
 *    Description:  defined queue function
 *         Others:  none
 *        History:
 *
 *        Version:  1.0
 *        Created:  04/23/10 14:03:18
 *         Author:  liukai , liukai@sunnorth.com.cn
 *        Company:  Sunplus Core Technology
 *
 *   Modification:
 *
 *
 * =====================================================================================
 */



#ifndef  SPCT_QUEUE_INC
#define  SPCT_QUEUE_INC

#include <pthread.h>
#include <semaphore.h>
#include "spct_mem.h"


/*
 *----------------------------------------------------------------------------------
 * queue node struct 
 *----------------------------------------------------------------------------------
 */
typedef struct queue_node
{
	struct queue_node * next;
	void  * data;
} Queue_Node;


/*
 *----------------------------------------------------------------------------------
 * queue struct . queue is fist in fist out struct. in out operation synchroniz by semaphore
 ----------------------------------------------------------------------------------
 */

typedef struct queue
{
	pthread_mutex_t mutex;
	pthread_mutex_t wait;
	int nodenum;
	Queue_Node * header;
	Queue_Node * tail;
	int status;
	Mem  mem;
} Queue;


/***************************************************************************
Function:  queue_construct
Description:construct queue struct 
Input:		queue, the pointer of queue
			size, the object numbers, NULL default 10
Output:		null
Return:		0 success, others return error code
Others:		NULL
 ***************************************************************************/

int queue_construct(Queue * queue, int size);

/***************************************************************************
Function:  queue_destroy
Description: deconstruct queue struct
Input:		queue, the pointer of queue
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int queue_destruct(Queue * queue);


/***************************************************************************
Function:  queue_in
Description: add object to queue
Input:		queue, the pointer of queue
			obj, the object pointer
Output:		null
Return:		0 success, others return error code
Others:		null
 ***************************************************************************/

int queue_in(Queue * queue, void *obj);

/***************************************************************************
Function:  queue_out
Description: output objec from queue
Input:		queue, the pointer of queue
Output:		null
Return:		NULL, nothing,  others the pointer of object
Others:		null
 ***************************************************************************/

void * queue_out(Queue * queue);


void queue_cancel(Queue * queue);


#endif   /* ----- #ifndef SPCT_QUEUE_INC  ----- */

