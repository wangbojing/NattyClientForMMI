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

#ifndef __NATTY_CLIENT_DEVICE__
#define __NATTY_CLIENT_DEVICE__

#include <string.h>
#include <stdio.h>

#include "NattyAbstractClass.h"

typedef struct _FRIENDSINFO {
	//C_DEVID devid;
	U8 sockfd;
	U32 addr;
	U16 port;
	U8 isP2P;
	U8 counter;
} FriendsInfo;


typedef enum {
	LEVEL_LOGIN = 0x00,
	LEVEL_HEARTBEART = 0x01,
	LEVEL_P2PCONNECT = 0x02,
	LEVEL_P2PCONNECT_ACK = 0x03,
	LEVEL_LOGOUT = 0x04,
	LEVEL_P2PADDR = 0x05,
	LEVEL_P2PCONNECTFRIEND = 0x08,
	LEVEL_P2PCONNECT_NOTIFY = 0x09,
	LEVEL_P2PDATAPACKET = 0x0A,
	LEVEL_DATAPACKET = 0x0B,
	LEVEL_DEFAULT			= 0xFF,
} LEVEL_COMMUNICATION;

#if 0
int sendP2PHeartbeatAck(C_DEVID fromId, C_DEVID toId);
int sendP2PHeartbeat(C_DEVID fromId, C_DEVID toId);
int sendLoginPacket(void);
int sendP2PConnectReq(void* fTree, C_DEVID id);
int sendP2PConnectAck(C_DEVID friId, U32 ack);
int sendP2PConnectNotify(C_DEVID fromId, C_DEVID toId);
int sendP2PConnectNotifyAck(C_DEVID friId, U32 ack);
#endif

int ntyClientDevInit(void);
int sendP2PDataPacketReq(C_DEVID friId, U8 *buf, int length);
int sendP2PDataPacketAck(C_DEVID friId, U32 ack);
int sendProxyDataPacketReq(C_DEVID friId, U8 *buf, int length);
int sendProxyDataPacketAck(C_DEVID friId, U32 ack);


#endif




