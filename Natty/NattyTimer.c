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



#include "kal_public_api.h"

#include "NattyTimer.h"



static void* ntyTimerCtor(void *_self, va_list *params) {
	NetworkTimer *timer = _self;
#if 0
	pthread_mutex_t blank_mutex = PTHREAD_MUTEX_INITIALIZER;
	pthread_cond_t blank_cond = PTHREAD_COND_INITIALIZER;
	
	timer->sigNum = SIGALRM;
	timer->timerProcess = 0;
	memcpy(&timer->timer_mutex, &blank_mutex, sizeof(timer->timer_mutex));
	memcpy(&timer->timer_cond, &blank_cond, sizeof(timer->timer_cond));
#else
	timer->networkMutex = kal_create_mutex("Network Mutex");
	timer->timerProcess = 0;
#endif
	return timer;
}

static void* ntyTimerDtor(void *_self) {
	return _self;
}

static int ntyStartTimerOpera(void *_self, HANDLE_TIMER fun) {
	NetworkTimer *timer = _self;
#if 0
	struct itimerval tick;
	timer->timerFunc = fun;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = MS(TIMER_TICK);

	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = MS(TIMER_TICK);

	
	pthread_mutex_lock(&timer->timer_mutex);
	while (timer->timerProcess) {
		pthread_cond_wait(&timer->timer_cond, &timer->timer_mutex);
	}
	timer->timerProcess = 1;
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");
	}
	pthread_mutex_unlock(&timer->timer_mutex);
#else
	kal_take_mutex(timer->networkMutex);
	//while (timer->timerProcess) {
		//post cond wait
	//} //untils = 0, timer to req
	timer->timerProcess = 1;

	//StartTimer();
	timer->timerFunc = fun;
	StartTimer(NATTY_NETWORK_COMFIRMED_TIMER, RESEND_TIMEOUT, timer->timerFunc);
	
	kal_give_mutex(timer->networkMutex);
#endif
	return 0;
}


static int ntyStopTimerOpera(void *_self) {
	NetworkTimer *timer = _self;
#if 0
	struct itimerval tick;

	signal(timer->sigNum, timer->timerFunc);
	memset(&tick, 0, sizeof(tick));

	tick.it_value.tv_sec = 0;
	tick.it_value.tv_usec = 0;//MS(TIMER_TICK);

	tick.it_interval.tv_sec = 0;
	tick.it_interval.tv_usec = 0;//MS(TIMER_TICK);

	pthread_mutex_lock(&timer->timer_mutex);
	timer->timerProcess = 0;
#if 0 //mac os don't support
	pthread_cond_broadcast(&timer->timer_cond);
#else
	pthread_cond_signal(&timer->timer_cond);
#endif
	if (setitimer(ITIMER_REAL, &tick, NULL) < 0) {
		printf("Set timer failed!\n");	
	}
	pthread_mutex_unlock(&timer->timer_mutex);
#else
	kal_take_mutex(timer->networkMutex);
	timer->timerProcess = 0;
	StopTimer(NATTY_NETWORK_COMFIRMED_TIMER);
	kal_give_mutex(timer->networkMutex);
#endif
	return 0;
}




static const TimerOpera ntyTimerOpera = {
	sizeof(NetworkTimer),
	ntyTimerCtor,
	ntyTimerDtor,
	ntyStartTimerOpera,
	ntyStopTimerOpera,
};

const void *pNtyTimerOpera = &ntyTimerOpera;

static void* pNetworkTimer = NULL;
void *ntyNetworkTimerInstance(void) {
	if (pNetworkTimer == NULL) {
		pNetworkTimer = New(pNtyTimerOpera);
	}
	return pNetworkTimer;
}


int ntyStartTimer(void *self,  HANDLE_TIMER func) {
	const TimerOpera* const *handle = self;
	if (self && (*handle) && (*handle)->start) {
		return (*handle)->start(self, func);
	}
	return -2;
}

int ntyStopTimer(void *self) {
	const TimerOpera* const *handle = self;
	if (self && (*handle) && (*handle)->stop) {
		return (*handle)->stop(self);
	}
	return -2;
}

void ntyNetworkTimerRelease(void *self) {
	
	Delete(self);
}


#if 1 //for 6261 mmi

#define MSG_POST_COND_WAIT		0x01
#define MSG_POST_COND_POLLING	0x02


void NattyConditionSendMsgToMMIMod(U8 command,void *param) {
#if 0
	ilm_struct *ilm_ptr = NULL;//__GEMINI__
	U8 *p = (U8*)construct_local_para(sizeof(void*)+1,TD_CTRL);
	p[0] = command;
	#if 0
	*(void*(&p[1])) = param;
	#else
	memcpy(&p[1], param, sizeof(void*));
	#endif
	ilm_ptr = allocate_ilm(MOD_MMI);//MOD_L4C
	ilm_ptr->msg_id = MSG_ID_MMI_JAVA_UI_TEXTFIELD_SHOW_REQ;
	ilm_ptr->local_para_ptr = (local_para_struct*)p;
	ilm_ptr->peer_buff_ptr = NULL;

	ilm_ptr->src_mod_id  = MOD_MMI;//MOD_L4C;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->sap_id = MOD_MMI;//MMI_L4C_SAP;
	msg_send_ext_queue(ilm_ptr);
#endif
}

static void NattyConditionPolling(void) {
	
}

static void NattyConditionProcMsgHdler(void *msg) {
	U8    *command = (PU8)msg;
	NetworkTimer *timer = (NetworkTimer *)(&command[1]);

	switch (command[0]) {
		case MSG_POST_COND_WAIT: {
			
			break;
		}
		case MSG_POST_COND_POLLING: {
			StartTimer(NATTY_CONDITION_TIMER, 2, NattyConditionPolling);
			break;
		}
	}
	
}


void NattyConditionRegisterMsg(void)
{
#if 0
	SetProtocolEventHandler(NattyConditionProcMsgHdler, MSG_ID_MMI_JAVA_UI_TEXTFIELD_SHOW_REQ);
#endif
}
 

#endif


