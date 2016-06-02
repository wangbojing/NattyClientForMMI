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

#include "YxBasicApp.h"
#include "TimerEvents.h"


#include "NattyProtocol.h"
#include "NattyNetwork.h"
#include "NattyRBTree.h"
#include "NattyClientDev.h"


extern sockaddr_struct serveraddr;
C_DEVID devid = 0x01;
C_DEVID friendId = 0x00;
static int level = LEVEL_LOGIN;
static int times = 0;

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
int sendP2PHeartbeatAck(C_DEVID fromId, C_DEVID toId)  {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0;
	sockaddr_struct friendaddr;
	void *pNetwork = ntyNetworkInstance();

	void *pTree = ntyRBTreeInstance();
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, toId);
	if (pFriend == NULL || pFriend->isP2P == 0) {
		printf(" Client Id : %lld, P2P is not Success\n", toId);
		return -1;
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
	return ntySendFrame(pNetwork, &friendaddr, notify, len);
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
int sendP2PHeartbeat(C_DEVID fromId, C_DEVID toId)  {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0;
	sockaddr_struct friendaddr;
	void *pNetwork = ntyNetworkInstance();

	void *pTree = ntyRBTreeInstance();
	FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, toId);
	if (pFriend == NULL || pFriend->isP2P == 0) {
		printf(" Client Id : %lld, P2P is not Success, state:%d\n", toId, pFriend->isP2P);
		return -1;
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
	return ntySendFrame(pNetwork, &friendaddr, notify, len);
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

int sendLoginPacket(void) {	
	int len;	
	U8 buf[CACHE_BUFFER_SIZE] = {0};	
	void *pNetwork = ntyNetworkInstance();

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_LOGIN_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]) = devid;	
	
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);				

	return ntySendFrame(pNetwork, &serveraddr, buf, len);
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

int sendP2PConnectReq(void* fTree, C_DEVID id) {
	//void *pRBTree = ntyRBTreeInstance();
	int len;	
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
		return -2;
	}
	return ntySendFrame(pNetwork, &friendaddr, buf, len);
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

int sendP2PConnectAck(C_DEVID friId, U32 ack) {
	int len;	
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

	return ntySendFrame(pNetwork, &friendaddr, buf, len);
	
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
int sendP2PDataPacketReq(C_DEVID friId, U8 *buf, int length) {
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

	return ntySendFrame(pNetwork, &friendaddr, buf, length);

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

int sendP2PDataPacketAck(C_DEVID friId, U32 ack) {
	int len;	
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

	return ntySendFrame(pNetwork, &friendaddr, buf, len);
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

int sendProxyDataPacketReq(C_DEVID friId, U8 *buf, int length) {
	void *pNetwork = ntyNetworkInstance();

	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_REQ;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEVID_IDX]) = (C_DEVID) devid;
	*(C_DEVID*)(&buf[NTY_PROTO_DATAPACKET_DEST_DEVID_IDX]) = friId;
	
	*(U16*)(&buf[NTY_PROTO_DATAPACKET_CONTENT_COUNT_IDX]) = (U16)length;
	length += NTY_PROTO_DATAPACKET_CONTENT_IDX;
	length += sizeof(U32);

	return ntySendFrame(pNetwork, &serveraddr, buf, length);
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

int sendProxyDataPacketAck(C_DEVID friId, U32 ack) {
	int len;	
	U8 buf[CACHE_BUFFER_SIZE] = {0}; 
	void *pNetwork = ntyNetworkInstance();
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_DATAPACKET_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 

	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);				

	return ntySendFrame(pNetwork, &serveraddr, buf, len);
	
}

int sendP2PConnectNotify(C_DEVID fromId, C_DEVID toId) {
	U8 notify[NTY_LOGIN_ACK_LENGTH] = {0};
	int len = 0;
	void *pNetwork = ntyNetworkInstance();
	
	notify[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	notify[NTY_PROTO_MESSAGE_TYPE] = (U8)MSG_REQ;
	notify[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_NOTIFY_REQ;

	
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]) = fromId;
	*(C_DEVID*)(&notify[NTY_PROTO_P2P_NOTIFY_DEST_DEVID_IDX]) = toId;
	
	len = NTY_PROTO_P2P_NOTIFY_CRC_IDX + sizeof(U32);

	printf("send P2P Connect Notify\n");
	return ntySendFrame(pNetwork, &serveraddr, notify, len);
}


int sendP2PConnectNotifyAck(C_DEVID friId, U32 ack) {
	int len;	
	U8 buf[CACHE_BUFFER_SIZE] = {0}; 
	void *pNetwork = ntyNetworkInstance();
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_P2P_NOTIFY_ACK;
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_ACK; 

	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = (C_DEVID) devid;		
	*(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]) = ack+1;
	*(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]) = friId;
	
	len = NTY_PROTO_CRC_IDX+sizeof(U32);				

	return ntySendFrame(pNetwork, &serveraddr, buf, len);
	
}

/*
 * Send Packet Type : 
 * LOGIN
 * Heart Beat
 * LOGOUT
 * 
 * at same time
 * Heart Beat / P2P Heart Beat / Send Packet / LOGOUT
 * 
 */

static U8 ntyUdpCallback(void * data)
{
    app_soc_notify_ind_struct * ind = (app_soc_notify_ind_struct *)data;
	//U8 sock = 0;
    int ret;
    Network *network = ntyNetworkInstance();
	U8 buf[CACHE_BUFFER_SIZE] = {0};
	sockaddr_struct addr;

    if (NULL == ind) {
        return MMI_FALSE;
    }
    //kwp_debug_print("kwp_udp_callback: event_type=%d", ind->event_type);
	//sock = ntyGetSocket(network);
    if (ind->socket_id == ntyGetSocket(network)) {
        switch (ind->event_type) {
        case SOC_WRITE:
            break;
        case SOC_READ: {
                do {
                	ret = ntyRecvFrame(network, buf, CACHE_BUFFER_SIZE, &addr);
                    //ret = soc_recvfrom(kwp_sock, protocol_recv_buf, PROTO_BUFF_SIZE, 0, &kwp_fromaddr);
                    //kwp_debug_print("soc_recvfrom len=%d from %d.%d.%d.%d", ret,
                    //                kwp_fromaddr.addr[0],kwp_fromaddr.addr[1],kwp_fromaddr.addr[2],kwp_fromaddr.addr[3]);
                    if (ret > 0) {
                        kal_prompt_trace(MOD_YXAPP,"%d.%d.%d.%d:%d size:%d --> %x\n", addr.addr[0], addr.addr[1],	
							addr.addr[2], addr.addr[3],	 addr.port, ret, buf[NTY_PROTO_TYPE_IDX]);

						if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_LOGIN_ACK) {
						//NTY_PROTO_LOGIN_ACK
							int i = 0;
							
							int count = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDS_COUNT_IDX]);
							void *pTree = ntyRBTreeInstance();

							for (i = 0;i < count;i ++) {
								C_DEVID friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]);

								FriendsInfo *friendInfo = ntyRBTreeInterfaceSearch(pTree, friendId);
								if (NULL == friendInfo) {
									#if 0
									FriendsInfo *pFriend = (FriendsInfo*)malloc(sizeof(FriendsInfo));
									#else
									FriendsInfo *pFriend = (FriendsInfo*)OslMalloc(sizeof(FriendsInfo));
									
									#endif
									if (pFriend == NULL) {
										kal_prompt_trace(MOD_YXAPP," malloc FriendsInfo failed");
										break;
									}
									pFriend->sockfd = ntyGetSocket(network);
									pFriend->isP2P = 0;
									pFriend->counter = 0;
									pFriend->addr = *(U32*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
									pFriend->port = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
									ntyRBTreeInterfaceInsert(pTree, friendId, pFriend);
								} else {
									friendInfo->sockfd = ntyGetSocket(network);
									friendInfo->isP2P = 0;
									friendInfo->counter = 0;
									friendInfo->addr = *(U32*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
									friendInfo->port = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
								}					
							}
							
							level = LEVEL_P2PCONNECT_NOTIFY;			
							//printf("NTY_PROTO_LOGIN_ACK\n");
							
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_NOTIFY_ACK) {
						//NTY_PROTO_P2P_NOTIFY_ACK
							//P2PConnect Notify Success
							//void *pTree = ntyRBTreeInstance();
							//sendP2PConnectReq(pTree, friendId);
							//
							void *pTree = ntyRBTreeInstance();
							FriendsInfo *friendInfo = NULL;
							friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]);
							friendInfo = ntyRBTreeInterfaceSearch(pTree, friendId);

							kal_prompt_trace(MOD_YXAPP, "%d.%d.%d.%d:%d\n", *(unsigned char*)(&friendInfo->addr), *((unsigned char*)(&friendInfo->addr)+1),	
							*((unsigned char*)(&friendInfo->addr)+2), *((unsigned char*)(&friendInfo->addr)+3),	 friendInfo->port
							);
							
							kal_prompt_trace(MOD_YXAPP, " Start to Connect P2P client\n");
							level = LEVEL_P2PCONNECTFRIEND;
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_HEARTBEAT_ACK) {
						//NTY_PROTO_HEARTBEAT_ACK
							if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_UPDATE) {
								int i = 0;
								
								int count = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDS_COUNT_IDX]);
								void *pTree = ntyRBTreeInstance();

								for (i = 0;i < count;i ++) {
									C_DEVID friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_DEVID_IDX(i)]);

									FriendsInfo *friendInfo = ntyRBTreeInterfaceSearch(pTree, friendId);
									if (NULL == friendInfo) {
										#if 0
										FriendsInfo *pFriend = (FriendsInfo*)malloc(sizeof(FriendsInfo));
										#else
										FriendsInfo *pFriend = (FriendsInfo*)OslMalloc(sizeof(FriendsInfo));
										#endif
										if (pFriend == NULL) {
											kal_prompt_trace(MOD_YXAPP," malloc FriendsInfo failed");
											break;
										}
										pFriend->sockfd = ntyGetSocket(network);
										pFriend->isP2P = 0;
										pFriend->counter = 0;
										pFriend->addr = *(U32*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
										pFriend->port = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
										ntyRBTreeInterfaceInsert(pTree, friendId, pFriend);
									} else {
										friendInfo->sockfd = ntyGetSocket(network);
										friendInfo->isP2P = 0;
										friendInfo->counter = 0;
										friendInfo->addr = *(U32*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_ADDR_IDX(i)]);
										friendInfo->port = *(U16*)(&buf[NTY_PROTO_LOGIN_ACK_FRIENDSLIST_PORT_IDX(i)]);
									}					
								}
								level = LEVEL_P2PCONNECT_NOTIFY;
							}
							
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_CONNECT_REQ) {
							//NTY_PROTO_P2P_CONNECT_REQ

							U32 ack = *(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]);	
							void *pTree = ntyRBTreeInstance();	
							FriendsInfo *pFriend = NULL;
							
							friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]);

							pFriend = ntyRBTreeInterfaceSearch(pTree, friendId);
							if (pFriend != NULL) {
								pFriend->sockfd = ntyGetSocket(network);
								pFriend->addr = *((U32*)addr.addr);//addr.sin_addr.s_addr;
								pFriend->port = addr.port;//addr.sin_port;
								pFriend->isP2P = 0;
								pFriend->counter = 0;

								kal_prompt_trace(MOD_YXAPP, " P2P client:%lld request connect\n", friendId);
							} else {
							#if 0
								FriendsInfo *friendInfo = (FriendsInfo*)malloc(sizeof(FriendsInfo));
							#else
								FriendsInfo *friendInfo = (FriendsInfo*)OslMalloc(sizeof(FriendsInfo));
							#endif
								if (pFriend == NULL) {
									kal_prompt_trace(MOD_YXAPP," malloc FriendsInfo failed");
									break;
								}
								friendInfo->sockfd = ntyGetSocket(network);
								friendInfo->addr = *((U32*)addr.addr);//addr.sin_addr.s_addr;
								friendInfo->port = addr.port;//addr.sin_port;
								friendInfo->isP2P = 0;
								friendInfo->counter = 0;
								ntyRBTreeInterfaceInsert(pTree, friendId, friendInfo);
							}

							sendP2PConnectAck(friendId, ack);	
							level = LEVEL_P2PDATAPACKET;				
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_CONNECT_ACK) {
							//NTY_PROTO_P2P_CONNECT_ACK
							
							//level = LEVEL_P2PDATAPACKET;
							void *pTree = ntyRBTreeInstance();	
							FriendsInfo *pFriend = NULL;
							friendId = *(C_DEVID*)(&buf[NTY_PROTO_LOGIN_REQ_DEVID_IDX]);	

							pFriend = ntyRBTreeInterfaceSearch(pTree, friendId);
							if (pFriend != NULL) {
								pFriend->isP2P = 1; 
							}				
							
							kal_prompt_trace(MOD_YXAPP, " P2P client %lld connect Success\n", friendId);
							level = LEVEL_P2PDATAPACKET;
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_NOTIFY_REQ) {	
							//NTY_PROTO_P2P_NOTIFY_REQ

							void *pTree = ntyRBTreeInstance();
							FriendsInfo *pFriend = NULL;
							U32 ack = *(U32*)(&buf[NTY_PROTO_P2P_NOTIFY_ACKNUM_IDX]);
							friendId =  *(C_DEVID*)(&buf[NTY_PROTO_P2P_NOTIFY_DEVID_IDX]);
							kal_prompt_trace(MOD_YXAPP, " P2P Connect Notify: %lld\n", friendId);

							pFriend = ntyRBTreeInterfaceSearch(pTree, friendId);
							if (pFriend != NULL) {
								pFriend->sockfd = ntyGetSocket(network);
								pFriend->addr = *(U32*)(&buf[NTY_PROTO_P2P_NOTIFY_IPADDR_IDX]);
								pFriend->port = *(U16*)(&buf[NTY_PROTO_P2P_NOTIFY_IPPORT_IDX]);
								pFriend->isP2P = 0;
								pFriend->counter = 0;
								kal_prompt_trace(MOD_YXAPP, " P2P client:%lld\n", friendId);
								kal_prompt_trace(MOD_YXAPP, "%d.%d.%d.%d:%d\n", *(unsigned char*)(&pFriend->addr), *((unsigned char*)(&pFriend->addr)+1),	
									*((unsigned char*)(&pFriend->addr)+2), *((unsigned char*)(&pFriend->addr)+3),	 pFriend->port);
							} else {
								#if 0
								FriendsInfo *pFriend = (FriendsInfo*)malloc(sizeof(FriendsInfo));
								#else
								FriendsInfo *pFriend = (FriendsInfo*)OslMalloc(sizeof(FriendsInfo));
								
								#endif
								if (pFriend == NULL) {
									kal_prompt_trace(MOD_YXAPP," malloc FriendsInfo failed");
									break;
								}
								pFriend->sockfd = ntyGetSocket(network);
								pFriend->addr = *(U32*)(&buf[NTY_PROTO_P2P_NOTIFY_IPADDR_IDX]);
								pFriend->port = *(U16*)(&buf[NTY_PROTO_P2P_NOTIFY_IPPORT_IDX]);
								pFriend->isP2P = 0;
								pFriend->counter = 0;
								ntyRBTreeInterfaceInsert(pTree, friendId, pFriend);
							}
							//send ack to src devid
							sendP2PConnectNotifyAck(friendId, ack);
							//just now send p2p connect req
							//sendP2PConnectReq(pTree, friendId);
							level = LEVEL_P2PCONNECTFRIEND;
							
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_REQ) {
							//U16 cliCount = *(U16*)(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
							U8 data[CACHE_BUFFER_SIZE] = {0};//NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_IDX
							U16 recByteCount = *(U16*)(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
							C_DEVID friId = *(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]);
							U32 ack = *(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]);

							memcpy(data, buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, recByteCount);
							kal_prompt_trace(MOD_YXAPP, " recv:%s\n", data);

							sendProxyDataPacketAck(friId, ack);
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_DATAPACKET_ACK) {
							printf(" send success\n");
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2PDATAPACKET_REQ) {
							U8 data[CACHE_BUFFER_SIZE] = {0};
							U16 recByteCount = *(U16*)(&buf[NTY_PROTO_DATAPACKET_NOTIFY_CONTENT_COUNT_IDX]);
							C_DEVID friId = *(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]);
							U32 ack = *(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]);
							
							void *pTree = ntyRBTreeInstance();
							FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, friendId);
							if (pFriend != NULL) {
								pFriend->isP2P = 1;
							}
							
							memcpy(data, buf+NTY_PROTO_DATAPACKET_CONTENT_IDX, recByteCount);
							kal_prompt_trace(MOD_YXAPP, " P2P recv:%s\n", data);
							//sendP2PDataPacketReq(friId, data);
							sendP2PDataPacketAck(friId, ack);
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2PDATAPACKET_ACK) {
							kal_prompt_trace(MOD_YXAPP, " P2P send success\n");
							level = LEVEL_P2PDATAPACKET;
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_HEARTBEAT_REQ) {
							C_DEVID fromId = *(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]);
							C_DEVID selfId = *(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]);
							void *pTree = ntyRBTreeInstance();
							FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, fromId);
							if (pFriend != NULL) {
								pFriend->counter = 0;
								pFriend->isP2P = 1;
							}
							sendP2PHeartbeatAck(selfId, fromId);
						} else if (buf[NTY_PROTO_TYPE_IDX] == NTY_PROTO_P2P_HEARTBEAT_ACK) {
							C_DEVID fromId = *(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]);
							C_DEVID selfId = *(C_DEVID*)(&buf[NTY_PROTO_DEST_DEVID_IDX]);
							void *pTree = ntyRBTreeInstance();
							FriendsInfo *pFriend = ntyRBTreeInterfaceSearch(pTree, fromId);
							if (pFriend != NULL) {
								pFriend->counter = 0;
								pFriend->isP2P = 1;
							}
						}
					}
                    //kwp_debug_print("soc_recvfrom rx ret = %d", ret);
                } while(ret != SOC_WOULDBLOCK);
				break;
            } 
        case SOC_CLOSE: {				
				break;
            }
        default: {
				break;
            }
        }
    }
    
    return MMI_TRUE;
}

void heartbeatProc(void) {
	int len, n;	
	U8 buf[NTY_LOGIN_ACK_LENGTH] = {0};	
	
	void *pNetwork = ntyNetworkInstance();
	#if 0
	bzero(buf, NTY_LOGIN_ACK_LENGTH);
	#else
	memset(buf, 0, NTY_LOGIN_ACK_LENGTH);
	#endif

	StopTimer(NATTY_HEARTBEAT_TIMER);
	
	buf[NEY_PROTO_VERSION_IDX] = NEY_PROTO_VERSION;	
	buf[NTY_PROTO_MESSAGE_TYPE] = (U8) MSG_REQ;	
	buf[NTY_PROTO_TYPE_IDX] = NTY_PROTO_HEARTBEAT_REQ;		
	*(C_DEVID*)(&buf[NTY_PROTO_DEVID_IDX]) = devid;
	
	len = NTY_PROTO_LOGIN_REQ_CRC_IDX+sizeof(U32);
	
	n = ntySendFrame(pNetwork, &serveraddr, buf, len);

	StartTimer(NATTY_HEARTBEAT_TIMER, 25 * 1000, heartbeatProc);
}


void sendProc(void) {
	StopTimer(NATTY_NETWORK_INIT_TIMER);
	if (level >= LEVEL_P2PDATAPACKET) {
		return ;
	}

	kal_prompt_trace(MOD_YXAPP, " sendProc : %x\n", level);
	if (level == LEVEL_LOGIN) {
		sendLoginPacket();
	} else if (level == LEVEL_P2PCONNECT_NOTIFY) {
		void *pTree = ntyRBTreeInstance();
		ntyFriendsTreeTraversalNotify(pTree, devid, sendP2PConnectNotify);
	} else if (level == LEVEL_P2PCONNECTFRIEND) { 
		//
		//printf("LEVEL_P2PCONNECTFRIEND times : %d, friendId:%lld\n", times, friendId);
		if (times++ < 3) {
			void *pTree = ntyRBTreeInstance();
			sendP2PConnectReq(pTree, friendId);
		} else {
			times = 0;
			level = LEVEL_DATAPACKET;
		}
	} else if (level == LEVEL_P2PCONNECT) {
		//
		//printf("LEVEL_P2PCONNECT times : %d, friendId:%lld\n", times, friendId);
		if (times ++ < 3) {
			void *pTree = ntyRBTreeInstance();
			ntyFriendsTreeTraversal(pTree, sendP2PConnectReq);			
		} else {
			times = 0;
			level = LEVEL_DATAPACKET;
		}
	}

	//StartTimer(NATTY_NETWORK_INIT_TIMER, 200, sendProc);
}

void ConnectProc(void) {
	int res = 0;
	void *pNetwork = ntyNetworkInstance();

	StopTimer(NATTY_CONNECT_TIMER);
	res = ntyConnect(pNetwork); //creat sim apn / socket
	kal_prompt_trace(MOD_YXAPP, " ntyConnect : %d\n", res);
}

int ntyClientDevInit(void) {
	void *pNetwork = ntyNetworkInstance();
	ntySetRecvProc(pNetwork, ntyUdpCallback); //set recv proc
	
	StartTimer(NATTY_CONNECT_TIMER, 5*1000, ConnectProc);
	StartTimer(NATTY_NETWORK_INIT_TIMER, 10*1000, sendProc); //
	StartTimer(NATTY_HEARTBEAT_TIMER, 25 * 1000, heartbeatProc);
}



