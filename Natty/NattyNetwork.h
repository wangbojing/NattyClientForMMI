/*
 *  Author : WangBoJing , email : 1989wangbojing@gmail.com
 * 
 *  Copyright Statement:
 *  --------------------
 *  This software is protected by Copyright and the information contained
 *  herein is confidential. The software may not be copied and the information
 *  contained herein may not be used or disclosed except with the written
 *  permission of Author. (C) 2016
 * 
 *
 
****       *****
  ***        *
  ***        *                         *               *
  * **       *                         *               *
  * **       *                         *               *
  *  **      *                        **              **
  *  **      *                       ***             ***
  *   **     *       ******       ***********     ***********    *****    *****
  *   **     *     **     **          **              **           **      **
  *    **    *    **       **         **              **           **      *
  *    **    *    **       **         **              **            *      *
  *     **   *    **       **         **              **            **     *
  *     **   *            ***         **              **             *    *
  *      **  *       ***** **         **              **             **   *
  *      **  *     ***     **         **              **             **   *
  *       ** *    **       **         **              **              *  *
  *       ** *   **        **         **              **              ** *
  *        ***   **        **         **              **               * *
  *        ***   **        **         **     *        **     *         **
  *         **   **        **  *      **     *        **     *         **
  *         **    **     ****  *       **   *          **   *          *
*****        *     ******   ***         ****            ****           *
                                                                       *
                                                                      *
                                                                  *****
                                                                  ****


 *
 */



#ifndef __NATTY_NETWORK_H__
#define __NATTY_NETWORK_H__

#include "soc_api.h"

#include "NattyAbstractClass.h"


#define CACHE_BUFFER_SIZE	1048

#define HEARTBEAT_TIMEOUT		25
#define P2P_HEARTBEAT_TIMEOUT	60
#define P2P_HEARTBEAT_TIMEOUT_COUNTR	5

#define RESEND_TIMEOUT			200*1000

typedef struct _NETWORK {
	const void *_;
	U8	sockfd;
	sockaddr_struct addr;
	int length;	
	HANDLE_TIMER onAck;
	U32 ackNum;
	U8 buffer[CACHE_BUFFER_SIZE];
#if 1 //For mmi
	U32 accountId;
#endif
	//void *timer;
} Network;


typedef struct _NETWORKOPERA {
	size_t size;
	void* (*ctor)(void *_self);
	void* (*dtor)(void *_self);
	int (*send)(void *_self, sockaddr_struct *to, U8 *buf, int len);
	int (*recv)(void *_self, U8 *buf, int len, sockaddr_struct *from);
	int (*resend)(void *_self);
} NetworkOpera;


void *ntyNetworkInstance(void);
int ntySendFrame(void *self, sockaddr_struct *to, U8 *buf, int len);
int ntyRecvFrame(void *self, U8 *buf, int len, sockaddr_struct *from);
void ntySetAddr(sockaddr_struct *addr, U32 addrNum, U16 port);





#endif




