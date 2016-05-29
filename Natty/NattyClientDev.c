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

#include "NattyProtocol.h"
#include "NattyNetwork.h"
#include "NattyRBTree.h"
#include "NattyClientDev.h"


extern sockaddr_struct serveraddr;
C_DEVID devid = 0x01;

/*
 * p2p heartbeat ack
 * VERSION					1			BYTE
 * MESSAGE TYPE				1			BYTE (req, ack)
 * TYPE					1			BYTE 
 * DEVID					8			BYTE
 * ACKNUM					4			BYTE (Network Module Set Value)
 * DEST_DEVI				8			BYTE 
 * CRC 					4			BYTE (Network Module Set Value)
 * 
 * send to server addr
 */
void sendP2PHeartbeatAck(C_DEVID fromId, C_DEVID toId)  {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0, n;
	sockaddr_struct friendaddr;
	void *pNetwork = ntyNetworkInstance();

	void *pTree = ntyRBTreeInstance();
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, toId);
	if (pFriend == NULL || pFriend->isP2P == 0) {
		printf(" Client Id : %lld, P2P is not Success\n", toId);
		return ;
	}
	
	notify[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_MESSAGE_TYPE] = (U8)MSG_ACK;
	notify[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_HEARTBEAT_ACK;

	
	*(C_DEVID*)(&notify[NTY_PROTO_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&notify[NTY_PROTO_DEST_DEVID_IDX]) = toId;
	
	len = NTY_PROTO_CRC_IDX + sizeof(U32);
#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr;				
	friendaddr.sin_port = pFriend->port;
#else
	ntySetAddr(&friendaddr, pFriend->addr, pFriend->port);
#endif
	//pFriend->counter ++; //timeout count
	n = ntySendFrame(pNetwork, &friendaddr, notify, len);
}


/*
 * p2p heartbeat Packet
 * VERSION					1			BYTE
 * MESSAGE TYPE				1			BYTE (req, ack)
 * TYPE					1			BYTE 
 * DEVID					8			BYTE
 * ACKNUM					4			BYTE (Network Module Set Value)
 * DEST_DEVI				8			BYTE 
 * CRC 					4			BYTE (Network Module Set Value)
 * 
 * send to server addr
 */
void sendP2PHeartbeat(C_DEVID fromId, C_DEVID toId)  {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0, n;
	sockaddr_struct friendaddr;
	void *pNetwork = ntyNetworkInstance();

	void *pTree = ntyRBTreeInstance();
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, toId);
	if (pFriend == NULL || pFriend->isP2P == 0) {
		printf(" Client Id : %lld, P2P is not Success, state:%d\n", toId, pFriend->isP2P);
		return ;
	}
	
	notify[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_MESSAGE_TYPE] = (U8)MSG_REQ;
	notify[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_HEARTBEAT_REQ;

	
	*(C_DEVID*)(&notify[NTY_PROTO_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&notify[NTY_PROTO_DEST_DEVID_IDX]) = toId;
	
	len = NTY_PROTO_CRC_IDX + sizeof(U32);
#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr; 			
	friendaddr.sin_port = pFriend->port;
#else
	ntySetAddr(&friendaddr, pFriend->addr, pFriend->port);
#endif


	pFriend->counter ++; //timeout count
	n = ntySendFrame(pNetwork, &friendaddr, notify, len);
}



/*
 * Login Packet
 * VERSION					1			BYTE
 * MESSAGE TYPE				1			BYTE (req, ack)
 * TYPE					1			BYTE 
 * DEVID					8			BYTE
 * ACKNUM					4			BYTE (Network Module Set Value)
 * CRC 					4			BYTE (Network Module Set Value)
 * 
 * send to server addr
 */

void sendLoginPacket(void) {	
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0};	
	void *pNetwork = ntyNetworkInstance();

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_LOGIN_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]) = devid;	
	
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);				

	n = ntySendFrame(pNetwork, &serveraddr, buf, len);
	
}

/*
 * P2P Connect Req
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to friend addr
 *
 */

void sendP2PConnectReq(void* fTree, C_DEVID id) {
	//void *pRBTree = ntyRBTreeInstance();
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0};
	sockaddr_struct friendaddr;
	void *pNetwork = ntyNetworkInstance();

	FriendsInfo *client = ntyRBTreeInterfaceSearch(fTree, id);
	if (client == NULL || (client->isP2P == 1)){//
		printf(" Client is not exist or P2P State : %d\n", client->isP2P);
		return -1;
	} //

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_CONNECT_REQ;
	
	*(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]) = (C_DEVID) devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = id;
	len = NTY_PROTO_CRC_IDX+sizeof(U32);

#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr; 			
	friendaddr.sin_port = pFriend->port;
	printf("sendP2PConnectReq:%d.%d.%d.%d:%d\n", *(unsigned char*)(&friendaddr.sin_addr.s_addr), *((unsigned char*)(&friendaddr.sin_addr.s_addr)+1),													
				*((unsigned char*)(&friendaddr.sin_addr.s_addr)+2), *((unsigned char*)(&friendaddr.sin_addr.s_addr)+3),													
				friendaddr.sin_port);
#else
	ntySetAddr(&friendaddr, client->addr, client->port);
#endif
	//msgAck |= SIGNAL_P2PCONNECT_REQ;
	if (client->addr == 0 || client->port == 0
		||client->addr == 0xFFFFFFFF || client->port == 0xFFFF) {
		client->isP2P = 0;
		return -1;
	}
	n = ntySendFrame(pNetwork, &friendaddr, buf, len);
	//n = sendto(sockfd_local, buf, len, 0, (struct sockaddr *)&friendaddr, sizeof(friendaddr));

}

/*
 * P2P Connect Req
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to friend addr
 *
 */

void sendP2PConnectAck(C_DEVID friId, U32 ack) {
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0};	
	void *pNetwork = ntyNetworkInstance();
	
	sockaddr_struct friendaddr;
	void *pTree = ntyRBTreeInstance();	
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, friId);

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_CONNECT_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	
	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;	
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);	
#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr; 			
	friendaddr.sin_port = pFriend->port;
#else
	ntySetAddr(&friendaddr, pFriend->addr, pFriend->port);
#endif

	n = ntySendFrame(pNetwork, &friendaddr, buf, len);
	
}

/*
 * P2P DataPacket Req
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CONTENT COUNT				2			 BYTE 
 * CONTENT					*(CONTENT COUNT)	 BYTE 
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to friend addr
 * 
 */
void sendP2PDataPacketReq(C_DEVID friId, U8 *buf, int length) {
	int n;
	void *pNetwork = ntyNetworkInstance();

	sockaddr_struct friendaddr;
	void *pTree = ntyRBTreeInstance();	
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, friId);
#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr; 			
	friendaddr.sin_port = pFriend->port;
#else
	ntySetAddr(&friendaddr, pFriend->addr, pFriend->port);
#endif


	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2PDATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = (C_DEVID) devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = friId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	n = ntySendFrame(pNetwork, &friendaddr, buf, length);

}

/*
 * P2P DataPacket Ack
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to server addr, proxy to send one client
 * 
 */

void sendP2PDataPacketAck(C_DEVID friId, U32 ack) {
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0}; 
	void *pNetwork = ntyNetworkInstance();					

	sockaddr_struct friendaddr;
	void *pTree = ntyRBTreeInstance();	
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, friId);
#if 0
	struct sockaddr_in friendaddr;
	friendaddr.sin_family = AF_INET;
	friendaddr.sin_addr.s_addr = pFriend->addr; 			
	friendaddr.sin_port = pFriend->port;
#else
	ntySetAddr(&friendaddr, pFriend->addr, pFriend->port);
#endif


	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2PDATAPACKET_ACK;

	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);

	n = ntySendFrame(pNetwork, &friendaddr, buf, len);
	
}


/*
 * Server Proxy Data Transport
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CONTENT COUNT				2			 BYTE 
 * CONTENT					*(CONTENT COUNT)	 BYTE 
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to server addr, proxy to send one client
 * 
 */

void sendProxyDataPacketReq(C_DEVID friId, U8 *buf, int length) {
	int n = 0;
	void *pNetwork = ntyNetworkInstance();

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = (C_DEVID) devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = friId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	n = ntySendFrame(pNetwork, &serveraddr, buf, length);
	
}

/*
 * Server Proxy Data Transport
 * VERSION					1			 BYTE
 * MESSAGE TYPE			 	1			 BYTE (req, ack)
 * TYPE				 	1			 BYTE 
 * DEVID					8			 BYTE (self devid)
 * ACKNUM					4			 BYTE (Network Module Set Value)
 * DEST DEVID				8			 BYTE (friend devid)
 * CRC 				 	4			 BYTE (Network Module Set Value)
 * 
 * send to server addr, proxy to send one client
 * 
 */

void sendProxyDataPacketAck(C_DEVID friId, U32 ack) {
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0}; 
	void *pNetwork = ntyNetworkInstance();
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 

	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);				

	n = ntySendFrame(pNetwork, &serveraddr, buf, len);
	
}

void sendP2PConnectNotify(C_DEVID fromId, C_DEVID toId) {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0, n;
	void *pNetwork = ntyNetworkInstance();
	
	notify[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_MESSAGE_TYPE] = (U8)MSG_REQ;
	notify[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_NOTIFY_REQ;

	
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEST_DEVID_IDX]) = toId;
	
	len = NTY_PROTO_P2P_NOTIFY_CRC_IDX + sizeof(U32);

	printf("send P2P Connect Notify\n");
	n = ntySendFrame(pNetwork, &serveraddr, notify, len);
}


void sendP2PConnectNotifyAck(C_DEVID friId, U32 ack) {
	int len, n;	
	U8 buf[CACHE_BUFFER_SIZE] = {0}; 
	void *pNetwork = ntyNetworkInstance();
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_NOTIFY_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 

	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);				

	n = ntySendFrame(pNetwork, &serveraddr, buf, len);
	
}



