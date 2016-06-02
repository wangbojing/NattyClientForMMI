#ifdef __USE_YX_APP_SERVICE__
#include "MMI_features.h"
#include "MMIDataType.h"
#include "kal_release.h"
#include "Gdi_layer.h"
#include "NVRAMEnum.h"
#include "NVRAMProt.h"
#include "NVRAMType.h"
#include "custom_nvram_editor_data_item.h"
#include "l1sm_public.h"
#if defined(IK158_BB)||defined(IK188_BB)
#include "gui_typedef.h"
#include "wgui_categories_util.h"
#endif
#include "YxBasicApp.h"

#ifdef __USE_NATTY_PROTOCOL__
#include "Natty/NattyClientDev.h"
#endif

/////////////////////////////////////////////////////////Global var/////////////////////////////////////////////////////////////

static U16  yxAppNeedRunFlag = 0;
static U8   yxAppExit = 0;
static YXAPPPARAM yxAppParam;

/////////////////////////////////////////////////////////Global api/////////////////////////////////////////////////////////////

#if defined(IK158_BB)||defined(IK188_BB)
static void YxAppSetAppIconDisplay(void)
{
	static U8  yxiconisset = 0;
	if(yxiconisset==0)
	{
		wgui_status_icon_bar_set_icon_display(STATUS_ICON_CIRCLE);
		wgui_status_icon_bar_update();
		yxiconisset = 1;
	}
}
#endif

U16 YxAppGetRunFlag(char kind)
{
	if(kind==1)
		return yxAppParam.uploadStart;
	return yxAppNeedRunFlag;
}

void YxAppDisableSleep(void)
{
	if(yxAppParam.sleepdisable==0)
	{
		if(yxAppParam.sleephandle!=0 && yxAppParam.sleephandle!=0xFF)
			L1SM_SleepDisable(yxAppParam.sleephandle);
		yxAppParam.sleepdisable = 1;
	}
}

void YxAppEnableSleep(void)
{
	if(yxAppParam.sleepdisable==1)
	{
		if(yxAppParam.sleephandle!=0 && yxAppParam.sleephandle!=0xFF)
			L1SM_SleepEnable(yxAppParam.sleephandle);
		yxAppParam.sleepdisable = 0;
	}
}

static void YxAppGprsUploadPs(void)
{
	S8 res = 0;
	yxAppParam.uploadStart |= YX_UPLOAD_KIND_GPS;
	res = YxAppConnectToMyServer();
#if(YX_IS_TEST_VERSION!=0)
	kal_prompt_trace(MOD_BT,"upload start\r\n");
#endif
	if(res==0)//传输失败
		YxAppUploadDataFinishCb();
	else if(res==3)//busing
	{
		yxAppParam.uploadStart &= ~YX_UPLOAD_KIND_GPS;
		yxAppParam.uploadStart |= YX_UPLOAD_NEED_GPS;
#if(YX_IS_TEST_VERSION!=0)
		kal_prompt_trace(MOD_BT,"upload busing\r\n");
#endif
		//YxAppSaveUnsendTxtcmdPackage();
	}
}

void YxAppUploadDataFinishCb(void)
{
	char  kind = 0;
	if(yxAppParam.uploadStart & YX_UPLOAD_KIND_GPS)
	{
		yxAppParam.uploadStart &= ~YX_UPLOAD_KIND_GPS;
		kind = 1;
	}
	else
		yxAppParam.uploadStart &= ~YX_UPLOAD_KIND_HEART;
//	YxAppEnableSleep();
	if((kind==1) && ((yxAppNeedRunFlag & YX_RUNKIND_OWNER_GPS)==0))
		YxAppSaveUnsendTxtcmdPackage();
	if((yxAppParam.uploadStart & YX_UPLOAD_NEED_PS)&&((yxAppNeedRunFlag & YX_RUNKIND_OWNER_GPS)==0))
	{
		yxAppParam.uploadStart = 0;
		yxAppParam.gpsTick = 200;
		YxAppUploadGpsProc();
	}
	else if(yxAppParam.uploadStart & YX_UPLOAD_NEED_GPS)
	{
		yxAppParam.uploadStart = 0;
		YxAppGprsUploadPs();
	}
	else if(yxAppParam.uploadStart & YX_UPLOAD_NEED_HEART)
	{
		yxAppParam.uploadStart = 0;
		yxAppParam.heartTick = 120;
		YxAppUploadHeartProc();
	}
	else
		yxAppParam.uploadStart = 0;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"upload end\r\n",12);
#endif
}

void YxAppStepRunMain(U16 runKind)
{
//	U16   oldFlag = yxAppNeedRunFlag&0x3FFF;
	if(yxAppParam.allIsReady!=3)
	{
		if(runKind==YX_RUNKIND_WIFI)
		{
			yxAppParam.allIsReady = 3;
#if defined(IK158_BB)||defined(IK188_BB)
			YxAppSetAppIconDisplay();
#endif
		}
		return;
	}
	yxAppNeedRunFlag &= (~runKind);
	if((yxAppNeedRunFlag&0x3FFF)==0)
	{
		if(yxAppNeedRunFlag & YX_RUNKIND_OWNER_GPS)//connect to sever
			yxAppNeedRunFlag &= ~YX_RUNKIND_OWNER_GPS;
	}
	if(runKind && ((yxAppNeedRunFlag & YX_RUNKIND_LBS)==0) 
#if defined(YXAPP_WIFI_UART)||defined(__MMI_WLAN_FEATURES__)
		&& ((yxAppNeedRunFlag & YX_RUNKIND_WIFI)==0)
#endif
#ifdef __MMI_BT_SUPPORT__
		&& ((yxAppNeedRunFlag & YX_RUNKIND_BT_SEARCH)==0)
#endif
		)
	{
		if(runKind==YX_RUNKIND_GPS && (GpsGetLongitudeV()==0.0 && GpsGetLatitudeV()==0.0))
			return;
		YxAppGprsUploadPs();
	}
#ifdef YXAPP_WIFI_UART
	if((runKind==YX_RUNKIND_WIFI)&&(yxAppNeedRunFlag & YX_RUNKIND_GPS))
		YxAppGpsControl(1);
#endif
#if defined(__MMI_WLAN_FEATURES__)
#ifdef __MMI_BT_SUPPORT__
	if((runKind==YX_RUNKIND_WIFI)&&(yxAppNeedRunFlag & YX_RUNKIND_BT_SEARCH))
		YxAppBtSearchDevices();
#endif
#endif
}

void YxAppIdleInition(void)
{
	if(yxAppParam.allIsReady>=1)
		return;
	YxAppMMiRegisterMsg();
#if(YXAPP_UART_TASK_OWNER_WAY==1)||defined(YXAPP_WIFI_UART)
	YxAppRegisterUartMsgToMMiTask();
#endif
	YxAppStartSecondTimer(1);
	yxAppExit = 0;
	yxAppParam.sleephandle = L1SM_GetHandle();
	yxAppParam.allIsReady = 1;
}

void YxAppIdleRunFunctions(void)
{
	if(yxAppParam.idleRun==1)
		return;
#if (YX_PROTOCOL_KIND==1)
	YxProtocolInition();
#endif
	YxAppStartSecondTimer(1);
#if(YX_GSENSOR_SURPPORT!=0)
	YxAppGsensorProcInit();
#endif
#if(YX_USE_PROXIMITY_IC==1)
	YxPsAlsInition();
#endif
	YxAppGetImeiNumber();//add by liu gong
#if 0//(YX_IS_TEST_VERSION==2)
	{
		YXSYSTEMPARAM    *setParam = YxAppGetSystemParam(0);
		setParam->onlyTest++;
		YxAppSaveSystemSettings();
	}
#endif

#ifdef __USE_NATTY_PROTOCOL__
	ntyClientDevInit();
#endif
	yxAppParam.idleRun = 1;
}

void YxAppInitionParam(void)
{
	memset((void*)&yxAppParam,0,sizeof(YXAPPPARAM));
	yxAppNeedRunFlag = 0;
}

void YxAppUploadDataByTickProc(U8 hour,U8 min)
{
	if(yxAppExit==1 || yxAppParam.allIsReady==0)
		return;
#if (YX_GSENSOR_SURPPORT!=0)
	YxAppMinTickCheckSteps();
	YxAppGsensorCheckCurrentTime(hour,min);
#endif
#if(YX_USE_PROXIMITY_IC==1)
	YxAppPsPollingByMinuteTick();
#endif
	YxAppUploadGpsProc();
//	YxAppSendMsgToMMIMod(YX_MSG_MAINAPP_TICK_CTRL,0,0);
}

void YxAppSecondTickProc(void)
{
	if(yxAppExit==1 || yxAppParam.allIsReady<3)
		return;
	YxAppSendMsgToMMIMod(YX_MSG_MAINAPP_SECOND_CTRL,0,0);
}

void YxAppStartPosAtOnceProc(char kind)//kind:1:is sos call,0:other call
{
	if(yxAppParam.allIsReady<3)
		return;
	if(kind==1)
		YxProtocolSetSosStatus();
	yxAppParam.gpsTick = 200;
	YxAppSendMsgToMMIMod(YX_MSG_START_POS_CTRL,0,0);
}

void YxAppSetOntimeCallGpsRunFlag(void)
{
	yxAppParam.uploadStart |= YX_UPLOAD_NEED_PS;
#if(YX_IS_TEST_VERSION!=0)
	kal_prompt_trace(MOD_BT,"run ps:%x\r\n",yxAppParam.uploadStart);
#endif
}

U16 YxAppGetSystemTick(char second)
{
	YXSYSTEMPARAM     *yxSetting = YxAppGetSystemParam(0);
	if(second)
		return yxSetting->systemTick;
	return yxSetting->minuteTick;
}

void YxAppEndAllAction(void)
{
	yxAppExit = 1;
#ifdef YXAPP_WIFI_UART
	YxAppWifiClose();
#endif
#if(YX_GSENSOR_SURPPORT!=0)
	YxAppGsensorPedometerStart(0);
#endif
	YxAppStartSecondTimer(0);
	YxAppCloseNetwork();
}

void YxAppUploadGpsProc(void)
{
	YXSYSTEMPARAM     *yxSetting = YxAppGetSystemParam(0);
	if(yxAppParam.allIsReady==1)//开机前1分钟,获取本机的BTMAC,name,wlan mac
	{
#ifdef __MMI_BT_SUPPORT__
		YxAppGetLocalBtNameMac(NULL,NULL);
#endif
#ifdef __MMI_WLAN_FEATURES__
		YxAppWlanPowerScan(1);
#else
		yxAppParam.allIsReady = 3;
#if defined(IK158_BB)||defined(IK188_BB)
		YxAppSetAppIconDisplay();
#endif
#endif
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData((U8*)"getmac\r\n",9);
#endif
		return;
	}
	if(yxAppParam.allIsReady<3)
		return;
	if(yxAppExit==1)
		return;
	if(yxSetting->minuteTick + 1 >= 0xFF00)
		yxSetting->minuteTick = 0;
	else
		yxSetting->minuteTick++;
	YxGpsCheckFiveMinuteTimeOut();
	yxAppParam.gpsTick++;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"mintick\r\n",9);
#endif
#if(YX_IS_TEST_VERSION!=0)
	if(yxAppParam.gpsTick >= 2)
#else
	if(yxAppParam.gpsTick >= yxSetting->gpsTimes)
#endif
	{
		yxAppParam.gpsTick = 0;
		if((yxAppNeedRunFlag & YX_RUNKIND_OWNER_GPS)||(yxAppParam.uploadStart & YX_UPLOAD_KIND_GPS))//正在运行
		{
#if(YX_IS_TEST_VERSION!=0)
			if(yxAppNeedRunFlag & YX_RUNKIND_OWNER_GPS)
				kal_prompt_trace(MOD_BT,"get ps busing:1:%x\r\n",yxAppNeedRunFlag);
			else
				kal_prompt_trace(MOD_BT,"get ps busing:2:%x\r\n",yxAppNeedRunFlag);
#endif
			return;
		}
		yxAppNeedRunFlag = YX_RUNKIND_OWNER_GPS|YX_RUNKIND_LBS|YX_RUNKIND_GPS;
#ifndef WIN32
#if defined(__MMI_WLAN_FEATURES__)||defined(YXAPP_WIFI_UART)
		yxAppNeedRunFlag |= YX_RUNKIND_WIFI;
#endif
#ifdef __MMI_BT_SUPPORT__
		if(yxSetting->searchBt == 1)
			yxAppNeedRunFlag |= YX_RUNKIND_BT_SEARCH;
#endif
#endif
		YxAppStepRunMain(0);
		YxAppAllPsDevicesStartContrl();
		return;
	}
}

void YxAppUploadHeartProc(void)
{
	YXSYSTEMPARAM     *yxSetting = YxAppGetSystemParam(0);
	if(yxAppParam.allIsReady<3)
		return;
	if(yxAppExit==1)
		return;
	if(yxSetting->systemTick + 1 >= 0xFF00)
		yxSetting->systemTick = 0;
	else
		yxSetting->systemTick++;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"10second\r\n",10);
#endif
	yxAppParam.heartTick++;
	if(yxAppParam.heartTick >= yxSetting->uploadTick)
	{
		S8 res = 0;
		res = YxAppConnectToMyServer();
		yxAppParam.heartTick = 0;
		yxAppParam.uploadStart |= YX_UPLOAD_KIND_HEART;
		if(res==0)//传输失败
			YxAppUploadDataFinishCb();
		else if(res==3)//busing
		{
			yxAppParam.uploadStart &= ~YX_UPLOAD_KIND_HEART;
			yxAppParam.uploadStart |= YX_UPLOAD_NEED_HEART;
		}
		return;
	}
	return;
}
#endif