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

static U8 addrArray[4] = {112,93,116,188};
static U16 addrPort = 8888;
sockaddr_struct serveraddr;


extern char YxAppGetSimOperator(char simId);
extern U32 YxAppDtcntMakeDataAcctId(char simId,char *apnName,char apnType,U8 *appId);

U8 ntyGetSocket(void *self);
U32 ntyGetAccountId(void *self);
U8 ntyGetReqType(void *self);
C_DEVID ntyGetDestDevId(void *self);





#define NTY_CRCTABLE_LENGTH			256
#define NTY_CRC_KEY		0x04c11db7ul
static U32 u32CrcTable[NTY_CRCTABLE_LENGTH] = {0};
void ntyGenCrcTable(void) {	
	U16 i,j;	
	U32 u32CrcNum = 0;	
	
	for (i = 0;i < NTY_CRCTABLE_LENGTH;i ++) {		
		u32CrcNum = (i << 24);		
		for (j = 0;j < 8;j ++) {			
			if (u32CrcNum & 0x80000000L) {				
				u32CrcNum = (u32CrcNum << 1) ^ NTY_CRC_KEY;
			} else {				
				u32CrcNum = (u32CrcNum << 1);			
			}		
		}		
		u32CrcTable[i] = u32CrcNum;	
	}
}

U32 ntyGenCrcValue(U8 *buf, int length) {	
	U32 u32CRC = 0xFFFFFFFF;		
	while (length -- > 0) {		
		u32CRC = (u32CRC << 8) ^ u32CrcTable[((u32CRC >> 24) ^ *buf++) & 0xFF];	
	}	
	return u32CRC;
}




void ntySetAddr(sockaddr_struct *addr, U32 addrNum, U16 port) {
	addr->addr[0] = *(U8*)(&addrNum);
	addr->addr[1] = *((U8*)(&addrNum)+1);
	addr->addr[2] = *((U8*)(&addrNum)+2);
	addr->addr[3] = *((U8*)(&addrNum)+3);

	addr->port = port;
	addr->addr_len = 0x04;
	addr->sock_type = SOC_SOCK_DGRAM;
}

static int ntyUdpCreate(void* self)
{
    U8 ret;
    U8 sock;
    U8 val;
	U8 appid;
    //sockaddr_struct addr;

	Network *network = self;
	U8     apn_check = (U8)YxAppGetSimOperator(YX_APP_SIM1);
	U32    account_id = 0;
    if((apn_check==MSIM_OPR_UNKOWN) || (apn_check==MSIM_OPR_NONE))
        return -1;
    //apn_check = (apn != MAPN_WAP && apn != MAPN_NET && apn != MAPN_WIFI);
	//if(apn_check)
	//	return 0;
	//yxNetContext_ptr.apn = apn;
   // yxNetContext_ptr.port = port;
	account_id = YxAppDtcntMakeDataAcctId(YX_APP_SIM1, NULL, MAPN_NET, &appid);
	if(account_id==0)
	{
		account_id = 0;
		//YxAppCloseAccount();
		return -1;
	}
	network->accountId = account_id;
	//yxNetContext_ptr.account_id = account_id;

    //create udp socket
    sock = soc_create(SOC_PF_INET, SOC_SOCK_DGRAM, 0, MOD_MMI, network->accountId);
    if (sock < 0) {
        return -2;
    }
    
    val = SOC_READ | SOC_WRITE | SOC_CONNECT |SOC_CLOSE;
    ret = soc_setsockopt(sock, SOC_ASYNC, &val, sizeof(val));
    if (ret != SOC_SUCCESS) {
        soc_close(sock);
        kal_prompt_trace(MOD_YXAPP, "set SOC_ASYNC failed");
        return -3;
    }

    val = KAL_TRUE;
    ret = soc_setsockopt(sock, SOC_NBIO, &val, sizeof(val));
    if (ret != SOC_SUCCESS) {
        soc_close(sock);
        kal_prompt_trace(MOD_YXAPP, "set SOC_NBIO failed");
        return -3;
    }
  
    return sock;
}

static U8 ntyUdpCallback(void * data)
{
    app_soc_notify_ind_struct * ind = (app_soc_notify_ind_struct *)data;
	//U8 sock = 0;
    int ret;
    Network *network = ntyNetworkInstance();
	U8 buf[CACHE_BUFFER_SIZE] = {0};
	sockaddr_struct addr;

    if (NULL == ind)
    {
        return MMI_FALSE;
    }
    //kwp_debug_print("kwp_udp_callback: event_type=%d", ind->event_type);
	//sock = ntyGetSocket(network);
    if (ind->socket_id == ntyGetSocket(network)) 
    {
        switch (ind->event_type)
        {
        case SOC_WRITE:
            break;
        case SOC_READ:
            {
                do
                {
                	ret = ntyRecvFrame(network, buf, CACHE_BUFFER_SIZE, &addr);
                    //ret = soc_recvfrom(kwp_sock, protocol_recv_buf, PROTO_BUFF_SIZE, 0, &kwp_fromaddr);
                    //kwp_debug_print("soc_recvfrom len=%d from %d.%d.%d.%d", ret,
                    //                kwp_fromaddr.addr[0],kwp_fromaddr.addr[1],kwp_fromaddr.addr[2],kwp_fromaddr.addr[3]);
                    if (ret > 0) {
                        kal_prompt_trace(MOD_YXAPP,"%d.%d.%d.%d:%d size:%d --> %x\n", addr.addr[0], addr.addr[1],	
							addr.addr[2], addr.addr[3],	 addr.port, ret, buf[NTY_PROTO_TYPE_IDX]);
                    }
                    //kwp_debug_print("soc_recvfrom rx ret = %d", ret);
                }while(ret != SOC_WOULDBLOCK);
            }break;
        case SOC_CLOSE:
            {
            }break;
        default:
            {
            }break;
        }
    }
    
    return MMI_TRUE;
}


static void ntyMessageOnAck(void) {
	
}

static void* ntyNetworkCtor(void *_self) {
	Network *network = _self;
	network->onAck = ntyMessageOnAck;
	network->ackNum = 1;

#if 1 //Socket Init	
	mmi_frm_set_protocol_event_handler(MSG_ID_APP_SOC_NOTIFY_IND, (PsIntFuncPtr)ntyUdpCallback, MMI_TRUE);
	network->sockfd = ntyUdpCreate(_self);
	if (network->sockfd < 0) {
		//error(" ERROR opening socket");
		kal_prompt_trace(MOD_YXAPP, "ERROR opening socket");
	}
	//init Server addr
	ntySetAddr(&serveraddr, *((U32*)addrArray), addrPort);
#endif
	
	return network;
}

static void* ntyNetworkDtor(void *_self) {
	return _self;
}



static U32 ntyNetworkResendFrame(void *_self) {
	U32 ret;
	Network *network = _self;
	ret = soc_sendto(network->sockfd, network->buffer, (kal_int32)network->length, 0, &network->addr);
    //kwp_debug_print("ntp send len %d", ret);
    if (0 > ret)
    {
        if (SOC_WOULDBLOCK == ret)
        {
            kal_prompt_trace(MOD_YXAPP,"SOC_WOULDBLOCK");
        }
        else
        {
            kal_prompt_trace(MOD_YXAPP,"send data failed");
        }
        
    }
	return ret;
}


static int ntyNetworkSendFrame(void *_self, sockaddr_struct *to, U8 *buf, int len) {
	//ntyStartTimer();
	U32 ret;
	Network *network = _self;	
	//void* pTimer = ntyNetworkTimerInstance();
	if (buf[NTY_PROTO_MESSAGE_TYPE] != MSG_ACK) {
		//ntyStartTimer(pTimer, network->onAck);	
		StartTimer(NATTY_NETWORK_COMFIRMED_TIMER, RESEND_TIMEOUT, network->onAck);
		network->ackNum ++;
	}
	
	memcpy(&network->addr, to, sizeof(sockaddr_struct));
	bzero(network->buffer, CACHE_BUFFER_SIZE);
	memcpy(network->buffer, buf, len);
	
	if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_REQ) {
		*(U32*)(&network->buffer[NTY_PROTO_ACKNUM_IDX]) = network->ackNum;
	}
	network->length = len;
	*(U32*)(&network->buffer[len-sizeof(U32)]) = ntyGenCrcValue(network->buffer, len-sizeof(U32));

	//printf("ntyNetworkSendFrame : %x\n", buf[NTY_PROTO_TYPE_IDX]);

	ret = soc_sendto(network->sockfd, network->buffer, (kal_int32)network->length, 0, &network->addr);
    //kwp_debug_print("ntp send len %d", ret);
    if (0 > ret)
    {
        if (SOC_WOULDBLOCK == ret)
        {
            kal_prompt_trace(MOD_YXAPP,"SOC_WOULDBLOCK");
        }
        else
        {
            kal_prompt_trace(MOD_YXAPP,"send data failed");
        }
        
    }
}

static U32 ntyNetworkRecvFrame(void *_self, U8 *buf, int len, sockaddr_struct *from) {
	//ntyStartTimer();
	U32 ret;
	int n = 0;
	int clientLen = sizeof(sockaddr_struct);
	sockaddr_struct addr = {0};
	U32 ackNum;

	Network *network = _self;

	ret = soc_recvfrom(network->sockfd, buf, CACHE_BUFFER_SIZE, 0, &addr);
	ackNum = *(U32*)(&buf[NTY_PROTO_ACKNUM_IDX]);

	memcpy(from, &addr, clientLen);
	if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_ACK) { //recv success		
		if (ackNum == network->ackNum + 1) {
			// CRC 
			
			// stop timer
			StopTimer(NATTY_NETWORK_COMFIRMED_TIMER);
			return n;	
		} else {
			return -1;
		}
	} else if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_RET) {
		
		StopTimer(NATTY_NETWORK_COMFIRMED_TIMER);
		//have send object
	} else if (buf[NTY_PROTO_MESSAGE_TYPE] == MSG_UPDATE) {
		StopTimer(NATTY_NETWORK_COMFIRMED_TIMER);
		
	}

	return ret;
	
}

static const NetworkOpera ntyNetworkOpera = {
	sizeof(Network),
	ntyNetworkCtor,
	ntyNetworkDtor,
	ntyNetworkSendFrame,
	ntyNetworkRecvFrame,
	ntyNetworkResendFrame,
};

const void *pNtyNetworkOpera = &ntyNetworkOpera;

static void *pNetworkOpera = NULL;

void *ntyNetworkInstance(void) {
	if (pNetworkOpera == NULL) {
		pNetworkOpera = New(pNtyNetworkOpera);
	}
	return pNetworkOpera;
}

void ntyNetworkRelease(void *self) {	
	Delete(self);
}


int ntySendFrame(void *self, sockaddr_struct *to, U8 *buf, int len) {
	const NetworkOpera *const * pNetworkOpera = self;

	if (self && (*pNetworkOpera) && (*pNetworkOpera)->send) {
		return (*pNetworkOpera)->send(self, to, buf, len);
	}
	return -1;
}

int ntyRecvFrame(void *self, U8 *buf, int len, sockaddr_struct *from) {
	const NetworkOpera *const * pNetworkOpera = self;

	if (self && (*pNetworkOpera) && (*pNetworkOpera)->recv) {
		return (*pNetworkOpera)->recv(self, buf, len, from);
	}
	return -2;
}

U8 ntyGetSocket(void *self) {
	Network *network = self;
	return network->sockfd;
}

U8 ntyGetReqType(void *self) {
	Network *network = self;
	return network->buffer[NTY_PROTO_TYPE_IDX];
}

C_DEVID ntyGetDestDevId(void *self) {
	Network *network = self;
	return *(C_DEVID*)(&network->buffer[NTY_PROTO_DEST_DEVID_IDX]);
}

U32 ntyGetAccountId(void *self) {
	Network *network = self;
	return *(C_DEVID*)(&network->accountId);
}

