#ifdef __USE_YX_APP_SERVICE__
#include "MMI_features.h"
#include "MMIDataType.h"
#include "kal_release.h"
#include "kal_public_api.h"
#include "Gdi_layer.h"
#include "NVRAMEnum.h"
#include "NVRAMProt.h"
#include "NVRAMType.h"
#include "custom_nvram_editor_data_item.h"
#include "l1sm_public.h"
#include "SmsGuiInterfaceProt.h"
#include "SMsGuiInterfaceType.h"
#include "SmsPsHandler.h"
#include "SSCStringHandle.h"
#include "wgui_categories.h"
#include "wgui_include.h"
#include "wgui_categories_util.h"
#ifdef __MMI_WLAN_FEATURES__
#include "wndrv_cnst.h"
#endif
//#include "DtcntSrvIntDef.h"
//#include "DtcntSrvGprot.h"
#include "DtcntSrvIprot.h"
#include "modeswitchsrvgprot.h"
#include "DataAccountStruct.h"
#ifdef __MMI_WLAN_FEATURES__
#include "gui_typedef.h"
#include "CommonScreensResDef.h"
#include "CommonScreens.h"
#include "DataAccountCore.h"
#include "DataAccountProt.h"
#endif
#include "gui.h"
#include "app2soc_struct.h"
#include "gui_themes.h"
#include "soc_api.h"
#include "PhoneBookGprot.h"
#include "SimCtrlSrvGprot.h"
#include "NwUsabSrvGprot.h"
#include "NwInfoSrvGprot.h"
#include "cbm_consts.h"
#include "TimerEvents.h"
#include "IdleAppDef.h"
#include "CphsSrvGprot.h"
#include "gpiosrvgprot.h"
#ifndef WIN32
#include "uart_internal.h"
#endif
#ifdef __MMI_BT_SUPPORT__
#include "BTMMIScrGprots.h"
#include "BtcmSrvProt.h"
#include "BthScoPathSrvGProt.h"
#include "av_bt.h"
#include "BTMMIA2DPScr.h"
#endif
#ifdef __NBR_CELL_INFO__
#include "CommonScreens.h"
#include "conversions.h"
#include "as2l4c_struct.h"
#endif
#include "DtcntSrvIntStruct.h"
#include "DtcntSrvDb.h"
#include "CharBatSrvGprot.h"
#include "UcmGProt.h"
#include "UcmProt.h"
#include "soc_api.h"
#include "soc_consts.h"
#include "cbm_api.h"
#include "fs_gprot.h"
#include "med_utility.h"
#include "mmi_rp_app_mre_def.h"
#include "ps_public_enum.h"
#include "app_datetime.h"
#include "EventsGprot.h"
#include "dcl_uart.h"
#include "med_utility.h"
#include "DateTimeGprot.h"
#include "ProfilesSrv.h"
#include "gpiosrvgprot.h"
#include "GeneralSettingSrvGprot.h"
#include "mmi_rp_app_phonebook_def.h"
#include "PhoneBookProt.h"
#include "NITZSrvGprot.h"
#include "YxBasicApp.h"

/////////////////////////////////////////////////////////user param config area///////////////////////////////////////////////

#ifdef __MMI_BT_SUPPORT__
#define  YXAPP_BT_MAX_SEARCH_TIME     8 //15s,蓝牙周边地址搜索时间
#endif

#ifdef __NBR_CELL_INFO__
#define  YXAPP_LBS_TIMER_OUT          10000 //5s,基站数据采集时间
#endif

#if(YX_SECOND_TICK_WAY==1)
#define  YXAPP_SECOND_TIMER_ID        145
#endif

#if(YXAPP_UART_TASK_OWNER_WAY==1)
#define  YXAPP_UART_TASK_OWNER        MOD_MMI
#else
#define  YXAPP_UART_TASK_OWNER        MOD_YXAPP
#endif

#if(YXAPP_WIFI_UART_MOD_WAY==0)
#define  YXAPP_WIFI_UART_MOD          MOD_MMI
#else
#define  YXAPP_WIFI_UART_MOD          MOD_YXAPP
#endif
/////////////////////////////////////////////////////////Global variants/////////////////////////////////////////////////////////////

static  char yxDataPool[SRV_BT_CM_BD_FNAME_LEN+1];
static  YXAPPSOCKDATAPARAM yxAppSockData;
#ifdef __MMI_WLAN_FEATURES__
static WLANMACPARAM wlanParams[WNDRV_MAX_SCAN_RESULTS_NUM];
static U8 yxLocalWlanMac[WNDRV_MAC_ADDRESS_LEN];
#endif
#ifdef YXAPP_WIFI_UART
static YXWIFIPARAM wifiParam;
#endif
#ifdef __MMI_BT_SUPPORT__
static S32 yxAppBtSrvhd = 0;
static char yxBtInSearching = 0;
static YXBTPARAM yxbtInfor[YXAPP_BT_MAX_SEARCH_DEVICES];
static U8 yxLocalBtName[SRV_BT_CM_BD_FNAME_LEN+1],yxLocalBtMac[YX_BT_MAC_BUFFER_LEN];
#endif
#if(YX_SECOND_TICK_WAY==1)
static stack_timer_struct YxappTaskStackTimer;
#endif
#ifdef __NBR_CELL_INFO__
static YXAPPCELLPARAM yxCellParam;
#endif
#if(YX_USE_PROXIMITY_IC==1)
static char yxProximityStatus = 0;
#endif
static YXAPPSMSPARAM yxSmsParam;
static YXAPPSOCKCONTEXT yxNetContext_ptr;
static YXSYSTEMPARAM  yxSystemSet;
#if(YX_GSENSOR_SURPPORT!=0)
static YXGSENSORPARAM yxGsensorParam;
static kal_timerid YxGsensortimerId =  0;
static char yxGsensorTimerFlag = 0;
#endif
static YXCALLPARAM yxCallParam;
static YXEFENCEPARAM yxEfencePkg[YX_EFENCE_MAX_NUM];
static YXFIREWALLPARAM yxFirewall;
static YXLOGPARAM yxLogsPkg[YX_LOGS_MAX_NUM];
static U16 yxPlayToneTick = 0;
static U8 yxappCmd[3];
static char yxTimerCountImei = 0;
/////////////////////////////////////////////////////////static local functions////////////////////////////////////////////////////////

#ifdef __MMI_BT_SUPPORT__
static char YxAppBtSearchDev(void);
#endif
#ifdef __MMI_WLAN_FEATURES__
static void YxAppWifiIntenalClose(void);
static char YxAppWifiOpenAndScan(void);
#endif
#ifdef YXAPP_WIFI_UART
static void YxAppWifiUartDealWithMessage(void);
#endif

#ifdef __NBR_CELL_INFO__
static void YxAppCellStartReq(void);
static void YxAppCellCancelReq(void);
#endif

static char YxAppCloseAccount(void);
static void YxAppSendSmsIntenal(S8* number,char gsmCode,U16* content);
static void YxAppConnectToServerTimeOut(void);

#define YXISINCALL(a)   srv_ucm_query_call_count(a, SRV_UCM_VOICE_CALL_TYPE_ALL, NULL)
#define GetDateTime(t)  applib_dt_get_date_time((applib_time_struct *)t)

//extern void mmi_dtcnt_set_account_default_app_type(U32 acct_id, MMI_BOOL with_popup_result);

/////////////////////////////////////////////////////////basic api//////////////////////////////////////////////////////////

void YxAppSendMsgToMMIMod(U8 command,U8 param1,U8 param2)
{
#if 1
	yxappCmd[0] = command;
	yxappCmd[1] = param1;
	yxappCmd[2] = param2;
	mmi_frm_send_ilm(MOD_MMI, MSG_ID_MMI_JAVA_UI_TEXTFIELD_HIDE_REQ, NULL, NULL);
#else
	ilm_struct *ilm_ptr = NULL;//__GEMINI__
	U8 *p = (U8*)construct_local_para(3,TD_CTRL);
	p[0] = command;
	p[1] = param1;
	p[2] = param2;
	ilm_ptr = allocate_ilm(MOD_MMI);//MOD_L4C
	ilm_ptr->msg_id = MSG_ID_MMI_JAVA_UI_TEXTFIELD_HIDE_REQ;
	ilm_ptr->local_para_ptr = (local_para_struct*)p;
	ilm_ptr->peer_buff_ptr = NULL;

	ilm_ptr->src_mod_id  = MOD_MMI;//MOD_L4C;
	ilm_ptr->dest_mod_id = MOD_MMI;
	ilm_ptr->sap_id = MOD_MMI;//MMI_L4C_SAP;
	msg_send_ext_queue(ilm_ptr);
#endif
}

#if(YX_USE_PROXIMITY_IC==1)
void YxAppProximitySendMsgToMMIMod(void)
{
	ilm_struct *ilm_ptr;   
    ilm_ptr = allocate_ilm(MOD_DRV_HISR);
    ilm_ptr->msg_id = MSG_ID_MMI_JAVA_UI_TEXTFIELD_HIDE_RSP;
    ilm_ptr->local_para_ptr = NULL;
    ilm_ptr->peer_buff_ptr = NULL;
    ilm_ptr->src_mod_id = MOD_DRV_HISR;
    ilm_ptr->dest_mod_id = MOD_MMI;
    ilm_ptr->sap_id = MOD_MMI;		
    msg_send_ext_queue( ilm_ptr);
}
#endif

#if(YX_GSENSOR_SURPPORT==1)//eint
static void YxMMIProcGsensorMsgHdler(void *msg)
{
	if(yxGsensorParam.isReady==1)
	{
		kal_int16   x_adc = 0,y_adc = 0,z_adc = 0;
		if(YxAppGsensorSampleData(&x_adc,&y_adc,&z_adc)==1)
		{
#if(YX_IS_TEST_VERSION!=0)
			char logBuf[51];
#endif
#if(YX_IS_TEST_VERSION!=0)
			sprintf(logBuf,"x_adc:%d,y_adc:%d,z_adc:%d\r\n",x_adc,y_adc,z_adc);
			YxAppTestUartSendData((U8*)logBuf,strlen(logBuf));
#endif
		}
	}
	return;
}
#endif

#if 0//(YX_USE_PROXIMITY_IC==1)
static void YxMMIProcTimerMsgHdler(void *msg)
{
	kal_uint8  val = YxPsReadProximityValue();
#if(YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT, "ldz ps eint val:%d\r\n",val);
#endif
}
#endif

static void YxMMIProcMsgHdler(void *msg)
{
	U8    *command = yxappCmd;//(PU8)msg;
	switch(command[0])
	{
	case YX_MSG_START_POS_CTRL:
	case YX_MSG_MAINAPP_TICK_CTRL:
		YxAppUploadGpsProc();
		return;
	case YX_MSG_GSENSOR_CTRL:
#if(YX_GSENSOR_SURPPORT!=0)
		if(command[1]==1)
			YxAppGsensorPedometerStart(1);
		else
			YxAppGsensorPedometerStart(0);
#endif
		return;
	case YX_MSG_ALL_PS_DEV_CTRL:
		YxAppStartAllPsDevices();
		return;
#ifdef __MMI_BT_SUPPORT__
	case YX_MSG_BLUETOOTH_CTRL:
		switch(command[1])
		{
		case YX_DATA_BTSETNAME:
			if(srv_bt_cm_get_power_status()!=SRV_BT_CM_POWER_ON)
				break;
			if(srv_bt_cm_set_host_dev_name((U8*)yxDataPool) != SRV_BT_CM_RESULT_SUCCESS)
				break;
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData((U8*)yxDataPool,strlen(yxDataPool));
#endif
			break;
		case YX_DATA_BTSEARCH:
			YxAppBtSearchDev();
			break;
		case YX_DATA_BT_ABORT_SEARCH:
			if(srv_bt_cm_search_abort() == SRV_BT_CM_RESULT_SUCCESS)
				yxBtInSearching = 0;
			break;
		case YX_DATA_BT_POWER_CTRL:
			{
				if(command[2]>0)//on
				{
					if(srv_bt_cm_get_power_status()==SRV_BT_CM_POWER_OFF)
						srv_bt_cm_switch_on();
				}
				else
				{
					if(srv_bt_cm_get_power_status()==SRV_BT_CM_POWER_ON)
					{
#if defined(__MMI_A2DP_SUPPORT__)
						mmi_a2dp_bt_power_off_callback(MMI_TRUE);
#endif
						srv_bt_cm_switch_off();
					}
				}
			}
			break;
		case YX_DATA_BT_SET_VISIBILITY:
			if(srv_bt_cm_get_power_status()==SRV_BT_CM_POWER_ON)
			{
				srv_bt_cm_visibility_type setToType = (command[2]>0) ? SRV_BT_CM_VISIBILITY_ON : SRV_BT_CM_VISIBILITY_OFF;
				srv_bt_cm_set_visibility(setToType);
#if(YX_IS_TEST_VERSION!=0)
				YxAppTestUartSendData("BT-VIS\r\n",8);
#endif
			}
			break;
		default:
			break;
		}
		return;
#endif
#ifdef __MMI_WLAN_FEATURES__
	case YX_MSG_WLAN_POWER_CTRL:
		if(command[1]==0)
		{
			YxAppWifiIntenalClose();
			YxAppStepRunMain(YX_RUNKIND_WIFI);
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("WIFICLOSE-OK\r\n",13);
#endif
		}
		else
		{
			if(YxAppWifiOpenAndScan()==0)
				YxAppStepRunMain(YX_RUNKIND_WIFI);
#if(YX_IS_TEST_VERSION!=0)
			else
				YxAppTestUartSendData("WIFIOPEN-OK\r\n",13);
#endif
		}		
		return;
#endif
#ifdef __NBR_CELL_INFO__
	case YX_MSG_LBS_CELL_CTRL:
		if(command[1]==YX_DATA_LBS_CANECL)
			YxAppCellCancelReq();
		else
			YxAppCellStartReq();
		return;
#endif
	case YX_MSG_CALL_CTRL:
		if(command[1]==YX_DATA_MAKE_CALL)
		{
			U16   phnum[YX_APP_CALL_NUMBER_MAX_LENGTH+1],i = 0;
			mmi_ucm_make_call_para_struct  make_call_para;
			mmi_ucm_init_call_para(&make_call_para);
			while(yxDataPool[i])
			{
				phnum[i] = yxDataPool[i];
				i++;
				if(i>=YX_APP_CALL_NUMBER_MAX_LENGTH)
					break;
			}
			phnum[i] = 0;
			make_call_para.ucs2_num_uri = (U16*)phnum;
			mmi_ucm_call_launch(0, &make_call_para);
		}
		return;
	case YX_MSG_SMS_CTRL:
		if(command[1]==YX_DATA_SEND_SMS)
		{
			if(command[2]==0)
				YxAppSendSmsIntenal(yxSmsParam.number,0,yxSmsParam.content);
			else
				YxAppSendSmsIntenal(yxSmsParam.number,1,yxSmsParam.content);
		}
		return;
	}
}

#if !defined(__MMI_NITZ__)
#define NITZ_SIGN_BIT	0x08
#define NITZ_TZ_BIT		0x07
#define NITZ_HIGH_BIT	0xF0
#define NITZ_LOW_BIT	0x0F
static S16 srv_nitz_convert_timezone(U8 t)
{
    U8 tmp;
    U32 NegFlag;
    S32 result;
    U8 timezone = t;
    tmp = (t & NITZ_HIGH_BIT) >> 4;
    if (t & NITZ_SIGN_BIT)  /* negative */
    {
        NegFlag = TRUE;
    }
    else    /* positive */
    {
        NegFlag = FALSE;
    }
    timezone &= NITZ_TZ_BIT;
    timezone = timezone * 10 + tmp;
    result = (S16) timezone;
    if (NegFlag)
        result = 0 - result;	
    return result;
}

static U8 srv_nitz_convert_time(U8 time)
{
    U8 tmp1, tmp2;
    tmp1 = time & NITZ_HIGH_BIT;
    tmp1 = tmp1 >> 4;
    tmp2 = time & NITZ_LOW_BIT;
    return tmp2 * 10 + tmp1;
}

static void srv_nitz_get_utc_time(void *msg, MYTIME *out_time)
{
    mmi_nw_time_zone_ind_struct *msg_info = (mmi_nw_time_zone_ind_struct*)msg;
	out_time->nYear = srv_nitz_convert_time(msg_info->nw_time_zone_time.year) + YEARFORMATE;
    out_time->nMonth = srv_nitz_convert_time(msg_info->nw_time_zone_time.month);
    out_time->nDay = srv_nitz_convert_time(msg_info->nw_time_zone_time.day);
    out_time->nHour = srv_nitz_convert_time(msg_info->nw_time_zone_time.hour);
    out_time->nMin = srv_nitz_convert_time(msg_info->nw_time_zone_time.min);
    out_time->nSec = srv_nitz_convert_time(msg_info->nw_time_zone_time.sec);
}

static void yxapp_nitz_dt_update_handler(void *msg)
{
	extern kal_uint8 applib_dt_sec_2_mytime(kal_uint32 utc_sec, applib_time_struct *result);
	S16       curTimeZone = 0;
	MYTIME    nitz_time,*nitz_timeP = NULL;
	mmi_nw_time_zone_ind_struct *msg_info = (mmi_nw_time_zone_ind_struct*)msg;
	U32       tick_count_2 = 0, diff_second = 0,tickCount = 0;
	if(msg_info->nw_time_zone_timeP)
    {
		FLOAT     nitz_timezone;
		MYTIME    ref_time;
	    srv_nitz_get_utc_time(msg, &ref_time);
        curTimeZone = srv_nitz_convert_timezone(msg_info->nw_time_zone_time.time_zone);
        nitz_timezone = (FLOAT)curTimeZone;
        nitz_timezone /= 4;
		/* increase NITZ date time with time zone and save to global context */
        mmi_dt_utc_to_rtc(nitz_timezone, ref_time, nitz_time);
	}
	kal_get_time(&tickCount);
	nitz_timeP = &nitz_time;
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"NITZ time zone:%d\r\n",curTimeZone);
#endif
	if(mmi_dt_is_valid(nitz_timeP))
	{
		applib_time_struct incTime;
        kal_get_time(&tick_count_2);        
        if(tick_count_2 >= tickCount)
            diff_second = kal_ticks_to_secs(tick_count_2 - tickCount);
        else
            diff_second = kal_ticks_to_secs(0xFFFF - (tickCount - tick_count_2));        
        applib_dt_sec_2_mytime(diff_second, (applib_time_struct*)&incTime);
        IncrementTime(nitz_time, incTime, nitz_timeP);
        mmi_dt_set_rtc_dt((MYTIME*)nitz_timeP);
		YxAppGpsSetServerTimeStatus();
#if (YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"NITZ update date:%d-%d-%d\r\n",nitz_timeP->nYear,nitz_timeP->nMonth,nitz_timeP->nDay);
#endif
    }
}

static void srv_nitz_msg_ind(void *msg, int mod_src)
{
#if 0
	mmi_sim_enum sim_id;
	sim_id = mmi_frm_l4c_mod_to_sim((module_type)mod_src);
#endif
	yxapp_nitz_dt_update_handler(msg);
}

void srv_nitz_init(void)
{
	SetProtocolEventHandler(srv_nitz_msg_ind, MSG_ID_MMI_NW_TIME_ZONE_IND);
}
#endif

void YxAppMMiRegisterMsg(void)
{
	SetProtocolEventHandler(YxMMIProcMsgHdler,MSG_ID_MMI_JAVA_UI_TEXTFIELD_HIDE_REQ);
#if(YX_USE_PROXIMITY_IC==1)
	//SetProtocolEventHandler(YxMMIProcTimerMsgHdler,MSG_ID_MMI_JAVA_UI_TEXTFIELD_HIDE_RSP);
#endif
}

#if(YX_USE_PROXIMITY_IC==1)
void YxAppPsPollingByMinuteTick(void)
{
	if(YxPsCheckStatus()==0)
	{
		if(yxProximityStatus==0)
		{
			YxAppLogAdd(LOG_OFFHANDS,'0',0);
#if (YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT, "ik take off\r\n");
#endif
			yxProximityStatus = 1;
			return;
		}
	}
	else
	{
		if(yxProximityStatus)
		{
#if (YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT, "ik take on\r\n");
#endif
			yxProximityStatus = 0;
			return;
		}
	}
}
#endif
/////////////////////////////////////////////////////////Uart api/////////////////////////////////////////////////////////////

static DCL_STATUS YXOS_UART_function(DCL_DEV port, DCL_CTRL_CMD cmd, DCL_CTRL_DATA_T *data)
{
	DCL_STATUS status;
	DCL_HANDLE handle;
	
	handle = DclSerialPort_Open(port,0);
   	status = DclSerialPort_Control(handle, cmd, data);
	DclSerialPort_Close(handle);
	return status;
}

static kal_bool yxos_UART_Open(DCL_DEV port, DCL_UINT32 ownerid)
{
    UART_CTRL_OPEN_T data;
	DCL_STATUS status;
	data.u4OwenrId = ownerid;
	status = YXOS_UART_function(port, SIO_CMD_OPEN, (DCL_CTRL_DATA_T*)&data);
	if(STATUS_OK != status)
		return KAL_FALSE;
	else
		return KAL_TRUE;
}

void SerialPort_UART_TurnOnPower(UART_PORT port, kal_bool enable) 
{
	UART_CTRL_POWERON_T data;
	data.bFlag_Poweron = enable;
	YXOS_UART_function(port, UART_CMD_POWER_ON, (DCL_CTRL_DATA_T*)&data);
}

static kal_bool SerialPort_UART_Open(UART_PORT port, module_type ownerid)
{
	return yxos_UART_Open(port,ownerid);
}

static  DCL_STATUS SerialPort_UART_SetDCBConfig(UART_PORT port, UARTDCBStruct *UART_Config, module_type ownerid)
{
	UART_CTRL_DCB_T data;
	data.u4OwenrId = ownerid;
	data.rUARTConfig.u4Baud = UART_Config->baud;
	data.rUARTConfig.u1DataBits = UART_Config->dataBits;
	data.rUARTConfig.u1StopBits = UART_Config->stopBits;
	data.rUARTConfig.u1Parity = UART_Config->parity;
	data.rUARTConfig.u1FlowControl = UART_Config->flowControl;
	data.rUARTConfig.ucXonChar = UART_Config->xonChar;
	data.rUARTConfig.ucXoffChar = UART_Config->xoffChar;
	data.rUARTConfig.fgDSRCheck = (DCL_BOOLEAN)(UART_Config->DSRCheck);
	YXOS_UART_function(port, SIO_CMD_SET_DCB_CONFIG, (DCL_CTRL_DATA_T*)&data);
	return STATUS_OK;
}

static void SerialPort_UART_SetOwner(UART_PORT port, module_type ownerid)
{
	UART_CTRL_OWNER_T data;
	data.u4OwenrId = ownerid;
	YXOS_UART_function(port, SIO_CMD_SET_OWNER, (DCL_CTRL_DATA_T*)&data);
}

static void SerialPort_UART_Close(UART_PORT port, module_type ownerid)
{
	UART_CTRL_CLOSE_T data;
	data.u4OwenrId = ownerid;
	YXOS_UART_function(port, SIO_CMD_CLOSE, (DCL_CTRL_DATA_T*)&data);
}

static void SerialPort_UART_Purge(UART_PORT port, UART_buffer dir, module_type ownerid)
{
	UART_CTRL_PURGE_T data;
	data.u4OwenrId = ownerid;
	data.dir = dir;
	YXOS_UART_function(port,SIO_CMD_PURGE, (DCL_CTRL_DATA_T*)&data);
}

static void SerialPort_UART_ClrRxBuffer(UART_PORT port, module_type ownerid)
{
	UART_CTRL_CLR_BUFFER_T  data;
	data.u4OwenrId = ownerid;
	YXOS_UART_function(port, SIO_CMD_CLR_RX_BUF, (DCL_CTRL_DATA_T*)&data);
}

static void SerialPort_UART_ClrTxBuffer(UART_PORT port, module_type ownerid)
{
	UART_CTRL_CLR_BUFFER_T  data;
	data.u4OwenrId = ownerid;
	YXOS_UART_function(port, SIO_CMD_CLR_TX_BUF, (DCL_CTRL_DATA_T*)&data);
}

static void SerialPort_UART_SetBaudRate(UART_PORT port, UART_baudrate baudrate, module_type ownerid)
{
	UART_CTRL_BAUDRATE_T data;
	data.u4OwenrId = ownerid;
	data.baudrate = baudrate;
	YXOS_UART_function(port, SIO_CMD_SET_BAUDRATE, (DCL_CTRL_DATA_T*)&data);
}

static kal_uint16 SerialPort_UART_GetBytes(UART_PORT port, kal_uint8 *Buffaddr, kal_uint16 Length, kal_uint8 *status, module_type ownerid)
{
	UART_CTRL_GET_BYTES_T data;
	DCL_HANDLE handle;

	data.u4OwenrId = ownerid;
	data.u2Length = Length;
	data.puBuffaddr = Buffaddr;
	data.pustatus = status;
	handle = DclSerialPort_Open(port,0);
	DclSerialPort_Control(handle,SIO_CMD_GET_BYTES, (DCL_CTRL_DATA_T*)&data);
	DclSerialPort_Close(handle);
    return data.u2RetSize;
}

static kal_uint16 SerialPort_UART_PutBytes(UART_PORT port, kal_uint8 *Buffaddr, kal_uint16 Length, module_type ownerid)
{
	UART_CTRL_PUT_BYTES_T data;

	data.u4OwenrId = ownerid;
	data.u2Length = Length;
	data.puBuffaddr = Buffaddr;
	
	YXOS_UART_function(port, SIO_CMD_PUT_BYTES, (DCL_CTRL_DATA_T*)&data);
	return data.u2RetSize;
}

static module_type SerialPort_UART_GetOwnerID(UART_PORT port)
{
	UART_CTRL_OWNER_T data;
	YXOS_UART_function(port, SIO_CMD_GET_OWNER_ID, (DCL_CTRL_DATA_T*)&data);
	return (module_type)(data.u4OwenrId);
}

#if 0
static void SerialPort_UART_Register_RX_cb(DCL_DEV port, DCL_UINT32 ownerid, DCL_UART_RX_FUNC func)
{
	UART_CTRL_REG_RX_CB_T data;
	data.u4OwenrId = ownerid;
	data.func = func;
	YXOS_UART_function(port, SIO_CMD_REG_RX_CB, (DCL_CTRL_DATA_T*)&data);
}
#endif

void YxAppUartTxFlush(void)
{
    SerialPort_UART_Purge(YXAPP_USE_UART, TX_BUF, YXAPP_UART_TASK_OWNER);		
    SerialPort_UART_ClrTxBuffer(YXAPP_USE_UART, YXAPP_UART_TASK_OWNER);
}

void YxAppUartRxFlush(void)
{
    SerialPort_UART_Purge(YXAPP_USE_UART, RX_BUF, YXAPP_UART_TASK_OWNER);
    SerialPort_UART_ClrRxBuffer(YXAPP_USE_UART, YXAPP_UART_TASK_OWNER);	
}

U16 YxAppUartSendData(U8 *buff,U16 len)
{
	return SerialPort_UART_PutBytes(YXAPP_USE_UART,buff,len,YXAPP_UART_TASK_OWNER);
}

U16 YxAppUartReadData(U8 *buff,U16 Maxlen)
{
	kal_uint8   status = 0;
	return SerialPort_UART_GetBytes(YXAPP_USE_UART, buff, Maxlen, &status, YXAPP_UART_TASK_OWNER);
}

void YxAppUartSetBaudrate(UART_baudrate baudrate)
{
	SerialPort_UART_SetBaudRate(YXAPP_USE_UART, baudrate, YXAPP_UART_TASK_OWNER);
}

#if 0
static void YxAppUartRxCallback(DCL_UINT32 port)
{
	extern void UART_sendilm(UART_PORT port, msg_type msgid);
	UART_sendilm(port,MSG_ID_UART_READY_TO_WRITE_IND);
}
#endif

static void YxAppUartConfigBaudrate(void)
{
	UARTDCBStruct    dcb;
	module_type      owner = (module_type)YXAPP_UART_TASK_OWNER;
	YXOS_UART_function(YXAPP_USE_UART,UART_CMD_BOOTUP_INIT,NULL);
	SerialPort_UART_TurnOnPower(YXAPP_USE_UART,KAL_TRUE);
	SerialPort_UART_Open(YXAPP_USE_UART,owner);
	//SerialPort_UART_Register_RX_cb(YXAPP_USE_UART,owner,YxAppUartRxCallback);
	dcb.baud = YX_GPS_UART_BAUDRATE;
	dcb.dataBits = len_8;
	dcb.stopBits = sb_1;
	dcb.parity = pa_none;
	dcb.flowControl = fc_none;//fc_none;
	dcb.xonChar = 0x11;
	dcb.xoffChar = 0x13;
	dcb.DSRCheck = KAL_FALSE;
	SerialPort_UART_SetDCBConfig(YXAPP_USE_UART,&dcb,owner);
	SerialPort_UART_SetOwner(YXAPP_USE_UART,owner);
}

void YxAppGpsUartOpen(void)
{
	module_type owner = SerialPort_UART_GetOwnerID(YXAPP_USE_UART);
	if(owner != YXAPP_UART_TASK_OWNER)
	{
		SerialPort_UART_Close(YXAPP_USE_UART,owner);
		SerialPort_UART_SetOwner(YXAPP_USE_UART,YXAPP_UART_TASK_OWNER);
	}
	YxAppUartConfigBaudrate();
}

void YxAppGpsUartClose(void)
{
	SerialPort_UART_Close(YXAPP_USE_UART,YXAPP_UART_TASK_OWNER);
	SerialPort_UART_TurnOnPower(YXAPP_USE_UART,KAL_FALSE);
}

kal_bool YxAppUartTaskInit(task_indx_type task_indx)
{
	return KAL_TRUE;
}

void YxAppUartTaskMain(task_entry_struct * task_entry_ptr)
{
	ilm_struct current_ilm;
	kal_uint32 task_index=0;
	kal_get_my_task_index(&task_index);
	stack_set_active_module_id(task_index, MOD_YXAPP);
#if(YX_GSENSOR_SURPPORT!=0)
	YxGsensortimerId = kal_create_timer((kal_char*)"yxsensor timer");
#endif
#if(YX_SECOND_TICK_WAY==1)
	stack_init_timer(&YxappTaskStackTimer, "YXAPP Timer", MOD_YXAPP);
#endif
	while(1)
	{
		receive_msg_ext_q_for_stack(task_info_g[task_index].task_ext_qid, &current_ilm);
		stack_set_active_module_id(task_index, current_ilm.dest_mod_id);
		switch(current_ilm.msg_id)
		{
		case MSG_ID_UART_READY_TO_READ_IND:
		case MSG_ID_UART_READY_TO_WRITE_IND:
			GpsTaskDealWithMessage();
			break;
#if(YX_GSENSOR_SURPPORT==1)
		case MSG_ID_USB_TEST_START_IND:
			YxMMIProcGsensorMsgHdler(NULL);
			break;
#endif
#if(YX_SECOND_TICK_WAY==1)
		case MSG_ID_TIMER_EXPIRY:
			{
				stack_timer_struct *pStackTimerInfo=(stack_timer_struct *)current_ilm.local_para_ptr;
				if(pStackTimerInfo->timer_indx==YXAPP_SECOND_TIMER_ID)
				{
					YxAppHeartTickProc();
					//YxAppSecondTickProc();
					YxAppStartSecondTimer(1);
				}
			}
			break;
#endif
		}
		free_ilm(&current_ilm);
	}
}

#if(YXAPP_UART_TASK_OWNER_WAY==1)||defined(YXAPP_WIFI_UART)
static MMI_BOOL yx_uart_readyToRead_ind_hdler(void *msg)
{
	uart_ready_to_read_ind_struct* uart_rtr_ind = (uart_ready_to_read_ind_struct*)msg;
	module_type      owner = SerialPort_UART_GetOwnerID(uart_rtr_ind->port);
#if defined(YXAPP_WIFI_UART)
	if(owner!= YXAPP_WIFI_UART_MOD)
        return MMI_FALSE;
	YxAppWifiUartDealWithMessage();
#else
	if(owner!= YXAPP_UART_TASK_OWNER)
        return MMI_FALSE;
	GpsTaskDealWithMessage();
#endif
	return MMI_TRUE;
}

void YxAppRegisterUartMsgToMMiTask(void)
{
    mmi_frm_set_protocol_event_handler(MSG_ID_UART_READY_TO_READ_IND, (PsIntFuncPtr)yx_uart_readyToRead_ind_hdler, MMI_FALSE);
}
#endif

kal_bool YxAppUartTaskCreate(comptask_handler_struct **handle)
{
	static const comptask_handler_struct yxdc_handler_info =
	{
		YxAppUartTaskMain,			/* task entry function */
		YxAppUartTaskInit,		  	/* task initialization function */
		NULL, 				/* task configuration function */
		NULL,			/* task reset handler */
		NULL			/* task termination handler */
	};
	*handle = (comptask_handler_struct *)&yxdc_handler_info;
	return KAL_TRUE;
}

#if(YX_IS_TEST_VERSION!=0)
void YxAppTestUartSendData(U8 *dataBuf,U16 dataLen)
{
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,(char*)dataBuf);
#else
	extern void rmmi_write_to_uart(kal_uint8 * buffer, kal_uint16 length, kal_bool stuff);
	rmmi_write_to_uart(dataBuf,dataLen,KAL_FALSE);
#endif
}
#endif

#if(YX_SECOND_TICK_WAY==2)
static void YxAppSecondTimerCallback(void)
{
	StopTimer(JMMS_SEND_TIMEOUT_TIMER);
//	YxAppTimerMsgToMMIMod();
	YxAppHeartTickProc();
	StartTimer(JMMS_SEND_TIMEOUT_TIMER,YX_HEART_TICK_UNIT,YxAppSecondTimerCallback);
}
#endif

void YxAppStartSecondTimer(char start)
{
#ifndef __USE_YX_ONLY_SIMCARD_NUMBER__
#if(YX_SECOND_TICK_WAY==1)
	if(start)
		stack_start_timer(&YxappTaskStackTimer, YXAPP_SECOND_TIMER_ID, KAL_TICKS_5_SEC);//每5秒一个TICK,KAL_TICKS_5_SEC
	else
		stack_stop_timer(&YxappTaskStackTimer);
#else
	if(start)
		StartTimer(JMMS_SEND_TIMEOUT_TIMER,YX_HEART_TICK_UNIT,YxAppSecondTimerCallback);
	else
		StopTimer(JMMS_SEND_TIMEOUT_TIMER);
#endif
#endif
}

void YxAppHeartTickProc(void)
{
	YxAppUploadHeartProc();
}

void YxAppCloseSearchWtTone(void)//press end key to stop melody
{
	if(yxPlayToneTick>0)
	{
		srv_profiles_stop_tone(SRV_PROF_TONE_FM);
		yxPlayToneTick = 0;
	}
}

#ifdef YXAPP_WIFI_UART
static void YxAppWifiUartConfigBaudrate(void)
{
	UARTDCBStruct    dcb;
	module_type      owner = (module_type)YXAPP_WIFI_UART_MOD;
	YXOS_UART_function(YXAPP_WIFI_UART,UART_CMD_BOOTUP_INIT,NULL);
	SerialPort_UART_TurnOnPower(YXAPP_WIFI_UART,KAL_TRUE);
	SerialPort_UART_Open(YXAPP_WIFI_UART,owner);
	//SerialPort_UART_Register_RX_cb(YXAPP_USE_UART,owner,YxAppUartRxCallback);
	dcb.baud = YXAPP_WIFI_UART_BAUD;
	dcb.dataBits = len_8;
	dcb.stopBits = sb_1;
	dcb.parity = pa_none;
	dcb.flowControl = fc_none;//fc_none;
	dcb.xonChar = 0x11;
	dcb.xoffChar = 0x13;
	dcb.DSRCheck = KAL_FALSE;
	SerialPort_UART_SetDCBConfig(YXAPP_WIFI_UART,&dcb,owner);
	SerialPort_UART_SetOwner(YXAPP_WIFI_UART,owner);
}

static YxAppWifiApTimerOut(void)
{
	YxAppWifiClose();
	YxAppStepRunMain(YX_RUNKIND_WIFI);
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"UART WIFI time out");
#endif
}

void YxAppWifiOpen(void)
{
	U8   oldFlg = wifiParam.wifiReady;
	module_type owner;
#ifndef WIN32
	YxAppWifiPowerCtrl(1);
#endif
	owner = SerialPort_UART_GetOwnerID(YXAPP_WIFI_UART);
	YxAppDisableSleep();
	if(owner != YXAPP_WIFI_UART_MOD)
	{
		SerialPort_UART_Close(YXAPP_WIFI_UART,owner);
		SerialPort_UART_SetOwner(YXAPP_WIFI_UART,YXAPP_WIFI_UART_MOD);
	}
	YxAppWifiUartConfigBaudrate();
	memset((void*)&wifiParam,0,sizeof(YXWIFIPARAM));
	wifiParam.wifiReady = oldFlg | 0x01;
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"UART WIFI open ok");
#endif
	StopTimer(AVATAR_DELAY_DECODE_4);
	StartTimer(AVATAR_DELAY_DECODE_4,YXAPP_WIFI_AP_TIME_OUT,YxAppWifiApTimerOut);
	YxAppWifiUartSendData((U8*)"AT+RST\r\n",8);
}

void YxAppWifiClose(void)
{
	if(wifiParam.wifiReady&0x0F)
	{
		SerialPort_UART_Close(YXAPP_WIFI_UART,YXAPP_WIFI_UART_MOD);
		SerialPort_UART_TurnOnPower(YXAPP_WIFI_UART,KAL_FALSE);
#ifndef WIN32
		YxAppWifiPowerCtrl(0);
#endif
		YxAppEnableSleep();
		wifiParam.cmdIdx = 0;
		wifiParam.wifiReady &= 0xF0;
#if (YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"UART WIFI close");
#endif
	}
}

void YxAppWifiUartTxFlush(void)
{
    SerialPort_UART_Purge(YXAPP_WIFI_UART, TX_BUF, YXAPP_WIFI_UART_MOD);		
    SerialPort_UART_ClrTxBuffer(YXAPP_WIFI_UART, YXAPP_WIFI_UART_MOD);
}

void YxAppWifiUartRxFlush(void)
{
    SerialPort_UART_Purge(YXAPP_WIFI_UART, RX_BUF, YXAPP_WIFI_UART_MOD);
    SerialPort_UART_ClrRxBuffer(YXAPP_WIFI_UART, YXAPP_WIFI_UART_MOD);	
}

U16 YxAppWifiUartSendData(U8 *buff,U16 len)
{
	return SerialPort_UART_PutBytes(YXAPP_WIFI_UART,buff,len,YXAPP_WIFI_UART_MOD);
}

U16 YxAppWifiUartReadData(U8 *buff,U16 Maxlen)
{
	kal_uint8   status = 0;
	return SerialPort_UART_GetBytes(YXAPP_WIFI_UART, buff, Maxlen, &status, YXAPP_WIFI_UART_MOD);
}

static char YxAppWifiStrcmp(S8 *srcStr,S8 *destStr,U16 len)
{
	if(srcStr==NULL || destStr==NULL)
		return 1;
	else
	{
		U16  i = 0;
		while(i<len)
		{
			if(srcStr[i] && (srcStr[i] != destStr[i]))
				return 1;
			i++;
		}
		if(i!=len)
			return 1;
		return 0;
	}
}

static void YxAppWifiUartDealWithMessage(void)
{
	U8    rbuffer[501];
	U16   rLen = YxAppWifiUartReadData(rbuffer,500),i = 0;
	U8    *datBuf = rbuffer+rLen; 
	while(rLen<500)
	{
		kal_sleep_task(10);
		i = YxAppWifiUartReadData(datBuf,500-rLen);
		if((i==0)||(rLen + i >= 500))
			break;
		datBuf += i;
		rLen += i;
	}
	YxAppWifiUartRxFlush();
	i = 0;
	if(rLen>0)
	{
		rbuffer[500] = 0;
		i = 0;
		while(i<rLen)
		{
			if((rbuffer[i]=='O')||((rbuffer[i]=='+')&&(rbuffer[i+1]=='C'))||(rbuffer[i]=='E')||(rbuffer[i]=='b')||(rbuffer[i]=='r'))
			{
				if((YxAppWifiStrcmp((S8*)(rbuffer+i),"OK",2)==0)||(YxAppWifiStrcmp((S8*)(rbuffer+i),"ready",5)==0))
				{
					if(wifiParam.wifiReady&0xF0==0)
					{
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
						extern char YxAutoTestCheckNeedTest(void);
#endif
#endif
						wifiParam.wifiReady |= 0x10;//for auto test
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
						if(YxAutoTestCheckNeedTest()==1)
						{
							YxAppWifiClose();
							return;
						}
#endif
#endif
					}
					if(wifiParam.cmdIdx<3)
					{
						if(wifiParam.cmdIdx==0)
							YxAppWifiUartSendData((U8*)"AT+CWMODE=1\r\n",13);
						else if(wifiParam.cmdIdx==1)
							YxAppWifiUartSendData((U8*)"AT+RST\r\n",8);
						else
							YxAppWifiUartSendData((U8*)"AT+CWLAP\r\n",10);
						wifiParam.cmdIdx++;
					}
					else
						YxAppWifiUartSendData((U8*)"AT+CWLAP\r\n",10);
#if 0//(YX_IS_TEST_VERSION==2)
					kal_prompt_trace(MOD_BT,"UART WIFI in 1:%d",wifiParam.cmdIdx);
#endif
					return;
				}
				else if(YxAppWifiStrcmp((S8*)(rbuffer+i),"+CWLAP:",7)==0)//ap list
				{
					//add later list
					U16   j = 8,tLen = rLen - i;
					U8    k = 0,*tempStr = rbuffer+i,p = 0,isEnd = 0;
					while(j<tLen)
					{
						if((tempStr[j]==0x22)&&(p<YXAPP_WIFI_AP_NUMBER))
						{
							k = 0;
							j++;
							while(j<tLen)
							{
								if(tempStr[j]!=0x22)
								{
									if(k<WIFI_AP_NAME_LEN)
										wifiParam.apList[p].apName[k++] = tempStr[j];
								}
								else
									break;
								j++;
							}
							wifiParam.apList[p].apName[k] = 0;
							k = 0;
							j += 2;
							while(j<tLen)
							{
								if(tempStr[j]!=',')
								{
									if(k<WIFI_RF_STR_LEN)
										wifiParam.apList[p].rfStr[k++] = tempStr[j];
								}
								else
									break;
								j++;
							}
							wifiParam.apList[p].rfStr[k] = 0;
							k = 0;
							j += 2;
							while(j<tLen)
							{
								if(tempStr[j]!=0x22)
								{
									if(k<WIFI_MAC_STRING_LEN)
										wifiParam.apList[p].macString[k++] = tempStr[j];
								}
								else
									break;
								j++;
							}
							wifiParam.apList[p++].macString[k] = 0;
							j += 3;
							while(j<tLen)
							{
								if(tempStr[j]==0x0D && tempStr[j+1]==0x0A)
								{
									j += 2;
									if(YxAppWifiStrcmp((S8*)(tempStr+j),"+CWLAP:",7)!=0)
									{
										isEnd = 1;
										break;
									}
									else
									{
										j += 7;
										break;
									}
								}
								j++;
							}
						}
						if(isEnd)
							break;
						j++;
					}
#if (YX_IS_TEST_VERSION==2)
					k = 0;
					while(k<p)
					{
						kal_prompt_trace(MOD_BT,"chongchi-software ap %d:%s,%s,%s\r\n",k,wifiParam.apList[k].apName,wifiParam.apList[k].macString,wifiParam.apList[k].rfStr);
						k++;
					}
#endif
					YxAppWifiClose();
					YxAppStepRunMain(YX_RUNKIND_WIFI);
					return;
				}
				else if(YxAppWifiStrcmp((S8*)(rbuffer+i),"ERROR",5)==0)
				{
					wifiParam.cmdIdx = 0;
					YxAppWifiUartSendData((U8*)"AT+RST\r\n",8);
					return;
				}
				else if(YxAppWifiStrcmp((S8*)(rbuffer+i),"busy",4)==0)
					return;
			}
			i++;
		}
		if(wifiParam.cmdIdx==0)
			YxAppWifiUartSendData((U8*)"AT+CWMODE=1\r\n",13);
		else if(wifiParam.cmdIdx==1)
			YxAppWifiUartSendData((U8*)"AT+RST\r\n",8);
		else //if(wifiParam.cmdIdx==3)
			YxAppWifiUartSendData((U8*)"AT+CWLAP\r\n",10);
#if 0//(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"UART WIFI in 3:%d,idx:%d",rLen,wifiParam.cmdIdx);
#endif
		return;
	}
}

YXWIFIAPPARAM *YxAppGetWlanApListInfor(U8 index)
{
	if(index<YXAPP_WIFI_AP_NUMBER)
	{
		if(strlen((S8*)(wifiParam.apList[index].macString))!=WIFI_MAC_STRING_LEN)
			return NULL;
		return &wifiParam.apList[index];
	}
	return NULL;
}

#ifdef __USE_YX_AUTO_TEST__
char YxAtUartWifiChipIsOk(void)
{
	if(wifiParam.wifiReady & 0x10)
		return 1;
	return 0;
}
#endif
#endif

/////////////////////////////////////////////////////////////////////GSENSOR APIS//////////////////////////////////////////////////////////////
#if(YX_GSENSOR_SURPPORT!=0)
static void YxAppGsensorSampleCb(void *parameter)
{
#ifndef WIN32
	extern short PEDO_ProcessAccelarationData(short v_RawAccelX_s16r, short v_RawAccelY_s16r, short v_RawAccelZ_s16r);
	extern kal_uint32 CTIRQ1_2_Mask_Status(void);
	extern unsigned long PEDO_GetStepCount(void);
	kal_uint32 CTIRQ_status = CTIRQ1_2_Mask_Status();
	if(CTIRQ_status) //all interrupt masked except CTIRQ_1/2
    {
        kal_set_timer(YxGsensortimerId,(kal_timer_func_ptr)YxAppGsensorSampleCb,NULL,1,0); //wait 1 more tick to execute callback
        return;
    }
	else
	{
		kal_int16   x_adc = 0,y_adc = 0,z_adc = 0;
		if(YxAppGsensorSampleData(&x_adc,&y_adc,&z_adc)==1)
		{
			PEDO_ProcessAccelarationData(x_adc, y_adc, z_adc);
			yxGsensorParam.stepCount = yxSystemSet.todaySteps + PEDO_GetStepCount();
#if(YX_IS_TEST_VERSION==2)
			//kal_prompt_trace(MOD_BT, "x_adc:%d,y_adc:%d,z_adc:%d,step:%d\r\n",x_adc,y_adc,z_adc,yxGsensorParam.stepCount);
#endif
		}
		kal_set_timer(YxGsensortimerId,(kal_timer_func_ptr)YxAppGsensorSampleCb,NULL,yxGsensorParam.sample_period,0);
		return;
	}
#endif
}

static void YxAppGsensorSample(kal_bool enable)
{
    if(enable)
    {
		if(yxGsensorTimerFlag==1)
			kal_cancel_timer(YxGsensortimerId);   
        kal_set_timer(YxGsensortimerId,(kal_timer_func_ptr)YxAppGsensorSampleCb,NULL,yxGsensorParam.sample_period,0);
		yxGsensorTimerFlag = 1;
		return;
    }  
    else
	{
		if(yxGsensorTimerFlag==1)
		{
			kal_cancel_timer(YxGsensortimerId);
			yxGsensorTimerFlag = 0;
		}
		return;
	}
}

void YxAppGsensorProcInit(void)
{
#ifndef WIN32
	YxAppGsensorInition();
#endif
	yxGsensorParam.sample_period = 9;//15;//250;
	yxGsensorParam.stepCount = 0;
	yxGsensorParam.stepReset = 0;
	yxGsensorParam.sleepCount = 0;
	yxGsensorParam.stepDisplay = 0;
	yxGsensorParam.sleepStartStep = 0;
	yxGsensorParam.timerCount = 0xFF;
	yxGsensorParam.eqcount = 0;
	yxGsensorParam.oldStep = 1;
	yxGsensorParam.lastOldStep = 0;
#ifndef WIN32
	if(yxSystemSet.gsensor==1)
		YxAppGsensorPedometerStart(1);
	else
		YxAppGsensorPowerControl(0);
#endif
}

void YxAppGsensorPedometerStart(U8 start)
{
#ifndef WIN32
	if(start && (yxSystemSet.gsensor==1))
	{
		extern void PEDO_InitAlgo(unsigned char v_GRange_u8r);
		if(yxSystemSet.todaySteps>0)
		{
			MYTIME  oldtime;
			U16     date = 0;
			GetDateTime(&oldtime);
			date = (oldtime.nMonth<<8)|oldtime.nDay;
			if(date != yxSystemSet.dateSteps)
				yxSystemSet.todaySteps = 0;
		}
		YxAppGsensorSample(KAL_FALSE);
		if(yxGsensorParam.stepCount == 0)
			PEDO_InitAlgo(0);
		if(yxSystemSet.todaySteps>0)
			yxGsensorParam.stepCount = yxSystemSet.todaySteps;
		YxAppGsensorPowerControl(1);
		YxAppGsensorSample(KAL_TRUE);
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT, "pedometer start:%d,on:%d\r\n",yxGsensorParam.stepCount,yxSystemSet.gsensor);
#endif
	}
	else
	{
		StopTimer(AVATAR_DELAY_DECODE_0);
		YxAppGsensorSample(KAL_FALSE);
		YxAppGsensorPowerControl(0);
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT, "pedometer off:%d,on:%d\r\n",yxGsensorParam.stepCount,yxSystemSet.gsensor);
#endif
	}
#endif
}

void YxAppGsensorCheckCurrentTime(U8 hour,U8 min)
{
	if(yxSystemSet.gsensor==0)
		return;
	if(hour==0 && min == 0 && yxGsensorParam.stepCount)//reset step
	{
		if(yxGsensorParam.stepReset==0)
		{
			if(yxGsensorParam.stepCount >= yxGsensorParam.sleepStartStep)
				yxGsensorParam.sleepCount = yxGsensorParam.stepCount - yxGsensorParam.sleepStartStep;
			yxGsensorParam.sleepStartStep = yxGsensorParam.sleepCount;
			yxGsensorParam.stepCount = 0;
			yxSystemSet.todaySteps = 0;
			YxAppGsensorPedometerStart(1);
			yxGsensorParam.stepReset = 1;
#if(YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT, "pedometer 0 reset:%d,on:%d\r\n",yxGsensorParam.sleepStartStep);
#endif
			return;
		}
	}
	if(yxGsensorParam.stepReset==1 && min!=0)
		yxGsensorParam.stepReset = 0;
	//sleep check time
	if(hour >= 21 || hour < 7)
	{
		if(hour==21 && min == 0 && yxGsensorParam.sleepStartStep == 0)
			yxGsensorParam.sleepStartStep = yxGsensorParam.stepCount;
		if(hour >= 21)
		{
			if(yxGsensorParam.stepCount >= yxGsensorParam.sleepStartStep)
				yxGsensorParam.sleepCount = yxGsensorParam.stepCount - yxGsensorParam.sleepStartStep;
		}
		else
			yxGsensorParam.sleepCount = yxGsensorParam.sleepStartStep + yxGsensorParam.stepCount;
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT, "pedometer sleep:%d\r\n",yxGsensorParam.sleepCount);
#endif
	}
	else
		yxGsensorParam.sleepCount = 0;
}

U16 YxAppGsensorGetStep(char sleep)
{
	if(sleep)
		return yxGsensorParam.sleepCount;
	return yxGsensorParam.stepCount;
}

void YxAppMinTickCheckSteps(void)
{
	if(yxGsensorParam.timerCount < 0xFF)
	{
		if(yxGsensorParam.timerCount)
			yxGsensorParam.timerCount--;
		if(yxGsensorParam.timerCount==0)
		{
			if(yxSystemSet.gsensor==1)
				YxAppGsensorPedometerStart(1);
			yxGsensorParam.timerCount = 0xFF;
#if(YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT, "pedometer restart:%d\r\n",yxSystemSet.gsensor);
#endif
		}
		return;
	}
	if(yxGsensorParam.oldStep == yxGsensorParam.stepCount)
	{
		if(yxGsensorParam.lastOldStep != yxGsensorParam.oldStep)
		{
			yxGsensorParam.eqcount = 1;
			yxGsensorParam.lastOldStep = yxGsensorParam.oldStep;
		}
		else
		{
			if(yxGsensorParam.eqcount<255)
				yxGsensorParam.eqcount++;
		}
		if(yxSystemSet.gsensor==1)
		{
			if(yxGsensorParam.eqcount==1)//dont' move in one minute
				yxGsensorParam.timerCount = 5;//every 5 minutes start again
			else if(yxGsensorParam.eqcount<=5)
				yxGsensorParam.timerCount = 10;
			else if(yxGsensorParam.eqcount<=10)
				yxGsensorParam.timerCount = 20;
			else if(yxGsensorParam.eqcount<=20)
				yxGsensorParam.timerCount = 30;
			else if(yxGsensorParam.eqcount<=30)
				yxGsensorParam.timerCount = 40;
			else
				yxGsensorParam.timerCount = 60;
#if(YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT, "pedometer paused:%d,count:%d,step:%d\r\n",yxGsensorParam.timerCount,yxGsensorParam.eqcount,yxGsensorParam.oldStep);
#endif
		}
		YxAppGsensorPedometerStart(0);
		return;
	}
	yxGsensorParam.oldStep = yxGsensorParam.stepCount;
}

void YxAppStepRestartByScreenLightOn(char kind)
{
	if(yxGsensorParam.timerCount!=0xFF)
	{
		if(kind==0)
		{
			if(yxGsensorParam.timerCount>5)
				yxGsensorParam.timerCount = 5;
			return;
		}
		if(yxSystemSet.gsensor==1)
			YxAppGsensorPedometerStart(1);
		yxGsensorParam.timerCount = 0xFF;
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT, "pedometer start by key:%d\r\n",yxSystemSet.gsensor);
#endif
	}
}
#endif

char YxAppGsensorStatus(void)
{
	return (yxSystemSet.gsensor==1)?1:0;
}

#if(YX_GSENSOR_SURPPORT!=0)
static void YxAppGsensorTimerCheckCb(void)
{
	if(yxGsensorParam.stepDisplay != yxGsensorParam.stepCount)
	{
		IdleShowPedometerSteps(1);
		yxGsensorParam.stepDisplay != yxGsensorParam.stepCount;
	}
	StartTimer(AVATAR_DELAY_DECODE_0,1000,YxAppGsensorTimerCheckCb);
}

void YxAppGsensorIdleTimerCheck(char start)
{
#ifdef IK188_BB
	extern char YxAppIsOnPedometerScreen(void);
	if(YxAppIsOnPedometerScreen()==0)
		return;
#else
	extern U16 on_idle_screen;
	if(on_idle_screen==0)
		return;
#endif
	StopTimer(AVATAR_DELAY_DECODE_0);
	if((start==0)||(yxSystemSet.gsensor==0))
		return;
	StartTimer(AVATAR_DELAY_DECODE_0,1000,YxAppGsensorTimerCheckCb);
}

void YxAppSaveTodaySteps(void)
{
	if(yxGsensorParam.stepCount != yxSystemSet.todaySteps)
	{
		MYTIME  oldtime;
		GetDateTime(&oldtime);
		yxSystemSet.dateSteps = (oldtime.nMonth<<8)|oldtime.nDay;
		yxSystemSet.todaySteps = yxGsensorParam.stepCount;
		YxAppSaveSystemSettings();
	}
}
#endif
/////////////////////////////////////////////////////////////////////WLAN APIS//////////////////////////////////////////////////////////////
#ifdef __MMI_WLAN_FEATURES__
static U8 yxapp_wlan_list_auto_conn_start(srv_dtcnt_wlan_scan_result_struct *scan_res)
{
    U8      index,j = 0;
	memset((void*)&wlanParams,0,sizeof(WLANMACPARAM)*WNDRV_MAX_SCAN_RESULTS_NUM);
    for(index = 0; index < scan_res->ap_list_num && index < WNDRV_MAX_SCAN_RESULTS_NUM; index++)
    {
        if(scan_res->ap_list[index]->ssid_len == 0 || scan_res->ap_list[index]->ssid[0] == 0)
            continue;   /* ignore invalid networks */        
#ifdef __WAPI_SUPPORT__
#ifdef __MMI_HIDE_WAPI__
        if(scan_res->ap_list[index]->wapi_ie_info_p)
            continue;   /* ignore WAPI networks if need to hide it in MMI */
#endif  /* __MMI_HIDE_WAPI__ */
#endif  /* __WAPI_SUPPORT__ */
        if ((TRUE == scan_res->ap_list[index]->rsn_ie_info_p) &&
			(WNDRV_SUPC_NETWORK_IBSS == scan_res->ap_list[index]->network_type) &&
			(scan_res->ap_list[index]->rsn_ie_info.key_mgmt & WPA_KEY_MGMT_WPA_NONE) == 0)
        {
            continue;   /* Discard hidden AP */
        }
		memcpy((void*)(wlanParams[j].bssid),(void*)(scan_res->ap_list[index]->bssid),WNDRV_MAC_ADDRESS_LEN);
		memcpy((void*)(wlanParams[j].ssid),(void*)(scan_res->ap_list[index]->ssid),scan_res->ap_list[index]->ssid_len);
		wlanParams[j].rssi = scan_res->ap_list[index]->rssi;
		j++;
	}
#if(YX_IS_TEST_VERSION!=0)
	if(j>0)
	{
		U8  buffer[100],*tempP = NULL;
		index = 0;
		while(index<j)
		{
			tempP = wlanParams[index].bssid;
			sprintf((char*)buffer,"T:%d,MAC:%d,%d,%d,%d,%d,%d,SS:%s,RF:%d\r\n",j,tempP[0],tempP[1],tempP[2],tempP[3],tempP[4],tempP[5],(char*)wlanParams[index].ssid,wlanParams[index].rssi);
			YxAppTestUartSendData(buffer,(U16)strlen((char*)buffer));
			index++;
		}
	}
#endif
	return j;
}

static void YxAppWlanCloseTimerCb(void)
{
	YxAppWifiIntenalClose();
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"ldz wifi timer close\r\n");
#endif
}

static void yxapp_wlan_list_auto_conn_scan_result_cb(U32 job_id, void *user_data, srv_dtcnt_wlan_scan_result_struct *scan_res)
{
	if(MMI_WLAN_UI_SCANNING == mmi_wlan_get_ui_flow())
        mmi_wlan_set_ui_flow(MMI_WLAN_UI_NONE);
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"ldz wifi cb:%d\r\n",scan_res->result);
#endif
    switch(scan_res->result)
	{
	case SRV_DTCNT_WLAN_SCAN_RESULT_SUCCESS:    /* WLAN scan success */
        if(0 != scan_res->ap_list_num)
            yxapp_wlan_list_auto_conn_start(scan_res);
		else
			memset((void*)&wlanParams,0,sizeof(WLANMACPARAM)*WNDRV_MAX_SCAN_RESULTS_NUM);
		//stop scan
		g_wlan_display_context.send_conn_req_ing = MMI_FALSE;
		if(g_wlan_display_context.scan_job_id > 0)
		{
			srv_dtcnt_wlan_scan_abort(g_wlan_display_context.scan_job_id);
			// make cursor focused on the first item
			g_wlan_display_context.wizard_cur_profile_list_index = 0;
			g_wlan_display_context.scan_job_id = 0;
			mmi_wlan_set_status_bar_icon();
			mmi_wlan_set_ui_flow(MMI_WLAN_UI_ABORT_SCAN);
		}
		StopTimer(AVATAR_DELAY_DECODE_3);
		StartTimer(AVATAR_DELAY_DECODE_3,4000,YxAppWlanCloseTimerCb);
		break;
    case SRV_DTCNT_WLAN_SCAN_RESULT_FAILED:     /* WLAN scan failed */
    case SRV_DTCNT_WLAN_SCAN_RESULT_ABORTED:    /* WLAN scan service is aborted */
		break;
    default:
		break;
	}
}

static char YxAppWifiOpenAndScan(void)//g_wlan_display_context.scan_job_id srv_dtcnt_wlan_auto_pw_on();
{
	srv_dtcnt_wlan_status_enum wlan_status = srv_dtcnt_wlan_status();
#if 0
	if(wlan_status!=SRV_DTCNT_WLAN_STATUS_INACTIVE)
	{
		YxAppWifiIntenalClose();
		wlan_status = srv_dtcnt_wlan_status();
	}
#endif
	if(wlan_status==SRV_DTCNT_WLAN_STATUS_INACTIVE)
	{
		U32 sjob_id = mmi_wlan_send_scan_request(yxapp_wlan_list_auto_conn_scan_result_cb, NULL);
		if(sjob_id>0)
			return 1;
	}
	return 0;
}

static void YxAppWlanStopCb(void *user_data, srv_dtcnt_wlan_req_res_enum res)
{
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"ldz YxAppWlanStopCb:%d\r\n",res);
#endif
	if(res==SRV_DTCNT_WLAN_REQ_RES_DONE)
		YxAppStepRunMain(YX_RUNKIND_WIFI);
}

static void YxAppWifiIntenalClose(void)
{
	srv_dtcnt_wlan_status_enum wlan_status;
	wlan_status = srv_dtcnt_wlan_status();
	if(wlan_status!=SRV_DTCNT_WLAN_STATUS_INACTIVE)
	{
		mmi_wlan_set_ui_flow(MMI_WLAN_UI_NONE);
		srv_dtcnt_wlan_deinit(YxAppWlanStopCb, NULL);
		g_wlan_display_context.deinit_when_connected = MMI_TRUE;
	}
	else
		YxAppStepRunMain(YX_RUNKIND_WIFI);
}

void YxAppWlanPowerScan(char scan)
{
	YxAppSendMsgToMMIMod(YX_MSG_WLAN_POWER_CTRL,scan,0);
}

WLANMACPARAM *YxAppGetWlanScanInfor(U8 index)
{
	if(index<WNDRV_MAX_SCAN_RESULTS_NUM)
	{
		char  i = 0,j = 0;
		while(i<WNDRV_MAC_ADDRESS_LEN)
		{
			if(wlanParams[index].bssid[i]==0)
				j++;
			i++;
		}
		if(j==WNDRV_MAC_ADDRESS_LEN)
			return NULL;
		return &wlanParams[index];
	}
	return NULL;
}

U8 *YxAppSetAndGetLocalMac(U8 *macBuf)
{
	if(macBuf)
		memcpy((void*)yxLocalWlanMac,(void*)macBuf,WNDRV_MAC_ADDRESS_LEN);
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"lmac:%02x:%02x:%02x:%02x:%02x:%02x\r\n",yxLocalWlanMac[0],yxLocalWlanMac[1],yxLocalWlanMac[2],yxLocalWlanMac[3],yxLocalWlanMac[4],yxLocalWlanMac[5]);
#endif
	return yxLocalWlanMac;
}
#endif//wlan
/////////////////////////////////////////////////////////////////////BT APIS//////////////////////////////////////////////////////////////
#ifdef __MMI_BT_SUPPORT__
static U8 YxAppBtGetSearchDevicesInfor(void)
{
	U8   devNum = (U8)srv_bt_cm_get_dev_num(SRV_BT_CM_DISCOVERED_DEV);
	memset((void*)&yxbtInfor,0,sizeof(YXBTPARAM)*YXAPP_BT_MAX_SEARCH_DEVICES);
	if(devNum>0)
	{
		const srv_bt_cm_dev_struct * info_int = NULL;
		U8    i = 0,j = 0;
		for(i=0;i<devNum;i++)
		{
			info_int = srv_bt_cm_get_dev_info_by_index(i,SRV_BT_CM_DISCOVERED_DEV);
			if(NULL != info_int)
			{
				const srv_bt_cm_bt_addr *src = &(info_int->bd_addr);
				yxbtInfor[j].lap = src->lap;
				yxbtInfor[j].uap = src->uap;
				yxbtInfor[j].nap = src->nap;
				strcpy((char*)yxbtInfor[j++].name,(char*)info_int->name);
				if(j>=YXAPP_BT_MAX_SEARCH_DEVICES)
					break;
			}
		}
		return j;
	}
	return 0;
}

static void YxAppBtSetNotifierCb(U32 event, void* para)
{
	switch(event)
	{
	case SRV_BT_CM_EVENT_ACTIVATE:
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("bt open\r\n",9);
#endif
		srv_bt_cm_init_event_handler();
		if(yxBtInSearching&0x10)//need search
			YxAppBtSearchDev();
		return;
	case SRV_BT_CM_EVENT_DEACTIVATE:
		yxBtInSearching = 0;
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("bt close\r\n",10);
#endif
		return;
	case SRV_BT_CM_EVENT_INQUIRY_IND:
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("bt inquiry\r\n",12);
#endif
		return;
	case SRV_BT_CM_EVENT_INQUIRY_COMPLETE:
		if(yxBtInSearching&0x01)
		{
#if(YX_IS_TEST_VERSION!=0)
			U8     num = YxAppBtGetSearchDevicesInfor();
			char   buffer[100];
			if(num>0)
				sprintf(buffer,"BT END:%d,%s,ADD:%d,%d,%d\r\n",num,(char*)yxbtInfor[0].name,yxbtInfor[0].lap,yxbtInfor[0].uap,yxbtInfor[0].nap);
			else
				sprintf(buffer,"BT END:%d\r\n",num);
			YxAppTestUartSendData((U8*)buffer,strlen(buffer));
#else
			YxAppBtGetSearchDevicesInfor();
#endif
			yxBtInSearching = 0;
			if(yxAppBtSrvhd>0)
			{
				srv_bt_cm_reset_notify(yxAppBtSrvhd);
				yxAppBtSrvhd = 0;
			}
			YxAppStepRunMain(YX_RUNKIND_BT_SEARCH);
		}
#if(YX_IS_TEST_VERSION!=0)
		else
			YxAppTestUartSendData("bt inquiry end\r\n",16);
#endif
		/* notify bt AV that inquiry is stopped */
#if defined(__MMI_A2DP_SUPPORT__)
		av_bt_inquiry_stop_callback();
#endif
		/* to notify bt profile that inquiry is stopped */
#if defined(__MMI_BT_AUDIO_VIA_SCO__) || defined(__MMI_BT_FM_VIA_SCO__)
		srv_btsco_inquiry_stop_callback();
#endif
		return;
	case SRV_BT_CM_EVENT_SET_NAME:
		{
			srv_bt_cm_set_name_struct *event = (srv_bt_cm_set_name_struct *)para;
			if(event->result==BTBM_ADP_SUCCESS)
			{
#if(YX_IS_TEST_VERSION!=0)
				YxAppTestUartSendData("set name ok\r\n",13);
#endif
			}
			else
			{
#if(YX_IS_TEST_VERSION!=0)
				YxAppTestUartSendData("set name fail\r\n",15);
#endif
			}
		}
		return;
	case SRV_BT_CM_EVENT_SET_VISIBILITY:
		if(srv_bt_cm_get_visibility() == SRV_BT_CM_VISIBILITY_OFF)
		{
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("set visi off\r\n",14);
#endif
		}
		else
		{
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("set visi on\r\n",13);
#endif
		}
		return;
	case SRV_BT_CM_EVENT_PANIC_IND:
		{
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("set name ldzk\r\n",15);
#endif
			srv_bt_cm_recover_panic_req();
			srv_bt_cm_init();
		}
		return;
	}
}

void YxAppBtSetNotify(U32 event_mask)
{
#if 1
	yxAppBtSrvhd = srv_bt_cm_set_notify((srv_bt_cm_notifier)YxAppBtSetNotifierCb, event_mask, NULL);
#else
	srv_bt_cm_power_status_enum status = srv_bt_cm_get_power_status();
	U32 event_mask = SRV_BT_CM_EVENT_ACTIVATE |
		SRV_BT_CM_EVENT_DEACTIVATE  |
		SRV_BT_CM_EVENT_INQUIRY_IND |
		SRV_BT_CM_EVENT_SET_NAME |
		SRV_BT_CM_EVENT_PANIC_IND |
		SRV_BT_CM_EVENT_SET_VISIBILITY |
		SRV_BT_CM_EVENT_INQUIRY_COMPLETE;
	if(yxAppBtSrvhd>0)
		srv_bt_cm_reset_notify(yxAppBtSrvhd);
	yxAppBtSrvhd = srv_bt_cm_set_notify((srv_bt_cm_notifier)YxAppBtSetNotifierCb, event_mask, NULL);
	if((status!=SRV_BT_CM_POWER_ON)&&(status!=SRV_BT_CM_POWER_SWITCHING_ON))
		YxAppBTPowerControl(1);
#endif
}

void YxAppBTPowerControl(char on)
{
	YxAppSendMsgToMMIMod(YX_MSG_BLUETOOTH_CTRL,YX_DATA_BT_POWER_CTRL,on);
}

char YxAppGetBtStatus(void)
{
	srv_bt_cm_power_status_enum status = srv_bt_cm_get_power_status();
	switch(status)
	{
	case SRV_BT_CM_POWER_ON:
		return 1;
	case SRV_BT_CM_POWER_OFF:
		return 0;
	case SRV_BT_CM_POWER_SWITCHING_ON:
		return 2;
	default://SRV_BT_CM_POWER_SWITCHING_OFF:
		return 3;
	}
}

static void YxAppGetBtAddressString(char *addressBuf,U32 lap,U8 uap,U16 nap)//buffer length 13
{
	sprintf(addressBuf,"%02x:%02x:%02x:%02x:%02x:%02x,-10,BT",(nap>>8),(nap&0x00FF),uap,((lap>>16)&0x00FF),((lap>>8)&0x00FF),(lap&0x00FF));
}

char YxAppGetLocalBtMac(U8 *nameBuf,U8 *addressBuf)//SRV_BT_CM_BD_FNAME_LEN,15,nameBuf:is utf8 string,addressbuf 18
{
	if(srv_bt_cm_get_power_status()==SRV_BT_CM_POWER_ON)
	{
		srv_bt_cm_dev_struct* dev_p = (srv_bt_cm_dev_struct*)srv_bt_cm_get_host_dev_info();
		char    i = 0,j = 0;//mmi_chset_ucs2_to_utf8_string,mmi_chset_utf8_to_ucs2_string
		if(nameBuf)
		{
			while(i<SRV_BT_CM_BD_FNAME_LEN)
			{
				if(dev_p->name[i] == 0)
				{
					nameBuf[j++] = 0x00;
					break;
				}
				else
					nameBuf[j++] = dev_p->name[i++];
			}
		}
		else
			j = 1;
		sprintf((char*)addressBuf,"%02x:%02x:%02x:%02x:%02x:%02x",(dev_p->bd_addr.nap>>8),(dev_p->bd_addr.nap&0x00FF),dev_p->bd_addr.uap,((dev_p->bd_addr.lap>>16)&0x00FF),((dev_p->bd_addr.lap>>8)&0x00FF),(dev_p->bd_addr.lap&0x00FF));
		return j;
	}
	return 0;
}

void YxAppSetBtDeviceName(char *nameBuf)//name is utf8 string,max length is SRV_BT_CM_BD_FNAME_LEN
{
	if((nameBuf==NULL)||(strlen(nameBuf)==0))
		return;
	memset((void*)yxDataPool,0,SRV_BT_CM_BD_FNAME_LEN+1);
	strcpy(yxDataPool,nameBuf);
	YxAppSendMsgToMMIMod(YX_MSG_BLUETOOTH_CTRL,YX_DATA_BTSETNAME,0);
	return;
}

void YxAppGetLocalBtNameMac(PU8 *name,PU8 *mac)
{
	if((name==NULL) && (mac==NULL))
	{
		YxAppGetLocalBtMac(yxLocalBtName,yxLocalBtMac);
		return;
	}
	if(name)
		*name = yxLocalBtName;
	if(mac)
		*mac = yxLocalBtMac;
	return;
}

static char YxAppBtSearchDev(void)
{
	if(srv_bt_cm_get_power_status()!=SRV_BT_CM_POWER_ON)
	{
		srv_bt_cm_switch_on();
		yxBtInSearching |= 0x10;
		return 0;
	}
	if(SRV_BT_CM_RESULT_SUCCESS==srv_bt_cm_search(YXAPP_BT_MAX_SEARCH_DEVICES, YXAPP_BT_MAX_SEARCH_TIME, (U32)0xFFFFFFFF, MMI_TRUE))
	{
		yxBtInSearching = 1;
		YxAppBtSetNotify(SRV_BT_CM_EVENT_INQUIRY_COMPLETE);
		return 1;
	}
	else
	{
		yxBtInSearching = 0;
		YxAppStepRunMain(YX_RUNKIND_BT_SEARCH);
	}
	return 0;
}

void YxAppBtSearchDevices(void)
{
	YxAppSendMsgToMMIMod(YX_MSG_BLUETOOTH_CTRL,YX_DATA_BTSEARCH,0);
}

void YxAppBtAbortSearch(void)
{
	YxAppSendMsgToMMIMod(YX_MSG_BLUETOOTH_CTRL,YX_DATA_BT_ABORT_SEARCH,0);
}

void YxAppBtSetVisibility(char on)
{
	YxAppSendMsgToMMIMod(YX_MSG_BLUETOOTH_CTRL,YX_DATA_BT_SET_VISIBILITY,on);
}

char YxAppBtGetDevicesAddress(U8 index,char *addressStr)
{
	U8   i = 0;
	while(i<YXAPP_BT_MAX_SEARCH_DEVICES)
	{
		if(i==index)
		{
			if((yxbtInfor[i].lap == 0)&&(yxbtInfor[i].uap == 0)&&(yxbtInfor[i].nap == 0))
				return 0;
			else
			{
				if(addressStr)
					YxAppGetBtAddressString(addressStr,yxbtInfor[i].lap,yxbtInfor[i].uap,yxbtInfor[i].nap);
				return 1;
			}
		}
		i++;
	}
	return 0;
}
#endif//bt

/////////////////////////////////////////////////////////////////////Cell infor//////////////////////////////////////////////////////////////
#ifdef __NBR_CELL_INFO__
static char YxAppCellSendMsg(msg_type msg_id)
{
#ifdef __MMI_DUAL_SIM__
	extern VMINT vm_sim_get_active_sim_card(void);
    oslModuleType type = MOD_L4C;
    char   sim = 1;
    sim = vm_sim_get_active_sim_card();
    if(1 == sim)
        type = MOD_L4C;
    else if(2 == sim)
        type = MOD_L4C_2; 
    mmi_frm_send_ilm(type, msg_id, NULL, NULL);
#else
    mmi_frm_send_ilm(MOD_L4C, msg_id, NULL, NULL);
#endif
	return 1;
}

static void YxAppCellRegRsp(l4c_nbr_cell_info_ind_struct *msg_ptr)
{
	if(msg_ptr)
	{
		gas_nbr_cell_info_struct cell_info;
		U8      i = 0,j = 0;
		if(KAL_TRUE == msg_ptr->is_nbr_info_valid)
			memcpy((void *)&cell_info, (void *)(&(msg_ptr->ps_nbr_cell_info_union.gas_nbr_cell_info)), sizeof(gas_nbr_cell_info_struct));
		else
			memset((void *)&cell_info, 0, sizeof(gas_nbr_cell_info_struct));	
#if 0
	//	yxCellParam.cellInfor[j].arfcn = cell_info.nbr_meas_rslt.nbr_cells[cell_info.serv_info.nbr_meas_rslt_index].arfcn;
	//	yxCellParam.cellInfor[j].bsic = cell_info.nbr_meas_rslt.nbr_cells[cell_info.serv_info.nbr_meas_rslt_index].bsic;
		yxCellParam.cellInfor[j].rxlev = cell_info.nbr_meas_rslt.nbr_cells[cell_info.serv_info.nbr_meas_rslt_index].rxlev;
		yxCellParam.cellInfor[j].mcc = cell_info.serv_info.gci.mcc;
		yxCellParam.cellInfor[j].mnc = cell_info.serv_info.gci.mnc;
		yxCellParam.cellInfor[j].lac = cell_info.serv_info.gci.lac;
		yxCellParam.cellInfor[j].ci = cell_info.serv_info.gci.ci;
#endif
		yxCellParam.cellNum = cell_info.nbr_cell_num;
		if(yxCellParam.cellNum>YX_APP_MAX_CELL_INFOR_NUM)
			yxCellParam.cellNum = YX_APP_MAX_CELL_INFOR_NUM;
		for(i = 0; (i < cell_info.nbr_cell_num)&&(i<YX_APP_MAX_CELL_INFOR_NUM); i++)
		{
			//yxCellParam.cellInfor[j].arfcn = cell_info.nbr_meas_rslt.nbr_cells[cell_info.nbr_cell_info[i].nbr_meas_rslt_index].arfcn;
			//yxCellParam.cellInfor[j].bsic = cell_info.nbr_meas_rslt.nbr_cells[cell_info.nbr_cell_info[i].nbr_meas_rslt_index].bsic;
			yxCellParam.cellInfor[j].rxlev = cell_info.nbr_meas_rslt.nbr_cells[cell_info.nbr_cell_info[i].nbr_meas_rslt_index].rxlev;
			yxCellParam.cellInfor[j].mcc = cell_info.nbr_cell_info[i].gci.mcc;
			yxCellParam.cellInfor[j].mnc = cell_info.nbr_cell_info[i].gci.mnc;
			yxCellParam.cellInfor[j].lac = cell_info.nbr_cell_info[i].gci.lac;
			yxCellParam.cellInfor[j].ci = cell_info.nbr_cell_info[i].gci.ci;
			j++;
		}
		return;
	}
}

static void YxAppCellTimerOut(void)
{
#if(YX_IS_TEST_VERSION!=0)
	char  buffer[80];
#endif
	StopTimer(AVATAR_DELAY_DECODE_1);
	YxAppCellCancelReq();
//	YxAppCellDeregReq();
#if(YX_IS_TEST_VERSION!=0)
	sprintf(buffer,"LBS end: N:%d,rx:%d,mc:%d,nc:%d,ac:%d,ci:%d\r\n",yxCellParam.cellNum,-1*yxCellParam.cellInfor[0].rxlev,yxCellParam.cellInfor[0].mcc,
		yxCellParam.cellInfor[0].mnc,yxCellParam.cellInfor[0].lac,yxCellParam.cellInfor[0].ci);
	YxAppTestUartSendData((U8*)buffer,strlen(buffer));
#endif
}

static void YxAppCellStartReq(void)
{
	//if((yxCellParam.isRunning>0)||(YXISINCALL(SRV_UCM_CALL_STATE_ALL) > 0))
	if(yxCellParam.isRunning>0)
		return;
	SetProtocolEventHandler(YxAppCellRegRsp,MSG_ID_L4C_NBR_CELL_INFO_REG_CNF);
	SetProtocolEventHandler(YxAppCellRegRsp,MSG_ID_L4C_NBR_CELL_INFO_IND);
	YxAppCellSendMsg(MSG_ID_L4C_NBR_CELL_INFO_REG_REQ);
	yxCellParam.isRunning = 1;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData("LBS_START\r\n",11);
#endif
	//gui_start_timer(YXAPP_LBS_TIMER_OUT,YxAppCellTimerOut);
	StopTimer(AVATAR_DELAY_DECODE_1);
	StartTimer(AVATAR_DELAY_DECODE_1,YXAPP_LBS_TIMER_OUT,YxAppCellTimerOut);
}

static void YxAppCellEndTimer(void)
{
	YxAppStepRunMain(YX_RUNKIND_LBS);
}

static void YxAppCellCancelReq(void)
{
	if(yxCellParam.isRunning)
	{
		ClearProtocolEventHandler(MSG_ID_L4C_NBR_CELL_INFO_IND);
		ClearProtocolEventHandler(MSG_ID_L4C_NBR_CELL_INFO_REG_CNF);
		YxAppCellSendMsg(MSG_ID_L4C_NBR_CELL_INFO_DEREG_REQ);
		yxCellParam.isRunning = 0;
	}
	StartTimer(AVATAR_DELAY_DECODE_1,1000,YxAppCellEndTimer);
	return;
}

void YxAppCellRegReq(void)
{
	YxAppSendMsgToMMIMod(YX_MSG_LBS_CELL_CTRL,YX_DATA_LBS_START,0);
}

void YxAppCellDeregReq(void)
{
	YxAppSendMsgToMMIMod(YX_MSG_LBS_CELL_CTRL,YX_DATA_LBS_CANECL,0);
}

void YxAppAllPsDevicesStartContrl(void)
{
	YxAppSendMsgToMMIMod(YX_MSG_ALL_PS_DEV_CTRL,YX_DEV_START,0);
}

U8 YxAppGetLBSInfor(char *lineBuffer,char idx)//lineBuffer:min length is 25
{
	if(idx>=YX_APP_MAX_CELL_INFOR_NUM+1)
		return yxCellParam.cellNum;
	sprintf(lineBuffer,"%d,%d,%d,%d,%d|",yxCellParam.cellInfor[idx].mcc,yxCellParam.cellInfor[idx].mnc,yxCellParam.cellInfor[idx].lac,yxCellParam.cellInfor[idx].ci,-1*yxCellParam.cellInfor[idx].rxlev);
	return yxCellParam.cellNum;
}

yxapp_cell_info_struct *YxAppLbsGetData(char idx)
{
	if(idx>=yxCellParam.cellNum)
		return NULL;
	return &(yxCellParam.cellInfor[idx]);
}
#endif

/////////////////////////////////////////////////////////////////////Call apis//////////////////////////////////////////////////////////////
static char YxAppIsAvailablePhnum(char *phnum)
{
	while(*phnum != 0)
	{
		if (   *phnum != '0'
			&& *phnum != '1'
			&& *phnum != '2'
			&& *phnum != '3'
			&& *phnum != '4'
			&& *phnum != '5'
			&& *phnum != '6'
			&& *phnum != '7'
			&& *phnum != '8'
			&& *phnum != '9'
			&& *phnum != 'P'
			&& *phnum != '*'
			&& *phnum != '+'
			&& *phnum != '#')
		{
			return 0;
		}
		phnum++;
	}
	return 1;
}

static char YxAppMakeCall(char check,char *number)//ascii code
{
	if(YXISINCALL(SRV_UCM_CALL_STATE_ALL) > 0)
		return 0;
	else
	{
		if((strlen(number) <= 0) || (strlen(number) >= YX_APP_CALL_NUMBER_MAX_LENGTH))
			return 0;
		if(YxAppIsAvailablePhnum(number)==0)
			return 0;
		else
		{
#if(YX_IS_TEST_VERSION!=0)
			char  bgBuf[30];
#endif
			if(check)
				return 1;
#if(YX_IS_TEST_VERSION!=0)
			sprintf(bgBuf,"make call:%s,%d\r\n",number,yxCallParam.callIndex);
			YxAppTestUartSendData((U8*)bgBuf,(U16)strlen(bgBuf));
#endif
			memset((void*)yxDataPool,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
			strcpy(yxDataPool,number);
			YxAppSendMsgToMMIMod(YX_MSG_CALL_CTRL,YX_DATA_MAKE_CALL,0);
		}
		return 1;
	}
}

void YxAppCircyleCallInit(void)
{
	if(yxCallParam.callKind==YX_CALL_CIRCYLE)
		return;
	yxCallParam.callIndex = 0;
}

S8 YxAppMakeWithCallType(S8 callType,S8 *number)
{
	if((number==NULL)&&(YxAppGetNumberItem(0xFF,NULL,NULL)==0))
	{
		mmi_phb_popup_display_ext(STR_ID_PHB_NO_ENTRY_TO_SELECT,MMI_EVENT_FAILURE);
		return 0;
	}
	if(callType==YX_CALL_CIRCYLE)
	{
		S8   i = 0,j = 0;
		if(yxCallParam.callKind==YX_CALL_CIRCYLE)
			return 0;
		while(i<YX_MAX_MONITOR_NUM)
		{
			if(YX_CHECK_MONITOR_INFOR(yxCallParam.yxMonitors[i].number,yxCallParam.yxMonitors[i].names)==1)
				j++;
			i++;
		}
		if(i==j)
			return 0;
		yxCallParam.callKind = callType;
		YxAppLogAdd(LOG_CALL,'0',0);
		return YxAppMakeCall(0,yxCallParam.yxMonitors[0].number);
	}
	else if(callType==YX_CALL_SEND1)
	{
		if(YX_CHECK_MONITOR_INFOR(yxCallParam.yxMonitors[0].number,yxCallParam.yxMonitors[0].names)==1)
		{
			mmi_phb_popup_display_ext(STR_ID_PHB_NO_ENTRY_TO_SELECT,MMI_EVENT_FAILURE);
			return 0;
		}
		yxCallParam.callKind = callType;
		return YxAppMakeCall(0,yxCallParam.yxMonitors[0].number);
	}
	else if(callType==YX_CALL_SEND2)
	{
		if(YX_CHECK_MONITOR_INFOR(yxCallParam.yxMonitors[1].number,yxCallParam.yxMonitors[1].names)==1)
		{
			mmi_phb_popup_display_ext(STR_ID_PHB_NO_ENTRY_TO_SELECT,MMI_EVENT_FAILURE);
			return 0;
		}
		yxCallParam.callKind = callType;
		return YxAppMakeCall(0,yxCallParam.yxMonitors[1].number);
	}
	else if(callType==YX_CALL_LISTEN)//check number
	{
		U16  i = 0;
		if(yxCallParam.number[0]<'0' || yxCallParam.number[0]>'9')
			return 0;
		i = YxAppSearchNumberFromList(yxCallParam.number);
		YxAppLogAdd(LOG_CALL,'0',i);
		number = yxCallParam.number;
	}
	if(YxAppMakeCall(1,number)==1)
	{
		yxCallParam.callKind = callType;
		return YxAppMakeCall(0,number);
	}
	return 0;
}

void YxAppCallStopCircyle(void)
{
	if(yxCallParam.callKind==YX_CALL_CIRCYLE)
		yxCallParam.callKind = YX_CALL_NORMAL;
}

void YxAppCallSetAudioMute(char onoff)
{
	if(onoff==2)
		YxAppCallStopCircyle();
	if(yxCallParam.callKind==YX_CALL_LISTEN)
	{
#if(YX_IS_TEST_VERSION!=0)
		if(onoff)
			YxAppTestUartSendData("yx call mute\r\n",14);
#endif
		if(onoff)
			srv_gpio_set_device_mute(AUDIO_DEVICE_SPEAKER, MMI_TRUE);
		else
			srv_gpio_set_device_mute(AUDIO_DEVICE_SPEAKER, MMI_FALSE);
	}
	else
		srv_gpio_set_device_mute(AUDIO_DEVICE_SPEAKER, MMI_FALSE);
}

void YxAppCallEndProc(void)
{
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData("yx call end\r\n",13);
#endif
	if(yxCallParam.callKind==YX_CALL_LISTEN)
	{
		yxCallParam.callKind = YX_CALL_NORMAL;
		YxAppCallSetAudioMute(0);
		return;
	}
	else if(yxCallParam.callKind==YX_CALL_CIRCYLE)
	{
		char isEnd = 1;
		yxCallParam.callIndex++;
		if(yxCallParam.callIndex<YX_MAX_MONITOR_NUM)
		{
			if(YxAppMakeCall(0,yxCallParam.yxMonitors[yxCallParam.callIndex].number)==1)
				isEnd = 0;
		}
		if(isEnd)
		{
			yxCallParam.callIndex = 0;
			yxCallParam.callKind = YX_CALL_NORMAL;
		}
		return;
	}
}

#if 0
void YxAppEndIncommingCall(void)
{
    srv_ucm_single_call_act_req_struct check_act_req;
    srv_ucm_call_info_struct call_info;
    srv_ucm_index_info_struct call_index;
    if(srv_ucm_query_call_count(SRV_UCM_INCOMING_STATE, SRV_UCM_CALL_TYPE_ALL, NULL) != 1)
        return;
    /* get uid */
    srv_ucm_query_call_count(SRV_UCM_INCOMING_STATE, SRV_UCM_CALL_TYPE_ALL, g_ucm_p->call_misc.index_list);
    call_index.call_index = g_ucm_p->call_misc.index_list[0].call_index;
    call_index.group_index = g_ucm_p->call_misc.index_list[0].group_index;
    if(MMI_FALSE == srv_ucm_query_call_data(call_index, &call_info))
        return;
    memcpy(&(check_act_req.action_uid), &(call_info.uid_info), sizeof(srv_ucm_id_info_struct));
    /* permit check */
    if(srv_ucm_query_act_permit(SRV_UCM_END_SINGLE_ACT, &check_act_req) == SRV_UCM_RESULT_OK)
        srv_ucm_act_request(SRV_UCM_END_SINGLE_ACT, &check_act_req, NULL, NULL);
}
#endif
/////////////////////////////////////////////////////////////////////SMS apis//////////////////////////////////////////////////////////////

static void yxapp_send_sms_callback(srv_sms_callback_struct* callback_data)
{
	if(callback_data->result == MMI_TRUE)//send successfully
	{
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("Sms ok\r\n",8);
#endif
	}
    else
	{
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("Sms fail\r\n",10);
#endif
 	}
}

static void YxAppSendSmsIntenal(S8* number,char gsmCode,U16* content)//ucs2
{
    SMS_HANDLE send_handle;
	U16        addrNum[SRV_SMS_MAX_ADDR_LEN+1],i = 0;
	send_handle = srv_sms_get_send_handle();
    if(send_handle == NULL)
		return;
    if((U16)srv_sms_is_bg_send_action_busy() == MMI_TRUE)
		return;
	while(i<strlen((char*)number))
	{
		addrNum[i] = number[i];
		i++;
		if(i>=SRV_SMS_MAX_ADDR_LEN)
			break;
	}
	addrNum[i] = 0;
    /* set address number */
    srv_sms_set_address(send_handle, (char*)addrNum);
    /* set content of DCS encoding */
    srv_sms_set_content_dcs(send_handle, (gsmCode==1)?SRV_SMS_DCS_7BIT:SRV_SMS_DCS_UCS2);
	srv_sms_set_reply_path(send_handle, MMI_FALSE);
    /* set content */
    srv_sms_set_content(send_handle, (S8*)content, (U16)((YxAppUCSStrlen(content)+1)*2));
    /* set SIM1 */
    srv_sms_set_sim_id(send_handle, SRV_SMS_SIM_1);
	srv_sms_set_no_sending_icon(send_handle);
    /* send request */
    srv_sms_send_msg(send_handle, yxapp_send_sms_callback, NULL);
}

void YxAppSetSmsNumber(S8 *number,U8 length)
{
	U8   offset = 0;
	if(number==NULL || length == 0)
		return;
	memset((void*)&yxSmsParam,0,sizeof(YXAPPSMSPARAM));
	if(number[0]=='+' && number[1]=='8' && number[2]=='6')
		offset = 3;
	else if(number[0]=='+')
		offset = 1;
	number += offset;
	if(strlen(number)<=SRV_SMS_MAX_ADDR_LEN)
		strcpy(yxSmsParam.storeNumber,number);
}

S32 YxAppUCSStrlen(U16 *string)
{
	S32  i = 0;
	if(string==NULL)
		return 0;
	while(string[i]!=0)
	{
		i++;
	}
	return i;
}

void YxAppSendSms(S8* number, U16* content,char gsmCode)
{
	char   useOld = 0;
	if(number==NULL)
	{
		useOld = 1;
		number = (S8*)yxSmsParam.storeNumber;
		strcpy(yxSmsParam.number,number);
	}
	if((number==NULL)||(content==NULL)||(strlen(number)>SRV_SMS_MAX_ADDR_LEN)||(strlen(number)==0)||(YxAppUCSStrlen(content)==0))
		return;
	if(useOld==0)
	{
		memset((void*)&yxSmsParam,0,sizeof(YXAPPSMSPARAM));
		strcpy(yxSmsParam.number,number);
	}
	else
		memset((void*)yxSmsParam.content,0,(YX_APP_MAX_SMS_CHARS+1)<<1);
	mmi_ucs2cpy((CHAR*)yxSmsParam.content,(CHAR*)content);
	YxAppSendMsgToMMIMod(YX_MSG_SMS_CTRL,YX_DATA_SEND_SMS,gsmCode);
}

void YxAppSmsCheckDateTime(MYTIME *timeStamp)
{
	if(timeStamp)
	{
		MYTIME  oldtime;
		GetDateTime(&oldtime);
		if((oldtime.nYear != timeStamp->nYear) || (oldtime.nMonth!=timeStamp->nMonth) || (oldtime.nDay!=timeStamp->nDay)||(oldtime.nHour != timeStamp->nHour))
		{
			mmi_dt_set_dt((const MYTIME*)timeStamp, NULL, NULL);
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData((U8*)"sms set time\r\n",14);
#endif
		}
	}
}

/////////////////////////////////////////////////////////////////////gprs apis//////////////////////////////////////////////////////////////
#define MCF_APN_CMWAP		"cmwap"
#define MCF_APN_CMNET		"cmnet"
#define MCF_APN_UNIWAP		"uniwap"
#define MCF_APN_UNINET		"uninet"
#define MCF_APN_CTWAP		"ctwap"
#define MCF_APN_CTNET		"ctnet"
#define MCF_APN_WIFI		"wifi"

#define MAX_ASC_APN_LEN     19

static char YxAppGetOperatorByPlmn(char *pPlmn)
{
	if(pPlmn)
	{
		if(memcmp(pPlmn,"46000",5)==0 || memcmp(pPlmn,"46002",5)==0 || memcmp(pPlmn,"46007",5)==0)
		{//china mobile
			return MSIM_OPR_CMCC;
		}
		else if(memcmp(pPlmn,"46001",5)==0 || memcmp(pPlmn,"46006",5)==0)
		{//china unicom
			return MSIM_OPR_UNICOM;
		}
		else if(memcmp(pPlmn,"46003",5)==0 || memcmp(pPlmn,"46005",5)==0)
		{//china telcom
			return MSIM_OPR_TELCOM;
		}
		else
			return MSIM_OPR_UNKOWN;
	}
	return MSIM_OPR_NONE;
}

char YxAppGetSimOperator(char simId)
{
	char           plmn[MAX_PLMN_LEN+1]={0};
	mmi_sim_enum   mmiSim = MMI_SIM1;
	if(simId==YX_APP_SIM2)//sim2
		mmiSim = MMI_SIM2;
#ifdef WIN32
	return MSIM_OPR_UNICOM;
#endif
	if(srv_nw_info_get_service_availability(mmiSim) == SRV_NW_INFO_SA_FULL_SERVICE)
	{
		srv_nw_info_location_info_struct nw_info;
		if(srv_nw_info_get_location_info(mmiSim,&nw_info))
			strncpy(plmn,nw_info.plmn,5);
		else
			return MSIM_OPR_NONE;
		return YxAppGetOperatorByPlmn(plmn);
	}
	else
		return MSIM_OPR_NONE;
}

static char *YxAppGetOptionalApn(char simId,char apnType)
{
    char    simOpr = YxAppGetSimOperator(simId);
    char    *pApn = NULL;
	////china mobile
	if(simOpr ==MSIM_OPR_CMCC)
	{
		if(apnType == MAPN_WAP)
			pApn = MCF_APN_CMWAP;
		else if(apnType == MAPN_NET)
			pApn= MCF_APN_CMNET;
	}
	//china unicom
	else if(simOpr == MSIM_OPR_UNICOM)
	{
		if(apnType == MAPN_WAP)
			pApn = MCF_APN_UNIWAP;
		else if(apnType == MAPN_NET)
			pApn= MCF_APN_UNINET;
	}
	//china telcom
	else if(simOpr == MSIM_OPR_TELCOM)
	{
		if(apnType == MAPN_WAP)
			pApn = MCF_APN_CTWAP;
		else if(apnType == MAPN_NET)
			pApn= MCF_APN_CTNET;
	}
	//WIFI
	else if(simOpr == MSIM_OPR_WIFI)
		pApn = MCF_APN_WIFI;
	else
		pApn = MCF_APN_CMWAP;
	return pApn;
}

static char YxAppNetGetAppid(char apn, U8 *app_id)
{
#if 0
	kal_int8 ret = cbm_register_app_id(app_id);
	cbm_hold_bearer(*app_id);
	if(CBM_OK != ret)
		return 0;
#else
	cbm_app_info_struct app_info;
    kal_int8 ret = 0;
	app_info.app_icon_id = 0;
	app_info.app_str_id = 0;
	if(apn==MAPN_WAP)
	{
		app_info.app_type = DTCNT_APPTYPE_MRE_WAP;
		ret = cbm_register_app_id_with_app_info(&app_info, app_id);
	}
	else if(apn==MAPN_NET)
	{
		app_info.app_type = DTCNT_APPTYPE_EMAIL;//DTCNT_APPTYPE_MRE_NET | DTCNT_APPTYPE_NO_PX;
		ret = cbm_register_app_id_with_app_info(&app_info, app_id);
	}
	else//wifi
	{
		app_info.app_type = DTCNT_APPTYPE_MRE_NET | DTCNT_APPTYPE_NO_PX;
		ret = cbm_register_app_id_with_app_info(&app_info, app_id);
	}
	cbm_hold_bearer(*app_id);
	if(0 != ret)
		return 0;
#endif
	return 1;
}

static S32 YxAppGetAccountByApn(char apnType,char simId,U8 *netAppid)
{
	char  *apnName = "cmnet";
	U8    *accountName = (U8*)L"China Mobile Net";
	S32   res = 0;
	char  checkSim = YxAppGetSimOperator(simId);
	if(apnType == MAPN_NET)
	{
		if(MSIM_OPR_UNICOM==checkSim)
		{
			accountName = (U8*)L"Unicom Net";
			apnName = "uninet";
		}
	}
	else if(apnType == MAPN_WAP)
	{
		if(MSIM_OPR_UNICOM==checkSim)
		{
			apnName = "uniwap";
			accountName = (U8*)L"Unicom WAPNet";
		}
		else
		{
			apnName = "cmwap";
			accountName = (U8*)L"China Mobile WAPNet";
		}
	}
	res = YxappAddOneApn(simId,(const U8 *)apnName,(const U8 *)accountName,NULL,NULL);
	if(res != -1)
	{
		if((netAppid)&&(netAppid[0]==CBM_INVALID_APP_ID))
			YxAppNetGetAppid(apnType,netAppid);
	}
	return res;
}

S32 YxappAddOneApn(char simId,const U8 *apnName,const U8 *accountName,char *userName,char *password)
{
	extern srv_dtcnt_result_enum yxapp_srv_dtcnt_get_acc_id_by_apn(S8 *apn,U32 *acc_id_out, S8 SimId);
	U32  accountid = 0;
	srv_dtcnt_result_enum result = SRV_DTCNT_RESULT_FAILED;
	srv_dtcnt_sim_type_enum  SimIdEnum = SRV_DTCNT_SIM_TYPE_1;
	srv_dtcnt_store_prof_data_struct prof_info;
	srv_dtcnt_prof_gprs_struct prof_gprs;
	memset(&prof_gprs, 0, sizeof(prof_gprs));
	if(simId==YX_APP_SIM2)
		SimIdEnum = SRV_DTCNT_SIM_TYPE_2;
	if(SRV_DTCNT_RESULT_SUCCESS == (result = yxapp_srv_dtcnt_get_acc_id_by_apn((S8*)apnName,&accountid,SimIdEnum)))
	{
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"find one:%d,ldz\r\n",accountid);
#endif
		accountid = cbm_get_original_account(accountid);
	}
	if(result != SRV_DTCNT_RESULT_SUCCESS)
	{
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"Add one:%d,apn:%s\r\n",accountid,(char*)apnName);
#endif
		prof_gprs.APN = apnName;
		prof_gprs.prof_common_header.Auth_info.AuthType = SRV_DTCNT_PROF_GPRS_AUTH_TYPE_NORMAL;
		prof_gprs.prof_common_header.sim_info = SimIdEnum;//SRV_DTCNT_SIM_TYPE_1;
		prof_gprs.prof_common_header.AccountName = accountName;
		prof_gprs.prof_common_header.acct_type = SRV_DTCNT_PROF_TYPE_USER_CONF;
	//	prof_gprs.prof_common_header.px_service = SRV_DTCNT_PROF_PX_SRV_HTTP;
		prof_gprs.prof_common_header.use_proxy = 0;
		prof_info.prof_data = &prof_gprs;
		prof_info.prof_type = SRV_DTCNT_BEARER_GPRS;
		prof_info.prof_fields = SRV_DTCNT_PROF_FIELD_ALL;
		result = srv_dtcnt_store_add_prof(&prof_info,&accountid);
		if(SRV_DTCNT_RESULT_SUCCESS == result)
		{
#if(YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT,"update one:%d,ldz\r\n",accountid);
#endif
			srv_dtcnt_store_update_prof(accountid,&prof_info);
		//	accountid = cbm_encode_data_account_id(accountid, CBM_SIM_ID_SIM1, 0, 0);
		}
	}
#if(YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"accoutid:%d,ldz\r\n",accountid);
#endif
	if(SRV_DTCNT_RESULT_SUCCESS == result)
		return (S32)accountid;
	return -1;
}

static U8 YxAppSockNotifyCb(void* inMsg)
{
    app_soc_notify_ind_struct *ind = NULL;
    if(inMsg==NULL)                   
        return MMI_FALSE;
	ind = (app_soc_notify_ind_struct*)inMsg;
	if((ind->socket_id != yxNetContext_ptr.sock)||(yxNetContext_ptr.sock<0))
		return MMI_FALSE;
	StopTimer(YX_SOCKET_TIMER_ID);
	switch(ind->event_type)
	{
	case SOC_CONNECT:
		if(ind->result == KAL_TRUE)
        {
			char  retTimer = 1;
			yxNetContext_ptr.socket_state = YX_NET_STATUS_CONN_CONNECTED;
			yxAppSockData.curSendLen = 0;
			yxAppSockData.sendLen = YxAppSockConnectedCallback(yxAppSockData.sendBuf,YX_SOCK_BUFFER_LEN);
			if(yxAppSockData.sendLen>0)
			{
				if(yxAppSockData.sendLen <= YX_SOCK_BUFFER_LEN)
				{
					retTimer = 0;
					YxAppTcpSockWrite(yxAppSockData.sendBuf, yxAppSockData.sendLen);
				}
				else if(yxAppSockData.sendLen <= YX_BIG_MEMERY_LEN)
				{
					retTimer = 0;
					YxAppTcpSockWrite(YxProtocolGetBigBuffer(), yxAppSockData.sendLen);
				}
			}
			if(retTimer)
				YxAppRestartServerTimer();
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("ldz-21\r\n",7);
#endif
		}
		else
		{
			YxAppCloseSocket(1);
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("ldz-9\r\n",7);
#endif
		}
		break;
	case SOC_READ:
__CONTINUE_READ:
		{
			S32   readLen = YxAppTcpSockRead(yxAppSockData.readBuffer,YX_SOCK_READ_BUFFER_LEN);
			if(readLen>0)
			{
				yxAppSockData.sendLen = YxAppSockReadDataCallback(yxAppSockData.readBuffer,(U16)readLen,yxAppSockData.sendBuf);
				if(yxAppSockData.sendLen>0)
				{
					yxAppSockData.curSendLen = 0;
					YxAppTcpSockWrite(yxAppSockData.sendBuf, yxAppSockData.sendLen);
				}
				else
				{
					if(yxAppSockData.sendLen == -1)//continue read
					{
						YxAppRestartServerTimer();//big data need restart timer again
						goto __CONTINUE_READ;
					}
					else
						YxAppCloseSocket(0);
				}
			}
			else
				YxAppRestartServerTimer();
		}
		break;
	case SOC_WRITE: //MTK GPRS发数据时,一次最大为4K,超过4K,要等此消息后,才能发送数据
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"soc write:%d,%d\r\n",yxAppSockData.curSendLen,yxAppSockData.sendLen);
#endif
		if((yxAppSockData.curSendLen>0) && (yxAppSockData.sendLen > YX_SOCK_BUFFER_LEN))
		{
			if(yxAppSockData.curSendLen >= yxAppSockData.sendLen)//send over
			{
				yxAppSockData.curSendLen = 0;
				yxAppSockData.sendLen = 0;
				YxAppRestartServerTimer();
				break;
			}
			else
				YxAppTcpSockWrite(YxProtocolGetBigBuffer(), yxAppSockData.sendLen);
		}
		else
			YxAppRestartServerTimer();
		break;
	case SOC_CLOSE:
		YxAppCloseSocket(0);
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("ldz-10\r\n",8);
#endif
		break;
	default:
		break;
	}
    return MMI_TRUE;
}

static S8 YxAppSocketConnect(S8 sock,sockaddr_struct *address)
{
	yxNetContext_ptr.socket_state = YX_NET_STATUS_CONN_CONNECTTING;
	switch(soc_connect(sock,address))
	{
	case SOC_SUCCESS:
	case SOC_WOULDBLOCK:
		SetProtocolEventHandler(YxAppSockNotifyCb, MSG_ID_APP_SOC_NOTIFY_IND);
		return 1;
	default:
		YxAppCloseSocket(0);
		return 2;
	}
}

#if __USE_NATTY_PROTOCOL__

#endif


static U8 YxAppGetHostByNameIntCb(void* msg)
{
    app_soc_get_host_by_name_ind_struct *dns_msg = NULL;
    if(NULL == msg)
        return 0;
    dns_msg = (app_soc_get_host_by_name_ind_struct *)msg;
	if(dns_msg->request_id != YX_APP_DNS_REQUEST_ID)
		return 0;
	if((dns_msg->result!=KAL_TRUE)||(dns_msg->num_entry==0))
	{
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"domain name err:%d,%d\r\n",dns_msg->num_entry,dns_msg->result);
#endif
		YxAppCloseSocket(1);
		return 0;
	}
	else
	{
		sockaddr_struct  address;
		memset(&address,0,sizeof(sockaddr_struct));
		address.addr_len = 4;
		address.port = YxAppGetHostNameAndPort(NULL);//yxNetContext_ptr.port;
		if(dns_msg->num_entry>0)
			memcpy((void*)(yxNetContext_ptr.dnsIplist.address),(void*)(dns_msg->entry[0].address),4);
		memcpy((void*)address.addr,(void*)(dns_msg->entry[0].address),4);
#if(YX_IS_TEST_VERSION==2)
		kal_prompt_trace(MOD_BT,"Ser ip:%d.%d.%d.%d\r\n",address.addr[0],address.addr[1],address.addr[2],address.addr[3]);
#endif
		ClearProtocolEventHandler(MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND);
		YxAppSocketConnect(yxNetContext_ptr.sock,&address);
    }
    return 0;
}

static S8 YxAppGetHostByNameInt(const S8 *host,U8 *addr,U8 *addr_len,U32 dtacct_id)
{
    kal_int8 ret = 0;    
    ret = soc_gethostbyname(KAL_FALSE,MOD_MMI,YX_APP_DNS_REQUEST_ID,(const kal_char *)host,(kal_uint8*)addr,(kal_uint8*)addr_len,0,dtacct_id);
	if(ret == SOC_SUCCESS)
	{
		sockaddr_struct  address;
		memset(&address,0,sizeof(sockaddr_struct));
		address.addr_len = 4;
		address.port = YxAppGetHostNameAndPort(NULL);
		memcpy((void*)(yxNetContext_ptr.dnsIplist.address),(void*)addr,4);
		yxNetContext_ptr.socket_state = YX_NET_STATUS_CONN_CONNECTTING;//YX_NET_STATUS_CONN_HOST_BY_NAME;
		memcpy((void*)address.addr,(void*)addr,4);
		YxAppSocketConnect(yxNetContext_ptr.sock,&address);
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("ldz-23\r\n",11);
#endif
	}
	else if(SOC_WOULDBLOCK == ret||ret == SOC_ALREADY)
    {
        yxNetContext_ptr.socket_state = YX_NET_STATUS_CONN_CONNECTTING;//YX_NET_STATUS_CONN_HOST_BY_NAME;
		SetProtocolEventHandler(YxAppGetHostByNameIntCb, MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND);
      //  mmi_frm_set_protocol_event_handler(MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND,YxAppGetHostByNameIntCb,MMI_FALSE);
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("ldz-24\r\n",11);
#endif
    }
    return ret;
}

static U16 YxAppGetProxyInfo(char *pProxy)
{
    if(yxNetContext_ptr.apn == MAPN_WAP)
    {
		char nOptr = YxAppGetSimOperator(YX_APP_SIM1);
		if(nOptr== MSIM_OPR_TELCOM)
        {
            if(pProxy)
				strcpy(pProxy,"10.0.0.200");
        }
        else
        {
            if(pProxy) 
				strcpy(pProxy,"10.0.0.172");
        }
       return 80;
    }
    return 0;
}

static S8 YxAppTcpStartConnect(const S8* host, U16 port, S8 apn)
{
	sockaddr_struct addr;               // connect 地址
	kal_uint8 addr_len = 0;
	kal_int8  ret = -1;
	kal_bool  is_ip_valid = KAL_FALSE;
	if(yxNetContext_ptr.sock<0 || yxNetContext_ptr.socket_state==YX_NET_STATUS_CONN_CONNECTTING)
		return 0;
	memset(&addr,0,sizeof(sockaddr_struct));
	addr.sock_type = SOC_SOCK_STREAM;
	if(apn == MAPN_WAP)    // cmwap 连接
	{
		S8           wapServerIp[21];
		addr.addr_len = 4;
		addr.port = YxAppGetProxyInfo(wapServerIp);
		if(soc_ip_check(wapServerIp, addr.addr, &is_ip_valid) == KAL_TRUE)
		{
			if(is_ip_valid==KAL_FALSE)
			{
				YxAppCloseSocket(0);
				return 2;
			}
		}
		else
		{
			YxAppCloseSocket(0);
			return 2;
		}
	}
	else// cmnet 连接
	{
		if(yxNetContext_ptr.dnsIplist.address[0]>0)//直接用上次获取到的IP地址进行通讯
		{
			memcpy(addr.addr, (void*)yxNetContext_ptr.dnsIplist.address, 4);
			addr.addr_len = 4;
			addr.port = port;
#if(YX_IS_TEST_VERSION==2)
			kal_prompt_trace(MOD_BT,"direct:%d:%d:%d:%d:%d,ipvalid:%d,soc:%d\r\n",addr.addr[0],addr.addr[1],addr.addr[2],addr.addr[3],addr.port,is_ip_valid,yxNetContext_ptr.sock);
#endif
			return YxAppSocketConnect(yxNetContext_ptr.sock,&addr);
		}
		if(soc_ip_check((kal_char*)host, addr.addr, &is_ip_valid) == KAL_FALSE)
		{
			kal_uint8  buf[16]={0};
			ret = YxAppGetHostByNameInt(host, buf, &addr_len, yxNetContext_ptr.account_id); 
			if(ret == SOC_SUCCESS)             // success
			{
				memcpy(addr.addr, buf, 4);
				addr.addr_len = addr_len;
				addr.port = port;
			}
			else if(ret == SOC_WOULDBLOCK)         // block
				return 1;
			else                                    // error
			{
				YxAppCloseSocket(1);
				return 2;
			}
		}
		else if(is_ip_valid == KAL_FALSE)  
		{
			YxAppCloseSocket(0);
			return 2;
		}
		else
		{
			addr.addr_len = 4;
			addr.port = port;
		}
	}
	return YxAppSocketConnect(yxNetContext_ptr.sock,&addr);
}

S8 YxAppTcpConnectToServer(const S8* host, U16 port, S8 apn)
{
    U8     apn_check = (U8)YxAppGetSimOperator(YX_APP_SIM1);
	U32    account_id = 0;
    if(!host  || port == 0 || (apn_check==MSIM_OPR_UNKOWN) || (apn_check==MSIM_OPR_NONE))
        return 0;
    apn_check = (apn != MAPN_WAP && apn != MAPN_NET && apn != MAPN_WIFI);
	if(apn_check)
		return 0;
	yxNetContext_ptr.apn = apn;
   // yxNetContext_ptr.port = port;
	account_id = YxAppDtcntMakeDataAcctId(YX_APP_SIM1,NULL,apn,&(yxNetContext_ptr.appid));
	if(account_id==0)
	{
		yxNetContext_ptr.account_id = 0;
		YxAppCloseAccount();
		return 0;
	}
	yxNetContext_ptr.account_id = account_id;
#ifdef WIN32
    soc_init_win32();
#endif
	yxNetContext_ptr.sock = soc_create(SOC_PF_INET, SOC_SOCK_STREAM, 0, MOD_MMI, yxNetContext_ptr.account_id);
    if(yxNetContext_ptr.sock >= 0)
    {
        kal_uint8 val = KAL_TRUE;
#if 0
		kal_int32 bufLen = YX_SOCK_BUFFER_LEN;
		if(soc_setsockopt(yxNetContext_ptr.sock, SOC_SENDBUF, &bufLen, sizeof(bufLen)) < 0)
		{
			YxAppCloseSocket(0);
			return 2;
		}
		bufLen = YX_SOCK_READ_BUFFER_LEN;
		if(soc_setsockopt(yxNetContext_ptr.sock, SOC_RCVBUF, &bufLen, sizeof(bufLen)) < 0)
		{
			YxAppCloseSocket(0);
			return 2;
		}
#endif
        if(soc_setsockopt(yxNetContext_ptr.sock, SOC_NBIO, &val, sizeof(val)) < 0)
        {
			YxAppCloseSocket(0);
			return 2;
        }
        else
        {   
            val = SOC_READ | SOC_WRITE | SOC_CLOSE | SOC_CONNECT;
            if(soc_setsockopt(yxNetContext_ptr.sock, SOC_ASYNC, &val, sizeof(val)) < 0)
            {
                YxAppCloseSocket(0);
				return 2;
            }
            else
			{
				return YxAppTcpStartConnect(host,port,apn);
			}
        }
    }
    else
        YxAppCloseSocket(0);
    return 2;
}

S32 YxAppTcpSockRead(U8 *buf,S32 len)
{
    S32   ret = 0;
    if(!buf || len == 0 || len > YX_SOCK_READ_BUFFER_LEN)
        return 0;
    if(yxNetContext_ptr.socket_state != YX_NET_STATUS_CONN_CONNECTED)
        return 0; 
    if((ret = soc_recv(yxNetContext_ptr.sock, buf, len, 0)) <= 0)
    {
        if(ret == SOC_WOULDBLOCK)
            return -2;
        else if(ret == 0)
            return -1;
        else
            return -3;
    }
    return ret;//ok
}

S32 YxAppTcpSockWrite(U8 *buf, S32 len)
{
    S32 ret = 0,sendLen = YX_SOCK_BUFFER_LEN;
	if(!buf || len == 0)
        return 0;
    if(yxNetContext_ptr.socket_state != YX_NET_STATUS_CONN_CONNECTED)
        return 0;
	len -= yxAppSockData.curSendLen;
	buf += yxAppSockData.curSendLen;
	while(len>0)
	{
		if(len < YX_SOCK_BUFFER_LEN)
			sendLen = len;
		if((ret = soc_send(yxNetContext_ptr.sock, buf, sendLen, 0)) < 0)
		{
			if(ret == SOC_WOULDBLOCK)
				ret = -2;
			else
				ret = -1;
			break;
		}
		yxAppSockData.curSendLen += ret;
		buf += ret;
		len -= ret;
    }
	YxAppRestartServerTimer();
    return ret;
}

static char YxAppCloseAccount(void)
{
	kal_int8 err = CBM_OK;
	yxAppSockData.curSendLen = 0;
	yxAppSockData.sendLen = 0;
	if(yxNetContext_ptr.sock>=0)
	{
		soc_close(yxNetContext_ptr.sock);
		yxNetContext_ptr.sock = -1;
		ClearProtocolEventHandler(MSG_ID_APP_SOC_NOTIFY_IND);
		ClearProtocolEventHandler(MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND);
	//	mmi_frm_clear_protocol_event_handler(MSG_ID_APP_SOC_GET_HOST_BY_NAME_IND, 
		//	(PsIntFuncPtr)YxAppGetHostByNameIntCb);
	//	mmi_frm_clear_protocol_event_handler(MSG_ID_APP_SOC_NOTIFY_IND, 
		//	(PsIntFuncPtr)YxAppSockNotifyCb);
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("soc close\r\n",11);
#endif
	}
#if 0//don't close gprs
	if(yxNetContext_ptr.appid!=CBM_INVALID_APP_ID)
	{
		cbm_release_bearer(yxNetContext_ptr.appid);
		err = cbm_deregister_app_id(yxNetContext_ptr.appid);
		yxNetContext_ptr.appid = CBM_INVALID_APP_ID;
#if(YX_IS_TEST_VERSION!=0)
		if(err ==CBM_OK || err == CBM_WOULDBLOCK)
			YxAppTestUartSendData("appid close ok\r\n",13);
		else
			YxAppTestUartSendData("appid close err\r\n",13);
#endif
	}
#endif
	yxNetContext_ptr.socket_state = YX_NET_STATUS_CONN_STOP;
	if(err ==CBM_OK || err == CBM_WOULDBLOCK)
		return 1;
	else 
		return 0;
}

void YxAppReconnectServer(void)
{
	S8    res,*hostname = NULL;
	U16   port = YxAppGetHostNameAndPort(&hostname);
	res = YxAppTcpConnectToServer((const S8*)hostname, port,yxNetContext_ptr.apn);
#if(YX_IS_TEST_VERSION!=0)
	if(res==0)
		YxAppTestUartSendData("reconnect-error\r\n",11);
	else
		YxAppTestUartSendData("reconnect\r\n",11);
#endif
	StopTimer(YX_SOCKET_TIMER_ID);
	if(res==0)
		StartTimer(YX_SOCKET_TIMER_ID,1000,YxAppConnectToServerTimeOut);
	else
		StartTimer(YX_SOCKET_TIMER_ID,YX_CONN_SERVER_TIME_OUT,YxAppConnectToServerTimeOut);
}

void YxAppRestartServerTimer(void)
{
	StopTimer(YX_SOCKET_TIMER_ID);
	StartTimer(YX_SOCKET_TIMER_ID,YX_CONN_SERVER_TIME_OUT,YxAppConnectToServerTimeOut);
}

void YxAppCloseSocket(char reconnect)
{
	StopTimer(YX_SOCKET_TIMER_ID);
	if((reconnect==1)&&(yxAppSockData.sendCount!=0xFF))//可能是通讯中断,需要重新再来
	{
		yxAppSockData.sendCount++;
		if(yxAppSockData.sendCount<YX_GPRS_RECONNECT_COUNT)
		{
			//YxAppSendMsgToMMIMod(YX_MSG_GPRS_CTRL,YX_DATA_GPRS_RECONNECT,0);
			YxAppReconnectServer();
			return;
		}
	}
	YxAppCloseAccount();
	yxAppSockData.sendCount = 0xFF;
	YxAppUploadDataFinishCb();
}

void YxAppCloseNetwork(void)
{
	StopTimer(YX_SOCKET_TIMER_ID);
	YxAppCloseAccount();
	if(yxNetContext_ptr.appid!=CBM_INVALID_APP_ID)
	{
		cbm_release_bearer(yxNetContext_ptr.appid);
		cbm_deregister_app_id(yxNetContext_ptr.appid);
		yxNetContext_ptr.appid = CBM_INVALID_APP_ID;
	}
}

U16 YxAppGetHostNameAndPort(S8 **hostName)
{
	if(hostName)
		*hostName = yxNetContext_ptr.hostName;
	return yxNetContext_ptr.port;
}

U32 YxAppDtcntMakeDataAcctId(char simId,char *apnName,char apnType,U8 *appId)
{
	U32    acc_id = 0;
	S32    resId = YxAppGetAccountByApn(apnType,simId,appId);
	if(resId>=0)
	{
		cbm_sim_id_enum sim_no = CBM_SIM_ID_SIM1;
		if(simId==YX_APP_SIM2)//sim2
			sim_no = CBM_SIM_ID_SIM2;
		acc_id = (U32)resId;
		acc_id = cbm_encode_data_account_id(acc_id, sim_no, appId[0], KAL_FALSE);
	}
#if (YX_IS_TEST_VERSION==2)
	kal_prompt_trace(MOD_BT,"accountid:%d,appid:%d\r\n",acc_id,*appId);
#endif
	return acc_id;
}

static void YxAppConnectToServerTimeOut(void)
{
	yxNetContext_ptr.socket_state=YX_NET_STATUS_CONN_TIME_OUT;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"Ser time out\r\n",14);
#endif
	YxAppCloseSocket(1);
}

S8 YxAppConnectToMyServer(void)
{
#ifdef __USE_YX_ONLY_SIMCARD_NUMBER__
	return 0;
#else
	S8    res,*hostname = NULL;
	U16   port = 0;
	if(yxNetContext_ptr.sock != -1)
	{
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData("gprs busing\r\n",13);
#endif
		return 3;
	}
	StopTimer(YX_SOCKET_TIMER_ID);
	memset((void*)&yxAppSockData,0,sizeof(YXAPPSOCKDATAPARAM));
	port = YxAppGetHostNameAndPort(&hostname);
#if(YX_NET_WAY==MAPN_NET)
	res = YxAppTcpConnectToServer((const S8*)hostname, port, MAPN_NET);
#else
	res = YxAppTcpConnectToServer((const S8*)hostname, port, MAPN_WAP);
#endif
	if(res==1)
		StartTimer(YX_SOCKET_TIMER_ID,YX_CONN_SERVER_TIME_OUT,YxAppConnectToServerTimeOut);
#if(YX_IS_TEST_VERSION!=0)
	if(res==1)
		YxAppTestUartSendData("server ok\r\n",11);
	else
		YxAppTestUartSendData("server fail\r\n",13);
#endif
	return res;
#endif
}

/////////////////////////////////////////////////////////////////////profiles apis//////////////////////////////////////////////////////////////
char YxAppGetCurrentProfile(void)
{
	switch(srv_prof_get_activated_profile())
	{
	case SRV_PROF_PROFILE_1://MMI_PROFILE_GENERAL
		return '1';
	case SRV_PROF_PROFILE_2://MMI_PROFILE_SILENT
		return '2';
	case SRV_PROF_PROFILE_3://MMI_PROFILE_MEETING
		return '3';
	case SRV_PROF_PROFILE_4://MMI_PROFILE_OUTDOOR
		return '4';
	default:
		return '0';
	}
}

void YxAppSetCurrentProfile(char value)
{
	switch(value)
	{
	case '1':
		srv_prof_activate_profile(SRV_PROF_PROFILE_1);
		return;
	case '2':
		srv_prof_activate_profile(SRV_PROF_PROFILE_2);
		return;
	case '3':
		srv_prof_activate_profile(SRV_PROF_PROFILE_3);
		return;
	case '4':
		srv_prof_activate_profile(SRV_PROF_PROFILE_4);
		return;
	default:
		return;
	}
}
/////////////////////////////////////////////////////////////////////efence apis//////////////////////////////////////////////////////////////
#if(YX_SMS_CMD_DONT_USE==0)
static void YxAppEfenceSendSms(U8 *efenceName,U8 kind)
{
	if((efenceName!=NULL)&&(YX_CHECK_MONITOR_INFOR(yxCallParam.yxMonitors[0].number,yxCallParam.yxMonitors[0].names)==0))
	{
		U16     smsContent[YX_EFENCE_NAME_LENGTH+1+22];//2015/10/20 13:28进入围栏:
		MYTIME  oldtime;
		char    i = 0,j = 0;
		U16     *tempStr = (U16*)efenceName;
		while(i<YX_EFENCE_NAME_LENGTH)
		{
			if(tempStr[i])
				j++;
			i++;
		}
		if(j==0)
			return;
		GetDateTime(&oldtime);
		kal_wsprintf(smsContent,"%04d/%02d/%02d %02d:%02d",oldtime.nYear,oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin);
		i = 16;
		if(kind==1)//进入
		{
			smsContent[i++]=0x8FDB;
			smsContent[i++]=0x5165;
		}
		else//离开
		{
			smsContent[i++]=0x79BB;
			smsContent[i++]=0x5F00;
		}
		smsContent[i++]=0x56F4;
		smsContent[i++]=0x680F;
		smsContent[i++]=':';
		j = 0;
		while(j<YX_EFENCE_NAME_LENGTH)
		{
			if(tempStr[j])
				smsContent[i++] = tempStr[j];
			else
				break;
			j++;
		}
		smsContent[i++]=0x0000;
		YxAppSendSms(yxCallParam.yxMonitors[0].number, smsContent,1);
		return;
	}
}
#endif

void YxAppCheckEfence(double log,double lat)
{
	U8  i = 0;
	if(log==0.0||lat==0.0)
		return;
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData("efence checking\r\n",17);
#endif
	while(i<YX_EFENCE_MAX_NUM)
	{
		if((yxEfencePkg[i].names[0]!=0)&&(yxEfencePkg[i].log!=0.0)&&(yxEfencePkg[i].lat!=0.0)&&(yxEfencePkg[i].radius!=0.0))
		{
			double distance = YxAppGetGpsTwoPointDistance(yxEfencePkg[i].lat, yxEfencePkg[i].log,lat,log);
			char   kind = 0;
			if((yxEfencePkg[i].radius < distance)&&(yxEfencePkg[i].lastKind==1))//out
			{
				yxEfencePkg[i].lastKind = 0;
				kind = 2;
			}
			else if((yxEfencePkg[i].radius > distance)&&(yxEfencePkg[i].lastKind==0))//in
			{
				yxEfencePkg[i].lastKind = 1;
				kind = 3;
			}
			if((kind==2)&&(yxEfencePkg[i].kind==0||yxEfencePkg[i].kind==2))//send sms,out
			{
				YxAppLogAdd(LOG_EFENCE,'0',i);
#if(YX_SMS_CMD_DONT_USE==0)
				YxAppEfenceSendSms(yxEfencePkg[i].names,0);
#endif
				break;
			}
			else if((kind==3)&&(yxEfencePkg[i].kind==1||yxEfencePkg[i].kind==2))//send smsin
			{
				YxAppLogAdd(LOG_EFENCE,'1',i);
#if(YX_SMS_CMD_DONT_USE==0)
				YxAppEfenceSendSms(yxEfencePkg[i].names,1);
#endif
				break;
			}
		}
		i++;
	}
}

/////////////////////////////////////////////////////////////////////logs apis//////////////////////////////////////////////////////////////
void YxAppLogAdd(U8 kind,U8 action,U16 value)
{
	U8   i = 0,j = 0,k = 0xFF;
	MYTIME  oldtime;
	GetDateTime(&oldtime);
	while(i<YX_LOGS_MAX_NUM)
	{
		if(yxLogsPkg[i].kind)
			j++;
		if(k==0xFF && (kind!= LOG_EFENCE) && yxLogsPkg[i].kind==kind && yxLogsPkg[i].action==action && (yxLogsPkg[i].sent != 1))
			k = i;
		i++;
	}
	if(k==0xFF)
	{
		if(j<i)
		{
			yxLogsPkg[j].kind = kind;
			yxLogsPkg[j].date = (oldtime.nMonth<<8)|oldtime.nDay;
			yxLogsPkg[j].time = (oldtime.nHour<<8)|oldtime.nMin;
			yxLogsPkg[j].action = action;
			yxLogsPkg[j].value = value;
			yxLogsPkg[j].sent = 0;
		}
		return;
	}
	else
	{
		if(kind != LOG_EFENCE)
		{
			yxLogsPkg[k].kind = kind;
			yxLogsPkg[k].date = (oldtime.nMonth<<8)|oldtime.nDay;
			yxLogsPkg[k].time = (oldtime.nHour<<8)|oldtime.nMin;
			yxLogsPkg[k].action = action;
			yxLogsPkg[k].value = value;
			yxLogsPkg[k].sent = 0;
		}
		return;
	}
}

YXLOGPARAM *YxAppGetLogByIndex(char index)
{
	if((index<YX_LOGS_MAX_NUM)&&(yxLogsPkg[index].kind>='1')&&(yxLogsPkg[index].sent<2))
		return &yxLogsPkg[index];
	return NULL;
}

void YxAppSetLogSentStatus(void)
{
	YXLOGPARAM        *logParam = NULL;
	U8                i = 0;
	while(i<YX_LOGS_MAX_NUM)
	{
		logParam = YxAppGetLogByIndex((char)i);
		if((logParam)&&(logParam->sent==1))
		{
			logParam->kind = 0xFF;
			logParam->sent = 2;
		}
		i++;
	}
}

void YxAppClearAllLogs(void)
{
	memset((void*)yxLogsPkg,0,sizeof(YXLOGPARAM)*YX_LOGS_MAX_NUM);
}

char YxAppSystemDtIsDefault(void)
{
	MYTIME  oldtime;
	GetDateTime(&oldtime);
	if((oldtime.nYear <= 2015) && (oldtime.nMonth<7))
		return 1;
	return 0;
}
/////////////////////////////////////////////////////////////////////file apis//////////////////////////////////////////////////////////////

#define  YXAPP_SERVER_FILENAME  L"\\serverfile.dat" //保存服务器有关的参数,如域名,端口号,所选IP序号
#define  YXAPP_MONITOR_FILENAME L"\\monitorfile.dat" //保存监护者名单列表
#define  YXAPP_WHITENAME_FILENAME L"\\whitefile.dat" //白名单列表
#define  YXAPP_SYSTEMSETTTING_FILENAME L"\\sysfile.dat" //系统参数设置
#define  YXAPP_EFENCE_FILENAME L"\\efencefile.dat" //电子围栏文件
#define  YXAPP_LOG_FILENAME L"\\logsfile.dat" //日志文件
#define  YXAPP_UNSEND_FILENAME L"\\unsendfile.dat" //未上传记录包,有多条记录
#define  YXAPP_TESTPRT_FILENAME L"\\testfile.dat" //AGPS数据
#if (YX_GPS_USE_AGPS==1)
#define  YXAPP_AGPS_FILENAME L"\\agpsfile.dat" //AGPS数据
#endif

static U8 YxAppGetFolderPath(PU8 pathbuf)
{
	S16 drive;
	FS_HANDLE fh = NULL;
	drive = SRV_FMGR_PUBLIC_DRV;
    if(drive > 0)
    {
		s8 path_ascii[64];
		S8 path[64 * 2];
        memset(path_ascii,0x00,64);
        sprintf(path_ascii,"%c:\\%s",(S8)drive,YX_FILE_FOLD_NAME);
        mmi_asc_to_ucs2(path,path_ascii);
		mmi_asc_to_ucs2((CHAR*)pathbuf,path_ascii);
        fh = FS_Open((U16*)path,FS_OPEN_DIR|FS_READ_ONLY);
		if(fh >= FS_NO_ERROR)
		{
			FS_Close(fh);
			return 1;
		}
	}
	return 0;
}

static void YxAppGetServerParams(const U16 *rootfolder)
{
	U8     filename[81];
	FS_HANDLE fh = 0;
	memset((void*)filename,0x00,81);
	mmi_ucs2cpy((CHAR*)filename,(CHAR*)rootfolder);
	mmi_ucs2cat((CHAR*)filename,(CHAR*)YXAPP_SERVER_FILENAME);
	fh = FS_Open((const U16*)filename,FS_READ_ONLY);
	if(fh >= FS_NO_ERROR)
    {
		U32   new_file_len = 0,bytes_read=0;
		U8    *data_buff = NULL;
        FS_GetFileSize(fh,&new_file_len);
		if(new_file_len >= 19)
		{
			data_buff = (U8*)OslMalloc(new_file_len);
			FS_Read(fh,(void*)data_buff,new_file_len,&bytes_read);
		}
        FS_Close(fh);
		if(data_buff)
		{
			U32  i = 0;
			U8   kind = 0,j = 0;
			while(i<new_file_len)
			{
				if((data_buff[i]==0x0D)&&(data_buff[i+1]==0x0A))
				{
					if(kind==0)
						yxNetContext_ptr.hostName[j] = 0x00;
					else if(kind==1)
					{
						filename[j] = 0;
						yxNetContext_ptr.port = (U16)atoi((char*)filename);
					}
					else
					{
						filename[j] = 0;
						break;
					}
					kind++;
					j = 0;
					i += 2;
					continue;
				}
				else
				{
					if(kind==0)
					{
						if(j<YX_HOST_NAME_MAX_LEN)
							yxNetContext_ptr.hostName[j++] = (S8)data_buff[i];
					}
					else if(kind==1)
					{
						if(j<5)
							filename[j++] = data_buff[i];
					}
					else //if(kind==1)
					{
						if(j<1)
							filename[j++] = data_buff[i];
					}
				}
				i++;
			}
			OslMfree(data_buff);
		}
		return;
	}
	else
	{
		strcpy((char*)yxNetContext_ptr.hostName,(char*)YX_DOMAIN_NAME_DEFAULT);
		yxNetContext_ptr.port = YX_SERVER_PORT;
		return;
	}
}

void YxAppSetServerParams(char *host,U16 port)
{
	if((!host)||(!port)||(strlen(host)<8)||(strlen(host)>YX_HOST_NAME_MAX_LEN))
		return;//FS_Delete((kal_wchar*) file_name);
	else
	{
		FS_HANDLE fh = 0;
		U8     path_ascii[64];
		U8     path[64 * 2];
		memset(path_ascii,0x00,64);
		sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
        mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
		mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_SERVER_FILENAME);
		fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
		if(fh>=FS_NO_ERROR)
		{
			UINT   wrsize = 0;
			sprintf((char*)path_ascii,"%s\r\n",host);
			wrsize = (UINT)strlen((char*)path_ascii);
			FS_Write(fh,(void*)path_ascii,wrsize,(UINT*)&wrsize);
			sprintf((char*)path_ascii,"%d\r\n",port);
			wrsize = (UINT)strlen((char*)path_ascii);
			FS_Write(fh,(void*)path_ascii,wrsize,(UINT*)&wrsize);
			FS_Close(fh);
			//network params
			memset((void*)yxNetContext_ptr.dnsIplist.address,0, 4);
			strcpy((char*)yxNetContext_ptr.hostName,host);
			yxNetContext_ptr.port = port;
		}
		return;
	}
}

S8 *YxAppGetServerName(U16 *port)
{
	if(!port)
		return (S8*)yxNetContext_ptr.hostName; 
	*port = yxNetContext_ptr.port;
	return (S8*)yxNetContext_ptr.hostName;
}

#if (YX_GPS_USE_AGPS==1)
char YxAgpsDataWriteToIc(U8 *buffer,U16 DataLen)
{
	if(DataLen>20)
	{
#if(YX_GPS_IS_MTK_CHIP==1)
		buffer += 8;
		DataLen -= 8;
		YxAppUartTxFlush();
		YxAppUartSendData(buffer,DataLen);//write to ic
#else//ublox
		U16  sendLen = 0;
		buffer += 8;
		DataLen -= 8;
		YxAppUartTxFlush();
		while(DataLen)
		{
			if(buffer[0]==0xB5 && buffer[1]==0x62 && buffer[2]==0x0B)
			{
				sendLen = (buffer[5]<<8)|buffer[4];
				sendLen += 8;
				if(sendLen<=DataLen)
				{
					YxAppUartSendData(buffer,sendLen);//write to ic
					if(DataLen - sendLen <= 0)
						break;
					DataLen -= sendLen;
					buffer += sendLen;
					continue;
				}
				else
					break;
			}
			else
				break;
		}
#endif
		return 1;
	}
	return 0;
}
#endif

U16 YxAppGetAgpsData(kal_uint8 *data,U16 maxLen)//文件数据格式:前8个B代表月日时分,各固定占2B,后面就是AGPS数据
{
#if (YX_GPS_USE_AGPS==1)
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_AGPS_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh >= FS_NO_ERROR)
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		if((new_file_len<20)||(new_file_len>maxLen))//error
		{
			if((new_file_len>8)&&(maxLen==8))
			{
				FS_Read(fh,(void*)data,8,NULL);
				FS_Close(fh);
				return 8;
			}
			FS_Close(fh);
			return 0;
		}
		else
		{
			U32   bytes_read = 0;
			FS_Read(fh,(void*)data,new_file_len,&bytes_read);
			FS_Close(fh);
			return (U16)bytes_read;
		}
	}
#endif
	return 0;
}

void YxAppSaveAgpsData(kal_uint8 *data,U16 dataLen)//文件数据格式:前8个B代表月日时分,各固定占2B,后面就是AGPS数据
{
#if (YX_GPS_USE_AGPS==1)
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	if(data==NULL||dataLen<=20)
		return;
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_AGPS_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = dataLen;
		FS_Write(fh,(void*)data,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
#endif
}

void YxAppEfenceSaveData(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_EFENCE_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = sizeof(YXEFENCEPARAM)*YX_EFENCE_MAX_NUM;
		FS_Write(fh,(void*)yxEfencePkg,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
}

U8 YxAppGetEfenceNumber(void)
{
	U8    i = 0,j = 0;
	while(i<YX_EFENCE_MAX_NUM)
	{
		if(yxEfencePkg[i].log == 0.0 && yxEfencePkg[i].lat == 0.0 && yxEfencePkg[i].radius == 0.0)
			j++;
		i++;
	}
	return j;
}

char YxAppEfenceUpdate(kal_uint8 i,kal_uint8 *eNames,kal_uint8 kind,double lon,double lat,double radius)
{
	if((i<YX_EFENCE_MAX_NUM)&&(kind==0&&lon==0.0&&lat==0.0&&radius==0.0))
	{
		memset((void*)&yxEfencePkg[i],0,sizeof(YXEFENCEPARAM));
		return 1;
	}
	if(lon==0.0||lat==0.0||radius==0.0)
		return 0;
	if((i<YX_EFENCE_MAX_NUM) && (eNames!=NULL) && (mmi_ucs2strlen((S8*)eNames)<=YX_EFENCE_NAME_LENGTH))
	{
		if((mmi_ucs2cmp((S8*)yxEfencePkg[i].names,(S8*)eNames)!=0)||yxEfencePkg[i].kind!=kind || yxEfencePkg[i].log!=lon || yxEfencePkg[i].lat!=lat || yxEfencePkg[i].radius!=radius)
		{
			mmi_ucs2cpy((S8*)yxEfencePkg[i].names,(S8*)eNames);
			yxEfencePkg[i].kind = kind;
			yxEfencePkg[i].log = lon;
			yxEfencePkg[i].lat = lat;
			yxEfencePkg[i].radius = radius;
			return 1;
		}
	}
	return 0;
}

static void YxAppEfenceInitialData(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_EFENCE_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh >= FS_NO_ERROR)
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		if(new_file_len!=sizeof(YXEFENCEPARAM)*YX_EFENCE_MAX_NUM)//error
		{
			FS_Close(fh);
			return;
		}
		else
		{
			U32   bytes_read = 0;
			FS_Read(fh,(void*)yxEfencePkg,new_file_len,&bytes_read);
			FS_Close(fh);
			return;
		}
	}
	return;
}

YXMONITORPARAM *YxAppGetMonitorList(U8 index,S8 loadFisrt)
{
	if(loadFisrt)
	{
		FS_HANDLE fh = 0;
		U8     path_ascii[64];
		U8     path[64 * 2];
		memset(path_ascii,0x00,64);
		sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
        mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
		mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_MONITOR_FILENAME);
		fh = FS_Open((const U16*)path,FS_READ_ONLY);
		if(fh >= FS_NO_ERROR)
		{
			U32   new_file_len = 0;
			FS_GetFileSize(fh,&new_file_len);
			if(new_file_len == sizeof(YXMONITORPARAM)*YX_MAX_MONITOR_NUM)
			{
				U32  bytes_read = 0;
				FS_Read(fh,(void*)yxCallParam.yxMonitors,new_file_len,&bytes_read);
			}
			FS_Close(fh);
		}
	}
	if(index<YX_MAX_MONITOR_NUM)
		return &yxCallParam.yxMonitors[index];
	return NULL;
}

char YxAppGetMonitorNameByCallNumber(S8 *number,U8 *nameBuf)
{
	S8 i = 0;
	while(i<YX_MAX_MONITOR_NUM)
	{
		if((strlen(number)>5)&&(strlen(yxCallParam.yxMonitors[i].number)>5)&&(strstr(number,yxCallParam.yxMonitors[i].number)))
		{
			if(nameBuf)
				mmi_ucs2cpy((CHAR*)nameBuf,(CHAR*)yxCallParam.yxMonitors[i].names);
			return 1;
		}
		i++;
	}
	return 0;
}

U8 YxAppGetNumberItem(U8 index,PU8 name,PU8 usc2_number)
{
	U8       i = 0,k = 0;
	while(i<YX_MAX_MONITOR_NUM)
	{
		if(strlen(yxCallParam.yxMonitors[i].number)>5)
		{
			if(index==k)
			{
				if(name)
				{
					if(mmi_ucs2strlen((S8*)yxCallParam.yxMonitors[i].names)==0)
						kal_wsprintf((WCHAR*)name,"%s",yxCallParam.yxMonitors[i].number);
					else
						mmi_ucs2cpy((CHAR*)name,(CHAR*)yxCallParam.yxMonitors[i].names);
				}
				if(usc2_number)
					kal_wsprintf((WCHAR*)usc2_number,"%s",yxCallParam.yxMonitors[i].number);
				return 1;
			}
			k++;
		}
		i++;
	}
	i = 0;
	while(i<YX_WHITE_NUM_LIST_MAX)
	{
		if(strlen(yxFirewall.yxWhiteList[i].number)>5)
		{
			if(index==k)
			{
				if(name)
				{
					if(mmi_ucs2strlen((S8*)yxFirewall.yxWhiteList[i].names)==0)
						kal_wsprintf((WCHAR*)name,"%s",yxFirewall.yxWhiteList[i].number);
					else
						mmi_ucs2cpy((CHAR*)name,(CHAR*)yxFirewall.yxWhiteList[i].names);
				}
				if(usc2_number)
					kal_wsprintf((WCHAR*)usc2_number,"%s",yxFirewall.yxWhiteList[i].number);
				return 1;
			}
			k++;
		}
		i++;
	}
	if(index==0xFF)
		return k;
	return 0;
}

U16 YxAppSearchNumberFromList(S8 *number)
{
	S8 i = 0;
	while(i<YX_MAX_MONITOR_NUM)
	{
		if((strlen(number)>5)&&(strlen(yxCallParam.yxMonitors[i].number)>5)&&(strstr(number,yxCallParam.yxMonitors[i].number)))
			return i;
		i++;
	}
	i = 0;
	while(i<YX_WHITE_NUM_LIST_MAX)
	{
		if((strlen(number)>5)&&(strlen(yxFirewall.yxWhiteList[i].number)>5)&&(strstr(number,yxFirewall.yxWhiteList[i].number)))
			return (0x0100|i);
		i++;
	}
	return 0xFFFF;
}

S8 *YxAppGetCallNumberBuffer(void)
{
	return yxCallParam.number;
}

char YxAppSetMonitorInfor(U8 idx,S8 *number,U8 *names)
{
	if((number==NULL)&&(names==NULL)&&(idx<YX_MAX_MONITOR_NUM))
	{
		memset((void*)yxCallParam.yxMonitors[idx].number,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
		memset((void*)yxCallParam.yxMonitors[idx].names,0,(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<1);
		return 1;
	}
	if(YX_CHECK_MONITOR_INFOR(number,names))
	{
		if(number&&number[0]==0&&(idx<YX_MAX_MONITOR_NUM))
		{
			memset((void*)yxCallParam.yxMonitors[idx].number,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
			memset((void*)yxCallParam.yxMonitors[idx].names,0,(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<1);
			return 1;
		}
		return 0;
	}
	else
	{
		if(idx<YX_MAX_MONITOR_NUM)
		{
			strcpy(yxCallParam.yxMonitors[idx].number,number);
#ifdef WIN32
			if(idx==0)
			{
				yxCallParam.yxMonitors[idx].names[0] = 0x88;
				yxCallParam.yxMonitors[idx].names[1] = 0x59;
				yxCallParam.yxMonitors[idx].names[2] = 0x88;
				yxCallParam.yxMonitors[idx].names[3] = 0x59;
			}
			else
			{
				yxCallParam.yxMonitors[idx].names[0] = 0x38;
				yxCallParam.yxMonitors[idx].names[1] = 0x72;
				yxCallParam.yxMonitors[idx].names[2] = 0x38;
				yxCallParam.yxMonitors[idx].names[3] = 0x72;
			}
#else
			mmi_ucs2cpy((CHAR*)yxCallParam.yxMonitors[idx].names,(CHAR*)names);
#endif
			return 1;
		}
	}
	return 0;
}

void YxAppSaveMonitorList(void)
{
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
	extern void YxAutoTestUsbForceStop(void);
#endif
#endif
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_MONITOR_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = sizeof(YXMONITORPARAM)*YX_MAX_MONITOR_NUM;
		FS_Write(fh,(void*)yxCallParam.yxMonitors,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
	YxAutoTestUsbForceStop();
#endif
#endif
}

static void YxAppGetFirewallData(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_WHITENAME_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh >= FS_NO_ERROR)
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		if(new_file_len == sizeof(YXFIREWALLPARAM)){
			U32  bytes_read = 0;
			FS_Read(fh,(void*)&yxFirewall,new_file_len,&bytes_read);
		}
		FS_Close(fh);
	}
}

YXMONITORPARAM *YxAppGetFirewallItemBy(U8 idx)
{
	if(idx<YX_WHITE_NUM_LIST_MAX)
		return &(yxFirewall.yxWhiteList[idx]);
	return NULL;
}

char YxAppGetFirewallNameByCallNumber(S8 *number,U8 *nameBuf)
{
	S8 i = 0;
	while(i<YX_WHITE_NUM_LIST_MAX)
	{
		if((strlen(number)>5)&&(strlen(yxFirewall.yxWhiteList[i].number)>5)&&(strstr(number,yxFirewall.yxWhiteList[i].number)))
		{
			if(nameBuf)
				mmi_ucs2cpy((CHAR*)nameBuf,(CHAR*)yxFirewall.yxWhiteList[i].names);
			return 1;
		}
		i++;
	}
	return 0;
}

char YxAppSetFirewallInfor(U8 idx,S8 *number,U8 *names)
{
	if((number==NULL)&&(names==NULL)&&(idx<YX_WHITE_NUM_LIST_MAX))
	{
		memset((void*)yxFirewall.yxWhiteList[idx].number,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
		memset((void*)yxFirewall.yxWhiteList[idx].names,0,(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<1);
		return 1;
	}
	if(YX_CHECK_MONITOR_INFOR(number,names))
	{
		if((number)&&number[0]==0&&(idx<YX_WHITE_NUM_LIST_MAX))
		{
			memset((void*)yxFirewall.yxWhiteList[idx].number,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
			memset((void*)yxFirewall.yxWhiteList[idx].names,0,(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<1);
			return 1;
		}
		return 0;
	}
	else
	{
		if(idx<YX_WHITE_NUM_LIST_MAX)
		{
			strcpy(yxFirewall.yxWhiteList[idx].number,number);
			mmi_ucs2cpy((CHAR*)yxFirewall.yxWhiteList[idx].names,(CHAR*)names);
			return 1;
		}
	}
	return 0;
}

void YxAppSaveFirewallList(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	path[0] = 0x00;
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_WHITENAME_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = sizeof(YXFIREWALLPARAM);
		FS_Write(fh,(void*)&yxFirewall,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
}

U8 YxAppFirewallReadWriteStatus(U8 status)
{
	if(status>=3)
		return yxFirewall.status;
	yxFirewall.status = status;
	return status;
}

char YxAppCheckTimeIsHiddenTime(void)
{
	MYTIME  oldtime;
	U8      i = 0,shour=0,smin=0,ehour=0,emin=0;
	GetDateTime(&oldtime);
	while(i<3)
	{
		if(yxFirewall.startTime[i]==0xFFFF||yxFirewall.endTime[i]==0xFFFF)
		{
			i++;
			continue;
		}
		shour = (U8)(yxFirewall.startTime[i]>>8);
		smin = (U8)(yxFirewall.startTime[i]&0x00FF);
		ehour = (U8)(yxFirewall.endTime[i]>>8);
		emin = (U8)(yxFirewall.endTime[i]&0x00FF);
		if(shour<=ehour)
		{
			if(((oldtime.nHour>shour && oldtime.nHour < ehour)
				||(oldtime.nHour>shour && (oldtime.nHour == ehour && oldtime.nMin<=emin))
				||(oldtime.nHour==shour && oldtime.nMin >= smin
				&& ((oldtime.nHour<shour)||(oldtime.nHour==ehour
				&&oldtime.nMin<=emin)))))
			{
				return 1;
			}
		}
		else
		{
			if((oldtime.nHour>shour && oldtime.nHour>ehour)
				||(oldtime.nHour<shour && (oldtime.nHour<ehour||(oldtime.nHour==shour&&oldtime.nMin<=smin)))
				||(oldtime.nHour==shour && oldtime.nMin>=smin
				&&((oldtime.nHour<ehour)||(oldtime.nHour==ehour&&oldtime.nMin<=emin))))
			{
				return 1;
			}
		}
		i++;
	}
	return 0;
}

char YxAppFirewallReadWriteTime(char read,U16 *startTime,U16 *endTime)
{
	char  i = 0;
	if(read)
	{
		if(startTime && endTime)
		{
			while(i<3)
			{
				startTime[i] = yxFirewall.startTime[i];
				endTime[i] = yxFirewall.endTime[i];
				i++;
			}
			return 1;
		}
		if(startTime==NULL)//check
		{
			char haveItem = 0;
			while(i<3)
			{
				if((yxFirewall.startTime[i]==0xFFFF)&&(yxFirewall.endTime[i]==0xFFFF))
					haveItem++;
				i++;
			}
			if(haveItem==i)
				return 0;
		}
		return 1;
	}
	else
	{
		if(startTime && endTime)
		{
			while(i<3)
			{
				if(startTime[i]==0 && endTime[i]==0)
				{
					yxFirewall.startTime[i] = 0xFFFF;
					yxFirewall.endTime[i] = 0xFFFF;
				}
				else
				{
					yxFirewall.startTime[i] = startTime[i];
					yxFirewall.endTime[i] = endTime[i];
				}
				i++;
			}
			return 1;
		}
		return 0;
	}
}

YXSYSTEMPARAM *YxAppGetSystemParam(S8 loadFile)
{
	if(loadFile)
	{
		FS_HANDLE fh = 0;
		U8     path_ascii[64],haveRead = 0;
		U8     path[64 * 2];
		memset(path_ascii,0x00,64);
		sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
        mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
		mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_SYSTEMSETTTING_FILENAME);
		fh = FS_Open((const U16*)path,FS_READ_ONLY);
		if(fh >= FS_NO_ERROR)
		{
			U32   new_file_len = 0,bytes_read=0;
			U8    *data_buff = NULL;
			FS_GetFileSize(fh,&new_file_len);
			if(new_file_len == sizeof(YXSYSTEMPARAM))
			{
				data_buff = (U8*)&yxSystemSet;
				FS_Read(fh,(void*)data_buff,new_file_len,&bytes_read);
				if((yxSystemSet.lowBattery&0x7F)>=100)
					yxSystemSet.lowBattery = 0;
				if(yxSystemSet.gpsTimes<YX_GPS_TIME_MIN || yxSystemSet.gpsTimes>YX_GPS_TIME_MAX)
					yxSystemSet.gpsTimes = YX_GPS_UPLOAD_TIME;
				if(yxSystemSet.uploadTick<YX_HEAT_TIME_MIN || yxSystemSet.uploadTick>YX_HEAT_TIME_MAX)
					yxSystemSet.uploadTick = YX_MIN_UPLOAD_TIME;
				yxSystemSet.allowOff = 0;
				yxSystemSet.minuteTick = 0;
				yxSystemSet.systemTick = 0;
				//if(yxSystemSet.sysloadTick<YX_MIN_UPLOAD_TIME)
				//	yxSystemSet.sysloadTick = YX_MIN_UPLOAD_TIME;
				//if(strlen((char*)(yxSystemSet.lastBt))!=14)
				//	memset((void*)yxSystemSet.lastBt,0x00,15);
				haveRead = 1;
			}
			FS_Close(fh);
		}
		if(!haveRead)
		{
			yxSystemSet.lowBattery = 0;
			yxSystemSet.allowOff = 0;
			yxSystemSet.minuteTick = 0;
			yxSystemSet.systemTick = 0;
			yxSystemSet.searchBt = 0;
#if(YX_IS_TEST_VERSION!=0)
			yxSystemSet.gsensor = 1;
#else
			yxSystemSet.gsensor = YX_GSENSOR_DEFAULT_VALUE;
#endif
			yxSystemSet.heartNum = 0;
#if(YX_GSENSOR_SURPPORT!=0)
			yxSystemSet.todaySteps = 0;
			yxSystemSet.dateSteps = 0;
#endif
			yxSystemSet.gpsTimes = YX_GPS_UPLOAD_TIME;
			yxSystemSet.uploadTick = YX_MIN_UPLOAD_TIME;
			//memset((void*)yxSystemSet.lastBt,0x00,15);
			yxSystemSet.deviceFlag = YX_DEVICE_BT_ON_FLAG;
			yxSystemSet.deviceFlag |= YX_DEVICE_WIFI_ON_FLAG;
		}
	}
	return &yxSystemSet;
}

U8 YxAppGetHeartNumbers(void)
{
	return yxSystemSet.heartNum;
}

char YxAppSystemReadWriteLowBattery(char read,char value)
{
	if(read)
		return (yxSystemSet.lowBattery&0x7F);
	else
	{
		if(value!=(yxSystemSet.lowBattery&0x7F))
		{
			if(value>=100)
				value = 99;
			yxSystemSet.lowBattery = value|(yxSystemSet.lowBattery&0x80);
			YxAppSaveSystemSettings();
			return 1;
		}
	}
	return 0;
}

void YxAppSaveSystemSettings(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_SYSTEMSETTTING_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = 0;
		wrsize = sizeof(YXSYSTEMPARAM);
		FS_Write(fh,(void*)&yxSystemSet,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
}

static void YxAppReadLogData(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_LOG_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh >= FS_NO_ERROR)
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		if(new_file_len == sizeof(YXLOGPARAM)*YX_LOGS_MAX_NUM){
			U32  bytes_read = 0;
			FS_Read(fh,(void*)yxLogsPkg,new_file_len,&bytes_read);
		}
		FS_Close(fh);
	}
}

void YxAppSaveLogData(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_LOG_FILENAME);
	fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
	if(fh>=FS_NO_ERROR)
	{
		UINT   wrsize = sizeof(YXLOGPARAM)*YX_LOGS_MAX_NUM;
		FS_Write(fh,(void*)yxLogsPkg,wrsize,(UINT*)&wrsize);
		FS_Close(fh);
	}
}

void YxAppSaveUnsendTxtcmdPackage(void)
{
#ifndef __USE_YX_ONLY_SIMCARD_NUMBER__
	U16  len = YxAppBuildTxtCmdGpsUploadPackage(1,yxAppSockData.sendBuf,YX_SOCK_BUFFER_LEN);
	if(len>0)
	{
		FS_HANDLE fh = 0;
		U8     path_ascii[64];
		U8     path[64 * 2];
		memset(path_ascii,0x00,64);
		sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
		mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
		mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_UNSEND_FILENAME);
		fh = FS_Open((const U16*)path,FS_READ_WRITE);
		if(fh<FS_NO_ERROR)
		{
			fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
			if(fh<FS_NO_ERROR)
				return;
		}
		else
		{
			U32   new_file_len = 0;
			FS_GetFileSize(fh,&new_file_len);
			if(new_file_len+len>YX_UNSEND_PKG_FILE_LEN)
			{
				FS_Close(fh);
				return;
			}
		}
		FS_Seek(fh,0,FS_FILE_END);
		FS_Write(fh,(void*)yxAppSockData.sendBuf, len, NULL);
		FS_Close(fh);
		return;
	}
#endif
}

void YxAppDeleteUnsendPkgFile(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_UNSEND_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh>=FS_NO_ERROR)
	{
		FS_Close(fh);
		FS_Delete((const U16*)path);
	}
}

S32 YxAppGetUnsendDataLength(void)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_UNSEND_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh>=FS_NO_ERROR)
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		FS_Close(fh);
		return (S32)new_file_len;
	}
	return 0;
}

S32 YxAppGetUnsendDataFromFile(U8 *buffer,S32 maxLen,S32 offset)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_UNSEND_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_ONLY);
	if(fh>=FS_NO_ERROR)
	{
		U32   new_file_len = 0,bytes_read = 0;
		FS_GetFileSize(fh,&new_file_len);
		if(((U32)offset)>=new_file_len)
		{
			FS_Close(fh);
			return 0;
		}
		FS_Seek(fh,offset,FS_FILE_BEGIN);
		bytes_read = new_file_len - offset;
		if(bytes_read<4)//error
		{
			FS_Close(fh);
			return 0;
		}
#if 0
		FS_Read(fh,(void*)&bytes_read,4,NULL);
		if(bytes_read>((U32)maxLen))//error
		{
			FS_Close(fh);
			return 0;
		}
#endif
		FS_Seek(fh,offset,FS_FILE_BEGIN);
		FS_Read(fh,(void*)buffer,bytes_read,&bytes_read);
		FS_Close(fh);
		return (S32)bytes_read;
	}
	return 0;
}

static void YxAppInitFileFolder(void)
{
	U8     path[64 * 2],isok=0;
	memset((void*)path,0x00,64*2);
	isok = YxAppGetFolderPath(path);
	if(!isok)
	{
		FS_CreateDir((const kal_wchar*)path);
#if 1
		FS_SetAttributes((unsigned short*)path,FS_ATTR_DIR);
#else
		FS_SetAttributes((unsigned short*)path,FS_ATTR_DIR|FS_ATTR_HIDDEN);
#endif
	}
	YxAppGetServerParams((const kal_wchar*)path);
	YxAppGetMonitorList(0,1);
	YxAppGetFirewallData();
	YxAppGetSystemParam(1);
	YxAppEfenceInitialData();
	YxAppReadLogData();
}

void YxAppPrintToFile(U8 *buffer,U16 length)
{
	FS_HANDLE fh = 0;
	U8     path_ascii[64];
	U8     path[64 * 2];
	memset(path_ascii,0x00,64);
	sprintf((char*)path_ascii,"%c:\\%s",(S8)SRV_FMGR_PUBLIC_DRV,YX_FILE_FOLD_NAME);
	mmi_asc_to_ucs2((CHAR*)path,(CHAR*)path_ascii);
	mmi_ucs2cat((CHAR*)path,(CHAR*)YXAPP_TESTPRT_FILENAME);
	fh = FS_Open((const U16*)path,FS_READ_WRITE);
	if(fh<FS_NO_ERROR)
	{
		fh = FS_Open((const U16*)path,FS_CREATE_ALWAYS|FS_READ_WRITE);
		if(fh<FS_NO_ERROR)
			return;
	}
	else
	{
		U32   new_file_len = 0;
		FS_GetFileSize(fh,&new_file_len);
		if(new_file_len>=YX_UNSEND_PKG_FILE_LEN)
		{
			FS_Close(fh);
			return;
		}
	}
	FS_Seek(fh,0,FS_FILE_END);
	FS_Write(fh,(void*)buffer, length, NULL);
	FS_Close(fh);
}

U16 YxAppUTF8ToUincode(U8 *inbuf,U16 lenin,U8 *outbuf,U16 lenout)
{
	U16   i = 0,j = 0,tempv = 0;
	if((outbuf)&&(!lenout))
		return 0;
	while(i < lenin)
	{
		if((i+1<lenin)&&((inbuf[i] & 0xE0)==0xC0)&&((inbuf[i+1] & 0xC0)==0x80))//two bytes
		{
			if(outbuf)
			{
				if(j < lenout)
				{
					U8  tkv = (inbuf[i+1]&0x3F)<<2;
					tempv = ((inbuf[i]&0x1F)<<6)|tkv;
					outbuf[j++] = (U8)(tempv);
					outbuf[j++] = (U8)(tempv>>8);
				}
				else
					return j;//not convert finish
			}
			else
				j += 2;
			i += 1;
		}
		else if((i+2<lenin)&&((inbuf[i] & 0xF0)==0xE0)&&((inbuf[i+1] & 0xC0)==0x80)&&((inbuf[i+2] & 0xC0)==0x80))
		{
			if(outbuf)
			{
				if(j < lenout)
				{
					U8   hbyte = (inbuf[i]<<4)|((((inbuf[i+1]&0x3F)<<2)&0xF0)>>4),lbye = (((((inbuf[i+1]&0x3F)<<2)&0x0F)>>2)<<6)|(inbuf[i+2]&0x3F);
					tempv = (hbyte<<8)|lbye;
					outbuf[j++] = (U8)(tempv);
					outbuf[j++] = (U8)(tempv>>8);
				}
				else
					return j;//not convert finish
			}
			else
				j += 2;
			i += 2;
		}
		else if((inbuf[i] & 0x80)==0x00)
		{
			if(inbuf[i]==0)
				break;
			if(outbuf)
			{
				if(j < lenout)
				{
					tempv = inbuf[i]&0x7F;
					outbuf[j++] = (U8)(tempv);
					outbuf[j++] = (U8)(tempv>>8);
				}
				else
					return j;//not convert finish
			}
			else
				j += 2;
		}
		i++;
	}
	if((outbuf)&&(j+2 <= lenout))
	{
		i = j;
		outbuf[j++] = 0x00;
		outbuf[j++] = 0x00;
		return i;
	}
	return j;
}

U16 YxAppUincodeToUTF8(U8 *inbuf,U16 lenin,U8 *outbuf,U16 lenout)
{
	U16   i = 0,j = 0;
	U32   tempv = 0;
	if((outbuf)&&(!lenout))
		return 0;
	while(i < lenin)
	{
		tempv = (inbuf[i+1]<<8)|inbuf[i];
		if(tempv < 0x80)
		{
			if(tempv==0)
				return j;
			if(outbuf)
			{
				if(j < lenout)
					outbuf[j++] = tempv & 0x7F;
				else
					return j;//not convert finish
			}
			else
				j++;
		}
		else if(tempv < 0x800)
		{
			if(outbuf)
			{
				if(j + 1 < lenout)
				{
					outbuf[j++] = ((tempv>>6) & 0x1F)|0xC0;
					outbuf[j++] = (tempv & 0x3F)|0x80;
				}
				else
					return j;//not convert finish
			}
			else
				j += 2;
		}
		else if(tempv < 0x10000)
		{
			if(outbuf)
			{
				if(j + 2 < lenout)
				{
					outbuf[j++] = ((tempv>>12) & 0x0F)|0xE0;
					outbuf[j++] = ((tempv>>6) & 0x3F)|0x80;
					outbuf[j++] = (tempv & 0x3F)|0x80;
				}
				else
					return j;//not convert finish
			}
			else
				j += 3;
		}
		else if(tempv < 0x110000)
		{
			if(outbuf)
			{
				if(j + 2 < lenout)
				{
					outbuf[j++] = ((tempv>>18) & 0x07)|0xF0;
					outbuf[j++] = ((tempv>>12) & 0x3F)|0x80;
					outbuf[j++] = ((tempv>>6) & 0x3F)|0x80;
					outbuf[j++] = (tempv & 0x3F)|0x80;
				}
				else
					return j;//not convert finish
			}
			else
				j += 4;
		}
		i += 2;
	}
	return j;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void *YxAppMemMalloc(kal_int32 size)
{
	return med_alloc_ext_mem_ext(size,MED_EXT_MEMORY_TYPE_NONCACHEABLE,__FILE__,__LINE__);
}

void YxAppMemFree(void *pptr)
{
	if(pptr)
		med_free_ext_mem_ext(&pptr,__FILE__,__LINE__);
}

void YxAppGetImeiNumber(void)
{
	S16     error = 0;
	U8      imei_key_buf[10],i = 0,j = 0;
	i = 0;
	j = 0;
	ReadRecord(NVRAM_EF_IMEI_IMEISV_LID,1,(U8*)imei_key_buf,10,&error);
	while(i<15)
	{
		if(imei_key_buf[j]>0x99)
			imei_key_buf[j] = 0x00;
		yxSystemSet.imei[i++] = (imei_key_buf[j]&0x0F)+0x30;
		if(i>=15)
			break;
		yxSystemSet.imei[i++] = (imei_key_buf[j]>>4)+0x30;
		j++;
	}
	yxSystemSet.imei[15] = 0x00;
}

U8 YxAppGetSignalLevelInPercent(void)
{
	return srv_nw_info_get_signal_strength_in_percentage(MMI_SIM1);
}

S8 *YxAppGetImeiCodeString(void)
{
	return (S8*)(yxSystemSet.imei);
}

U8 YxAppBatteryLevel(void)
{
	U8    level = 0;
	switch(srv_charbat_get_battery_level())
    {
	case BATTERY_LOW_POWEROFF:
		level = 3;
		break;
	case BATTERY_LOW_TX_PROHIBIT:
	case BATTERY_LOW_WARNING:
		level = 13;
		break;
	case BATTERY_LEVEL_0:
		level = 20;
		break;

	case BATTERY_LEVEL_1:
	case BATTERY_LEVEL_2:
	case BATTERY_LEVEL_3:
		level = 33 * (srv_charbat_get_battery_level() - BATTERY_LEVEL_0);
		break;

	default:
		level = 33;
		break;
    }
	return level;//yxSystemSet.batteryLevel;
}

static void YxAppShutdownTimer(void)
{
	yxSystemSet.allowOff = 1;
	YxAppRunShutdownCmd();
}

void YxAppAutoShutdown(void)
{
	StartTimer(JDD_TIMER_00,1000,YxAppShutdownTimer);
}

void YxAppSetAllowShutdown(void)
{
	yxSystemSet.allowOff = 1;
}

char YxAppCheckAllowShutDown(void)
{
#ifndef __USE_YX_ONLY_SIMCARD_NUMBER__
	if(srv_nw_usab_is_usable(MMI_SIM1))
	{
		if(yxSystemSet.allowOff==1)
			return 1;
#if(YX_IS_TEST_VERSION!=0)
		return 1;
#else
		return 0;
#endif
	}
#endif
	return 1;
}

static void YxAppScPlayToneTimer(void)
{
	//TONE_SetOutputVolume(255, 0);
	srv_prof_play_tone_with_id(SRV_PROF_TONE_FM, SRV_PROF_AUD_TONE3, SRV_PROF_RING_TYPE_REPEAT, NULL);
}

void YxAppSearchWtPlayTone(void)
{
	StopTimer(JDD_TIMER_01);
	StartTimer(JDD_TIMER_01,1000,YxAppScPlayToneTimer);
	yxPlayToneTick = YxAppGetSystemTick(1);
}

void YxAppSetTimeFormat(char kind)
{
	if(kind=='1')//24
		srv_setting_set_time_format(SETTING_TIME_FORMAT_24_HOURS);
	else
		srv_setting_set_time_format(SETTING_TIME_FORMAT_12_HOURS);
}

void YxAppSetBacklightTime(char kind)
{
	S8   bl_sec = 5;
	switch(kind)
	{
	case '1':
		bl_sec = 15;
		break;
	case '2':
		bl_sec = 30;
		break;
	case '3':
		bl_sec = 45;
		break;
	case '4':
		bl_sec = 60;
		break;
	}
	srv_gpio_setting_set_bl_time(bl_sec);
}

static void YxAppImeiKeyTimerCb(void)
{
	yxTimerCountImei = 0;
}

void YxAppImeiKeyPress(void)
{
	yxTimerCountImei++;
	if(yxTimerCountImei>=2)
	{
		yxTimerCountImei = 0;
		SSCHandleIMEI();
		return;
	}
	StopTimer(VOIP_POPUP_TIMER);
	StartTimer(VOIP_POPUP_TIMER,1000,YxAppImeiKeyTimerCb);
}

void YxAppCheckLowBatteryShreshold(U8 percent)
{
#if(YX_IS_TEST_VERSION!=0)
	char   logBuf[21];
	sprintf(logBuf,"bat percent:%d\r\n",percent);
	YxAppTestUartSendData((U8*)logBuf,strlen(logBuf));
#endif
	if((yxSystemSet.lowBattery&0x80)&&(percent>(yxSystemSet.lowBattery&0x7F)))
	{
		yxSystemSet.lowBattery &= 0x7F;
		YxAppSaveSystemSettings();
		return;
	}
	if(((yxSystemSet.lowBattery&0x7F)==0)||(yxSystemSet.lowBattery&0x80)||(srv_nw_usab_is_usable(MMI_SIM1)==MMI_FALSE)||(YX_CHECK_MONITOR_INFOR(yxCallParam.yxMonitors[0].number,yxCallParam.yxMonitors[0].names)==1))
		return;
	else
	{
		if(percent <= (yxSystemSet.lowBattery&0x7F))
		{
#if(YX_NO_SIMCARD_GET_DATA==1)
			percent = yxSystemSet.lowBattery&0x7F;
			yxSystemSet.lowBattery |= 0x80;
			YxAppSaveSystemSettings();
			YxAppLogAdd(LOG_LOWBAT,'0',percent);
#else
			U16     smsContent[24+5+1];//2015/10/20 13:28宝贝的电量低于90%
			char    i = 16;
			MYTIME  oldtime;
			percent = yxSystemSet.lowBattery&0x7F;
			yxSystemSet.lowBattery |= 0x80;
			YxAppSaveSystemSettings();
			GetDateTime(&oldtime);
			kal_wsprintf(smsContent,"%04d/%02d/%02d %02d:%02d",oldtime.nYear,oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin);
			smsContent[i++] = 0x5B9D;
			smsContent[i++] = 0x8D1D;
			smsContent[i++] = 0x7684;
			smsContent[i++] = 0x7535;
			smsContent[i++] = 0x91CF;
			smsContent[i++] = 0x4F4E;
			smsContent[i++] = 0x4E8E;
			smsContent[i++] = (percent/10)+'0';
			smsContent[i++] = (percent%10)+'0';
			smsContent[i++] = '%';
			YxAppLogAdd(LOG_LOWBAT,'0',percent);
			YxAppSendSms(yxCallParam.yxMonitors[0].number,smsContent,1);
#endif
		}
		return;
	}
}

void YxAppGetSystemDateTimeStr(char *strBuf)
{
	applib_time_struct   dt;
	applib_dt_get_date_time(&dt);
	sprintf(strBuf,"%04d/%02d/%02d %02d:%02d:%02d",dt.nYear,dt.nMonth,dt.nDay,dt.nHour,dt.nMin,dt.nSec);
}

void YxAppStartAllPsDevices(void)
{
	U16   flag = YxAppGetRunFlag(0);
	if(flag & YX_RUNKIND_LBS)
#ifdef WIN32
		YxAppStepRunMain(YX_RUNKIND_LBS);
#else
		YxAppCellStartReq();
#endif
#if defined(__MMI_WLAN_FEATURES__)
	if(flag & YX_RUNKIND_WIFI)
	{
		if(YxAppWifiOpenAndScan()==0)
			YxAppStepRunMain(YX_RUNKIND_WIFI);
#if(YX_IS_TEST_VERSION!=0)
		else
			YxAppTestUartSendData("WIFIOPEN2-OK\r\n",13);
#endif
	}
#else
#ifdef YXAPP_WIFI_UART
	if(flag & YX_RUNKIND_WIFI)
		YxAppWifiOpen();
#endif
#ifdef __MMI_BT_SUPPORT__
	if(flag & YX_RUNKIND_BT_SEARCH)
#ifdef WIN32
		YxAppStepRunMain(YX_RUNKIND_BT_SEARCH);
#else
		YxAppBtSearchDev();
#endif
#endif
#endif
#ifndef YXAPP_WIFI_UART
	if(flag & YX_RUNKIND_GPS)
		YxAppGpsControl(1);
#endif
}

void YxAppInitionBasicApp(void)
{
	U8  i = 0;
#ifdef __MMI_BT_SUPPORT__
	yxAppBtSrvhd = 0;
	yxBtInSearching = 0;
	memset((void*)&yxbtInfor,0,sizeof(YXBTPARAM)*YXAPP_BT_MAX_SEARCH_DEVICES);
	memset((void*)yxLocalBtName,0,SRV_BT_CM_BD_FNAME_LEN+1);
	memset((void*)yxLocalBtMac,0,YX_BT_MAC_BUFFER_LEN);
#endif
#ifdef __MMI_WLAN_FEATURES__
	memset((void*)&wlanParams,0,sizeof(WLANMACPARAM)*WNDRV_MAX_SCAN_RESULTS_NUM);
	memset((void*)yxLocalWlanMac,0,WNDRV_MAC_ADDRESS_LEN);
#endif
#ifdef YXAPP_WIFI_UART
	memset((void*)&wifiParam,0,sizeof(YXWIFIPARAM));
#endif
#ifdef __NBR_CELL_INFO__
	memset((void*)&yxCellParam,0,sizeof(YXAPPCELLPARAM));
#endif
	memset((void*)&yxSmsParam,0,sizeof(YXAPPSMSPARAM));
	memset((void*)&yxNetContext_ptr,0,sizeof(YXAPPSOCKCONTEXT));
	yxNetContext_ptr.sock = -1;
	yxNetContext_ptr.appid = CBM_INVALID_APP_ID;
	memset((void*)&yxSystemSet,0,sizeof(YXSYSTEMPARAM));
	memset((void*)&yxCallParam,0,sizeof(YXCALLPARAM));
	memset((void*)&yxFirewall,0,sizeof(YXFIREWALLPARAM));
	for(i=0;i<3;i++)
	{
		yxFirewall.startTime[i] = 0xFFFF;
		yxFirewall.endTime[i] = 0xFFFF;
	}
	memset((void*)yxEfencePkg,0,sizeof(YXEFENCEPARAM)*YX_EFENCE_MAX_NUM);
	memset((void*)yxLogsPkg,0,sizeof(YXLOGPARAM)*YX_LOGS_MAX_NUM);
	YxAppInitFileFolder();
	YxAppGpsInition(
#if(YXAPP_GPS_KIND==3)
		YX_GPS_KIND_GN,
#elif (YXAPP_GPS_KIND==2)
		YX_GPS_KIND_GL,
#elif (YXAPP_GPS_KIND==1)
		YX_GPS_KIND_BD,
#else
		YX_GPS_KIND_GP,
#endif
		1);//此处1请不要修改
	YxAppInitionParam();
#if(YX_GSENSOR_SURPPORT!=0)
	memset((void*)&yxGsensorParam,0,sizeof(YXGSENSORPARAM));
	yxGsensorParam.timerCount = 0xFF;
	yxGsensorTimerFlag = 0;
#endif
#ifdef WIN32
	{
		YxAppSetMonitorInfor(0,"17877777777",(U8*)L"LDE");
		YxAppSetMonitorInfor(1,"14454545454",(U8*)L"LKK");
		YxAppSaveMonitorList();
		//YxAppSetMonitorInfor(2,"14524353172",(U8*)L"KLDJ");
		//YxAppSetMonitorInfor(3,"12224353172",(U8*)L"WDS");
		//YxAppSetMonitorInfor(4,"18824353172",(U8*)L"GHJK");
		//YxAppSetMonitorInfor(5,"19924353172",(U8*)L"JLJLK");
	}
#endif
}
#endif