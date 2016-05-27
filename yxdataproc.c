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
#include "DateTimeGprot.h"
#include "gui.h"
#include "TimerEvents.h"
#include "app_datetime.h"
#include "YxBasicApp.h"
////////////////////////////////////////////////////////////Global define///////////////////////////////////////////////////////////////////
#define GetDateTime(t) applib_dt_get_date_time((applib_time_struct *)t)
#define   YXAPP_SMS_COMMAND_NUM        3
#define   PROTOCOL_VER      '1'
#define   YXAPP_WATCH_VERSION  "1.3"      //soft version number
#define   CUSTOMER_ID       0x0001        //user id,server give you

//cmwap http协议样本
//a是要打印数据的BUFFER,b:服务器文件路径,c:数据包nContent-Length:长度,d:数据包内容,端口只能访问80,h:主机域名
#define MAKE_HTTP_WAP_HEAD_POST(a,b,h,c,d)  sprintf(a, "POST %s HTTP/1.1\r\nHost: 10.0.0.172\r\nUser-Agent: MAUI_WAP_Browser\r\nX-Online-Host:%s\r\nAccept: */*\r\nAccept-Charset: utf-8, utf-16, iso-8859-1, iso-10646-ucs-2, GB2312, windows-1252, us-ascii\r\nAccept-Language: zh-tw, zh-cn, en\r\nContent-Type:plication/x-www-form-urlencoded; charset=utf-8\r\nContent-Length: %d\r\n\%s\r\n\r\n",b,h,c,d);

//此为获取YX_DOMAIN_NAME_DEFAULT网页内容,a为BUFFER,B为访问主机的目录文件,h:主机域名
#define MAKE_HTTP_WAP_HEAD_GET(a,b,h)  sprintf(a, "GET %s HTTP/1.1\r\nHost: 10.0.0.172\r\nUser-Agent: MAUI_WAP_Browser\r\nX-Online-Host: %s\r\nAccept: */*\r\nAccept-Charset: utf-8, utf-16, iso-8859-1, iso-10646-ucs-2, GB2312, windows-1252, us-ascii\r\nAccept-Language: zh-tw, zh-cn, en\r\nConnection: Keep-Alive\r\n\r\n\r\n",b,h);

//cmnet http协议样本,YX_DOMAIN_NAME_DEFAULT网页内容,a为BUFFER,B为访问主机的目录文件,h:主机域名
#define MAKE_HTTP_NET_HEAD_GET(a,b,h)  sprintf(a, "GET %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: MAUI_WAP_Browser\r\nAccept: */*\r\nAccept-Charset: utf-8, utf-16, iso-8859-1, iso-10646-ucs-2, GB2312, windows-1252, us-ascii\r\nAccept-Language: zh-tw, zh-cn, en\r\nConnection: Keep-Alive\r\n\r\n\r\n",b,h);

//a是要打印数据的BUFFER,b:服务器文件路径,c:数据包nContent-Length:长度,d:数据包内容,端口只能访问80,h:主机域名
#define MAKE_HTTP_NET_HEAD_POST(a,b,h,c,d)  sprintf(a, "POST %s HTTP/1.1\r\nHost: %s\r\nUser-Agent: MAUI_WAP_Browser\r\nAccept: */*\r\nAccept-Charset: utf-8, utf-16, iso-8859-1, iso-10646-ucs-2, GB2312, windows-1252, us-ascii\r\nAccept-Language: zh-tw, zh-cn, en\r\nContent-Type:plication/x-www-form-urlencoded; charset=utf-8\r\nContent-Length: %d\r\n\%s\r\n\r\n",b,h,c,d);

#define MG_ADD_KEYDATA_STRING(a,k,d) sprintf(a,"%s+%s=%s\r\n\r\n",a,k,d);
#define MG_ADD_KEYDATA_VALUE(a,k,d) sprintf(a,"%s+%s=%d\r\n\r\n",a,k,d);

#define MG_ADD_KEYDATA_HEAD(a,k)   sprintf(a,"%s+%s=",a,k);
#define MG_ADD_KEYDATA_DATA(a,d)   strcat(a,d);
#define MG_ADD_KEYDATA_TAIL(a)     strcat(a,"\r\n\r\n");
////////////////////////////////////////////////////////////Global vars///////////////////////////////////////////////////////////////////
//采用IK通讯协议
static YXPROTOCOLPARAM yxProtocolPkg;
//本协议总体格式:长度(4B)+版本(1B)+数据种类(1B,'1'为文本,'2'为语音,'3'为图片,'4'为AGPS星历数据,'5'为过去包数据)+ID(5B)+IMEI(15B)+DATA(文本指令或语音数据或图片数据)+CRC(2B)
//文本指令采用"+CMD=\r\n"格式,文字编码采用UTF8格式,结束符为:&#,
////////////////////////////////////////////////////////////SMS data proc///////////////////////////////////////////////////////////////////
static void YxAppSmsSendGetMoney(void)
{
	char *numberk = "10086";
	U16  content[5];
	if(YxAppGetSimOperator(YX_APP_SIM1)==MSIM_OPR_UNICOM)
	{
		content[0] = 'C';
		content[1] = 'X';
		content[2] = 'H';
		content[3] = 'F';
		content[4] = 0;
		numberk = "10010";
	}
	else
	{
		content[0] = 'Y';
		content[1] = 'E';
		content[2] = 0;
		content[3] = 0;
		content[4] = 0;
	}
	YxAppSendSms(numberk, content,1);
}

static void YxAppSmsSetYeBufferContent(U8 *smsContent,U16 length)
{
	if((yxProtocolPkg.isYe==1) && (length>0))
		yxProtocolPkg.yeLength = YxAppUincodeToUTF8(smsContent,length,yxProtocolPkg.yeSms,YX_APP_YES_SMS_LENGTH);
}

//返回值说明:0:不拦截此短信,1:拦截此短信,本样列以儿童机为例,所有短信都拦截,都认为是命令,即都返回1
char YxAppCustomerSmsContentFilter(U8* number,U8 num_len,U8* content,U16 content_len,char encoding,MYTIME *timeStamp)
{
#if(YX_IS_TEST_VERSION!=0)
	U8     bufft[40];
#endif
#if(YX_IS_TEST_VERSION!=0)
	sprintf((char*)bufft,"New sms:%d,%d,%d,%d:%d:%d:NUM:%d\r\n",content[0],content[1],content[2],content[3],content_len,encoding,num_len);
	YxAppTestUartSendData(bufft,strlen((char*)bufft));
#endif
	YxAppSmsCheckDateTime(timeStamp);
	if((encoding!=0 && encoding!=8)||(num_len==0)||(number==NULL))//only proc english,gsm ascii
		return 1;
	else if(encoding==8)//SRV_SMS_DCS_UCS2
	{
		if((strstr((char*)number,"10086")!=NULL)||(strstr((char*)number,"10010")!=NULL))
		{
#if 1
			U16  i = 0;
			char isYeSms = 0;
			while(i<content_len)
			{
				if((i+4<content_len)&&(content[i]==0x59&&content[i+1]==0x4F&&content[i+2]==0x9D&&content[i+3]==0x98))//余额
				{
					isYeSms = 1;
					break;
				}
				i += 2;
			}
			if(isYeSms)
				YxAppSmsSetYeBufferContent(content,content_len);
#else
			char startN = 0,j = 0,number[12];
			U16  i = 0;
			while(i<content_len)
			{
				if(startN==1)
				{
					if((j<12)&&((content[i]>='0' && content[i]<= '9')||content[i]=='.'))
						number[j++] = (char)content[i];
					else
						break;
				}
				if((i+4<content_len)&&(content[i]==0x59&&content[i+1]==0x4F&&content[i+2]==0x9D&&content[i+3]==0x98))//余额
				{
					startN = 1;
					i += 2;
				}
				i += 2;
			}
			number[j] = 0;
			if(j>1)
			{
				U16  content[21];
				j = 0;
				startN = 0;
				i = (U16)(atof(number)*100);
			//	YxAppLogAdd(LOG_SMS,'1',i);
				content[j++] = 0x5F53;
				content[j++] = 0x524D;
				content[j++] = 0x4F59;
				content[j++] = 0x989D;
				while(j<19)
				{
					if(startN<strlen(number))
						content[j++] = number[startN++];
					else
						break;
				}
				content[j++] = 0x5143;
				content[j++] = 0;
				YxAppSendSms(NULL, content,0);
			}
#endif
		}
		return 1;
	}
	else
	{
		U16    i = 0,k = 0;
		char   oki = 1,j=0;
		const char *smskeyword[YXAPP_SMS_COMMAND_NUM]=//only examples,format:#host#
		{
			"host",
			"cxye",
			"yxjt"
		};
		j = 0;
		while(j < YXAPP_SMS_COMMAND_NUM)
		{
			i = 0;
			k = 0;
			oki = 1;
			if(content[i] != '#')
				return 1;
			i += 2;
			while((smskeyword[j][k])&&(i < content_len))
			{
				if(content[i] != smskeyword[j][k])
				{
					oki = 0;
					if(content[i]=='#')
						i += 2;
					break;
				}
				i += 2;
				k++;
			}
			if((smskeyword[j][k]==0x00)&&(oki)&&(content[i]=='#'))
			{
				i += 2;
				break;
			}
			j++;
		}
		if(j >= YXAPP_SMS_COMMAND_NUM)
			return 1;
		switch(j)//commands proc
		{
		case 0://format:#host#=www.abc.com;8080;
			{
				char   hostName[YX_HOST_NAME_MAX_LEN+1],num[6];
				U16    port = 0;
				memset((void*)num,0,6);
			//	i += 12;
				j = 0;
				oki = 0;
				while(i<content_len)
				{
					if(content[i] == '=')
						oki=1;
					else if(content[i] == ',')
					{
						if(oki==1)//host name
							hostName[j] = 0x00;
						else if(oki==2)//port
						{
							num[j] = 0;
							break;
						}
						j = 0;
						oki++;
					}
					else
					{
						if(oki==1)//host name
						{
							if(j<YX_HOST_NAME_MAX_LEN)
								hostName[j++] = (char)content[i];
						}
						else if(oki==2)//port
						{
							if(j<5)
								num[j++] = (char)content[i];
						}
					}
					i += 2;
				}
				port = atoi(num);
				YxAppSetServerParams(hostName,port);//save host name and port
			}
			break;
		case 1://#cxye#查询余额
			{
				char *numberk = "10086";
				U16  content[5];
				YxAppSetSmsNumber((S8*)number,num_len);
				if(YxAppGetSimOperator(YX_APP_SIM1)==MSIM_OPR_UNICOM)
				{
					content[0] = 'C';
					content[1] = 'X';
					content[2] = 'H';
					content[3] = 'F';
					numberk = "10010";
				}
				else
				{
					content[0] = 'C';
					content[1] = 'X';
					content[2] = 'Y';
					content[3] = 'E';
				}
				content[4] = 0;
				YxAppSendSms(numberk, content,1);
			}
			break;
		case 2://#yxjt#语音监听
			{
				if(YxAppMakeWithCallType(YX_CALL_LISTEN,(S8*)number)==0)
				{//通话中，请稍候再监听
					const U16  content[11]={0x901A,0x8BDD,0x4E2D,0xFF0C,0x8BF7,0x7A0D,0x5019,0x518D,0x76D1,0x542C,0x0000};
					YxAppSendSms((S8*)number, (U16*)content,1);
				}
			}
			break;
		}
	}
	return 1;//is command sms,don't display
}

////////////////////////////////////////////////////////////Call call cb///////////////////////////////////////////////////////////////////

kal_bool YxAppCallNumberIsAllowed(char callType,S8 *number)//callType:1:来电.号码为ASCII码,返回TRUE:允许,FASE:禁止
{
	YXMONITORPARAM    *numList = NULL;
	U8                index = 0,i = 0;
#if(YX_IS_TEST_VERSION!=0)
	char              logBuf[51];
	sprintf(logBuf,"calls:%s,t:%d\r\n",number,callType);
	YxAppTestUartSendData((U8*)logBuf,strlen(logBuf));
#endif
	if(callType==1)
	{
		if(number && (strlen(number)<3))//no incomming call number
		{
			if(YxAppFirewallReadWriteStatus(3)!=0)
				return KAL_FALSE;
			else
				return KAL_TRUE;
		}
		if(YxAppCheckTimeIsHiddenTime()==1)
			return KAL_FALSE;
	}
	while(index<YX_MAX_MONITOR_NUM)
	{
		numList = YxAppGetMonitorList(index,0);
		if((numList)&&(strlen(numList->number)>0)&&(strstr(number,numList->number)))
			return KAL_TRUE;
		if((numList)&&(numList->number[0]==0))
			i++;
		index++;
	}
	if(callType==1)
	{
		index = 0;
		i = 0;
		while(index<YX_WHITE_NUM_LIST_MAX)
		{
			numList = YxAppGetFirewallItemBy(index);
			if((numList)&&(strlen(numList->number)>0)&&(strstr(number,numList->number)))
				return KAL_TRUE;
			if((numList)&&(numList->number[0]==0))
				i++;
			index++;
		}
	}
	if(i==index)
		return KAL_TRUE;
	return KAL_FALSE;
}

void YxAppEndCallCb(void)//when call is released,it calls this cb
{
	YxAppCallEndProc();
}

////////////////////////////////////////////////////////////gprs data proc///////////////////////////////////////////////////////////////////

static void YxProtocolFreeRes(void)
{
	yxProtocolPkg.totalLen = 0;
	yxProtocolPkg.sendOk = 0;
	yxProtocolPkg.dataLen = 0;
}

static kal_uint16 YxProtocolCheckSum(kal_uint8 *buf,kal_uint16 len)
{
#define   MOD_ADLER   55171
	kal_uint32     a = 7,b = 0;
	kal_uint16     i = 0,checksum = 0;
	len -= 2;
	while(i<len)
	{
		a = (a + buf[i]+13) % MOD_ADLER;
		b = (b + a + 3) % MOD_ADLER;
		i++;
	}
	a = (b<<16) | a;
	checksum = (kal_uint16)(a & 0x0000FFFF);
	if(checksum==0xFFFF)
		checksum = (kal_uint16)(a >> 16);
	return checksum;
}

U16 YxAppBuildTxtCmdGpsUploadPackage(char savepkg,U8 *sendBuf,U16 maxLen) //定位包
{
	YXSYSTEMPARAM     *setParam = YxAppGetSystemParam(0);
	YXLOGPARAM        *logParam = NULL;
	MYTIME            oldtime;
#if (YX_GPS_USE_AGPS==1)
	U8                agpsTimeBuf[9];
#endif
	S8   *tempStr = (S8*)(sendBuf+4);
#ifdef __MMI_WLAN_FEATURES__
	WLANMACPARAM      *wlanP = NULL;
#elif defined(YXAPP_WIFI_UART)
	YXWIFIAPPARAM     *apInfor = NULL;
#endif
	yxapp_cell_info_struct *cellIfr = NULL;
	U16  i = 0,checkSum = 0,j = 0;
	GetDateTime(&oldtime);
	if(savepkg==1)
	{
		if(yxProtocolPkg.sendOk==1)
			return  0;
	}
	oldtime.nYear -= 2000;
	sprintf(tempStr,"%c1%05d%s+IKPS=DT:%02d%02d%02d%02d%02d,SOS:%d\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei,oldtime.nYear,oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin,yxProtocolPkg.isSos);
	j = (U16)strlen(tempStr);
	tempStr += j;
	//lbs
	strcpy(tempStr,"+IKLBS=");
	j = (U16)strlen(tempStr);
	tempStr += j;
	i = 0;
#if 0//def WIN32
	sprintf(tempStr,"MCC:%d,MNC:%d,LAC:%d,CELLID:%d,RF:%d|",460,1,27374,78998,-87);
	j = (U16)strlen(tempStr);
	tempStr += j;
	sprintf(tempStr,"MCC:%d,MNC:%d,LAC:%d,CELLID:%d,RF:%d\r\n",460,0,27374,78998,-87);
	j = (U16)strlen(tempStr);
	tempStr += j;
#else
	while(i<YX_APP_MAX_CELL_INFOR_NUM)
	{
		cellIfr = YxAppLbsGetData((char)i);
		if(cellIfr)
		{
			if(YxAppLbsGetData((char)(i+1)))
				sprintf(tempStr,"MCC:%d,MNC:%d,LAC:%d,CELLID:%d,RF:%d|",cellIfr->mcc,cellIfr->mnc,cellIfr->lac,cellIfr->ci,-1*cellIfr->rxlev);
			else
				sprintf(tempStr,"MCC:%d,MNC:%d,LAC:%d,CELLID:%d,RF:%d",cellIfr->mcc,cellIfr->mnc,cellIfr->lac,cellIfr->ci,-1*cellIfr->rxlev);
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		else
			break;
		i++;
	}
#endif
	strcpy(tempStr,"\r\n");
	tempStr += 2;
	//gps
	if(GpsGetLongitudeV()==0.0||GpsGetLatitudeV()==0.0)
		strcpy(tempStr,"+IKGPS=\r\n");
	else
		sprintf(tempStr,"+IKGPS=LON:%.6f,LAT:%.6f,SPD:%.6f,ANG:%.4f\r\n",GpsGetLongitudeV(),GpsGetLatitudeV(),GpsGetSpeedV(),GpsGetAngleV());
	i = (U16)strlen(tempStr);
	tempStr += i;
	//wifi
#if defined(__MMI_WLAN_FEATURES__)||defined(YXAPP_WIFI_UART)
	i = 0;
	strcpy(tempStr,"+IKWIFI=");
	j = (U16)strlen(tempStr);
	tempStr += j;
#if 0//def WIN32
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK",0xeF,0xAB,0xAC,0x12,0x87,0x98,-90);
	i = (U16)strlen(tempStr);
	tempStr += i;
#elif defined(YXAPP_WIFI_UART)
	while(i<YXAPP_WIFI_AP_NUMBER)
	{
		apInfor = YxAppGetWlanApListInfor(i++);
		if(apInfor)
		{
			if(YxAppGetWlanApListInfor((U8)i)==NULL)
				sprintf(tempStr,"%s,%s,%s",apInfor->macString,apInfor->rfStr,apInfor->apName);
			else
				sprintf(tempStr,"%s,%s,%s|",apInfor->macString,apInfor->rfStr,apInfor->apName);
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
	}
#else
	while(i<8)
	{
		wlanP = YxAppGetWlanScanInfor((U8)(i++));
		if(wlanP)
		{
			if(YxAppGetWlanScanInfor((U8)i)==NULL)
				sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK",wlanP->bssid[0],wlanP->bssid[1],wlanP->bssid[2],wlanP->bssid[3],wlanP->bssid[4],wlanP->bssid[5],wlanP->rssi);
			else
				sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,TP-LINK|",wlanP->bssid[0],wlanP->bssid[1],wlanP->bssid[2],wlanP->bssid[3],wlanP->bssid[4],wlanP->bssid[5],wlanP->rssi);
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		else
			break;
	}
#endif
	strcpy(tempStr,"\r\n");
	tempStr += 2;
#endif
	//bt
#ifdef __MMI_BT_SUPPORT__
	i = 0;
	strcpy(tempStr,"+IKBT=");
	j = (U16)strlen(tempStr);
	tempStr += j;
#if 0//def WIN32
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,BT|",0xeF,0xAB,0xAC,0x12,0x87,0x98,-10);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,BT|",0xeC,0xAB,0xAC,0x12,0x87,0x98,-10);
	i = (U16)strlen(tempStr);
	tempStr += i;
	sprintf(tempStr,"%02x:%02x:%02x:%02x:%02x:%02x,%d,BT",0xeD,0xAB,0xAC,0x12,0x87,0x98,-10);
	i = (U16)strlen(tempStr);
	tempStr += i;
#else
	while(i<YXAPP_BT_MAX_SEARCH_DEVICES)
	{
		if(YxAppBtGetDevicesAddress((U8)(i++),tempStr)==1)
		{
			if(YxAppBtGetDevicesAddress((U8)i,NULL)==1)
				strcat(tempStr,"|");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		else
			break;
	}
#endif
	strcpy(tempStr,"\r\n");
	tempStr += 2;
#endif
	//信号,电量百分比(0-100),情景模式('1':一般,'2':静音,'3':会议,'4':户外)
	oldtime.nYear -= 2000;
	i = YxAppGetSignalLevelInPercent();
	sprintf(tempStr,"+IKSYS=RF:%02d,BAT:%02d,PRO:%c,GPRS:%05d,GPS:%05d,GSEN:%d,DT:%02d%02d%02d%02d%02d\r\n",i,YxAppBatteryLevel(),YxAppGetCurrentProfile(),setParam->uploadTick,setParam->gpsTimes,setParam->gsensor,oldtime.nYear,oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin);
	j = (U16)strlen(tempStr);
	tempStr += j;
	//check log first
	i = 0;
	j = 0;
	while(i<YX_LOGS_MAX_NUM)
	{
		logParam = YxAppGetLogByIndex((char)i);
		if((logParam)&&(logParam->kind==LOG_EFENCE))
		{
			j = 1;
			break;
		}
		i++;
	}
	if(j==1)
	{
		i = 0;
		strcpy(tempStr,"+IKLOG=");//log
		j = (U16)strlen(tempStr);
		tempStr += j;
		while(i<YX_LOGS_MAX_NUM)
		{
			logParam = YxAppGetLogByIndex((char)i);
			if((logParam)&&(logParam->kind==LOG_EFENCE))
			{
				if(YxAppGetLogByIndex((char)(i+1))==NULL)
					sprintf(tempStr,"K:%c,DT:%02d%02d%02d%02d,ACT:%c,V:%04x",logParam->kind,(logParam->date>>8),logParam->date&0x00FF,logParam->time>>8,logParam->time&0x00FF,logParam->action,logParam->value);
				else
					sprintf(tempStr,"K:%c,DT:%02d%02d%02d%02d,ACT:%c,V:%04x|",logParam->kind,(logParam->date>>8),logParam->date&0x00FF,logParam->time>>8,logParam->time&0x00FF,logParam->action,logParam->value);
				logParam->sent = 1;
				j = (U16)strlen(tempStr);
				tempStr += j;
			}
			i++;
		}
		strcpy(tempStr,"\r\n");
		tempStr += 2;
	}
#if(YX_GSENSOR_SURPPORT!=0)
	//pedo
	strcpy(tempStr,"+IKPED=DT:");
	j = (U16)strlen(tempStr);
	tempStr += j;
	sprintf(tempStr,"%02d%02d%02d%02d,N:%d\r\n",oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin,YxAppGsensorGetStep(0));
	j = (U16)strlen(tempStr);
	tempStr += j;
	//sleep
	if(oldtime.nHour>=21 || oldtime.nHour<7)
	{
		strcpy(tempStr,"+IKSLP=DT:");
		j = (U16)strlen(tempStr);
		tempStr += j;
		sprintf(tempStr,"%02d%02d%02d%02d,N:%d\r\n",oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin,YxAppGsensorGetStep(1));
		j = (U16)strlen(tempStr);
		tempStr += j;
	}
#endif
#if (YX_GPS_USE_AGPS==1)
	//agps
	strcpy(tempStr,"+IKAGPS=GET:");
	j = (U16)strlen(tempStr);
	tempStr += j;
	if(YxAgpsGetDataTime(agpsTimeBuf)==1)
	{
		agpsTimeBuf[8] = 0;
		strcpy(tempStr,(S8*)agpsTimeBuf);
		j = (U16)strlen(tempStr);
		tempStr += j;
	}
	strcpy(tempStr,"\r\n");
	tempStr += 2;
#endif
	//end
	i = (U16)(strlen((S8*)(sendBuf+4))+6);
	sendBuf[0] = (U8)(i&0x00FF);
	sendBuf[1] = (U8)(i>>8);
	sendBuf[2] = 0;
	sendBuf[3] = 0;
	checkSum = YxProtocolCheckSum(sendBuf,i);
	sendBuf[i-2] = (U8)(checkSum&0x00FF);
	sendBuf[i-1] = (U8)(checkSum>>8);
	if((yxProtocolPkg.isSos & 0x10) == 0)
		yxProtocolPkg.isSos |= 0x10;
	return i;
}

U16 YxAppBuildTxtCmdHeartUploadPackage(U8 *sendBuf,U16 maxLen)//心跳包,准备要上传的数据
{
	YXSYSTEMPARAM     *setParam = YxAppGetSystemParam(0);
	YXLOGPARAM        *logParam = NULL;
	MYTIME            oldtime;
	S8   *tempStr = (S8*)(sendBuf+4);
	U16  i = 0,checkSum = 0,j = 0;
	GetDateTime(&oldtime);
	sprintf(tempStr,"%c1%05d%s+XT=\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei);
	j = (U16)strlen(tempStr);
	tempStr += j;
	if(yxProtocolPkg.isFirst)//上传自检包
	{
		YXMONITORPARAM    *moniInfor = NULL;
		S8   *serverName = NULL;
#ifdef __MMI_BT_SUPPORT__
		U8   *btMac = NULL,*btName = NULL;
#endif
#ifdef __MMI_WLAN_FEATURES__
		U8   *wMac = NULL;
#endif
#ifdef __MMI_BT_SUPPORT__
		YxAppGetLocalBtNameMac(&btName,&btMac);
#endif
		serverName = YxAppGetServerName(&i);
#ifdef __MMI_WLAN_FEATURES__
		wMac = YxAppSetAndGetLocalMac(NULL);
#endif
#if defined(__MMI_BT_SUPPORT__)&&defined(__MMI_WLAN_FEATURES__)
		sprintf(tempStr,"+IKVER=V:%s,SRV:%s,PORT:%d,BMAC:%s,BNAME:%s,WMAC:%02x:%02x:%02x:%02x:%02x:%02x\r\n",YXAPP_WATCH_VERSION,serverName,i,(S8*)btMac,(S8*)btName,wMac[0],wMac[1],wMac[2],wMac[3],wMac[4],wMac[5]);
#elif defined(__MMI_BT_SUPPORT__)
		sprintf(tempStr,"+IKVER=V:%s,SRV:%s,PORT:%d,BMAC:%s,BNAME:%s,WMAC:%02x:%02x:%02x:%02x:%02x:%02x\r\n",YXAPP_WATCH_VERSION,serverName,i,(S8*)btMac,(S8*)btName,0,0,0,0,0,0);
#elif defined(__MMI_WLAN_FEATURES__)
		sprintf(tempStr,"+IKVER=V:%s,SRV:%s,PORT:%d,BMAC:00:00:00:00:00:00,BNAME:BT,WMAC:%02x:%02x:%02x:%02x:%02x:%02x\r\n",YXAPP_WATCH_VERSION,serverName,i,wMac[0],wMac[1],wMac[2],wMac[3],wMac[4],wMac[5]);
#else
		sprintf(tempStr,"+IKVER=V:%s,SRV:%s,PORT:%d,BMAC:00:00:00:00:00:00,BNAME:BT,WMAC:%02x:%02x:%02x:%02x:%02x:%02x\r\n",YXAPP_WATCH_VERSION,serverName,i,0,0,0,0,0,0);
#endif
		j = (U16)strlen(tempStr);
		tempStr += j;
		moniInfor = YxAppGetMonitorList(0,0);
		if(moniInfor->number[0]==0)//获取监护号码列表
		{
			strcpy(tempStr,"+IKQXHM=GET\r\n");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		moniInfor = YxAppGetFirewallItemBy(0);
		if(moniInfor->number[0]==0)//获取白名单号码列表
		{
			strcpy(tempStr,"+IKBMD=GET\r\n");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		if(YxAppGetEfenceNumber()==0)//获取电子围栏列表
		{
			strcpy(tempStr,"+IKDZWL=GET\r\n");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		if(YxAppFirewallReadWriteStatus(3)==0)//获取防火状态
		{
			strcpy(tempStr,"+IKFHQ=GET\r\n");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		if(YxAppFirewallReadWriteTime(1,NULL,NULL)==0)
		{
			strcpy(tempStr,"+IKMDR=GET\r\n");//获取免打扰时间
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
#if 0 //不需要服务设置时间,已添加基站授时,插入SIM卡后,即可自动设置时间
		strcpy(tempStr,"+IKDT=GET\r\n");//服务器授时
		j = (U16)strlen(tempStr);
		tempStr += j;
#endif
		if(YxAppGetActiveAlarmNumber()==0)//get alarm
		{
			strcpy(tempStr,"+IKALM=GET\r\n");
			j = (U16)strlen(tempStr);
			tempStr += j;
		}
		//爱心奖励
		strcpy(tempStr,"+IKJL=GET\r\n");
		j = (U16)strlen(tempStr);
		tempStr += j;
		//信号,电量百分比(0-100),情景模式('1':一般,'2':静音,'3':会议,'4':户外)
		oldtime.nYear -= 2000;
		i = YxAppGetSignalLevelInPercent();
		sprintf(tempStr,"+IKSYS=RF:%02d,BAT:%02d,PRO:%c,GPRS:%05d,GPS:%05d,GSEN:%d,DT:%02d%02d%02d%02d%02d\r\n",i,YxAppBatteryLevel(),YxAppGetCurrentProfile(),setParam->uploadTick,setParam->gpsTimes,setParam->gsensor,oldtime.nYear,oldtime.nMonth,oldtime.nDay,oldtime.nHour,oldtime.nMin);
		j = (U16)strlen(tempStr);
		tempStr += j;
		yxProtocolPkg.isFirst = 2;
	}
	//check log first
	i = 0;
	j = 0;
	while(i<YX_LOGS_MAX_NUM)
	{
		logParam = YxAppGetLogByIndex((char)i);
		if(logParam&&(logParam->kind!=LOG_EFENCE))
		{
			j = 1;
			break;
		}
		i++;
	}
	//send log
	if(j==1)
	{
		i = 0;
		strcpy(tempStr,"+IKLOG=");//log
		j = (U16)strlen(tempStr);
		tempStr += j;
		while(i<YX_LOGS_MAX_NUM)
		{
			logParam = YxAppGetLogByIndex((char)i);
			if(logParam&&(logParam->kind!=LOG_EFENCE))
			{
				if(YxAppGetLogByIndex((char)(i+1))==NULL)
					sprintf(tempStr,"K:%c,DT:%02d%02d%02d%02d,ACT:%c,V:%04x",logParam->kind,(logParam->date>>8),logParam->date&0x00FF,logParam->time>>8,logParam->time&0x00FF,logParam->action,logParam->value);
				else
					sprintf(tempStr,"K:%c,DT:%02d%02d%02d%02d,ACT:%c,V:%04x|",logParam->kind,(logParam->date>>8),logParam->date&0x00FF,logParam->time>>8,logParam->time&0x00FF,logParam->action,logParam->value);
				logParam->sent = 1;
				j = (U16)strlen(tempStr);
				tempStr += j;
			}
			i++;
		}
		strcpy(tempStr,"\r\n");
		tempStr += 2;
	}
	//ye sms
	if((yxProtocolPkg.isYe>0)&&(yxProtocolPkg.yeLength>0))
	{
		sprintf(tempStr,"+IKYE=%04x",yxProtocolPkg.yeLength);
		j = (U16)strlen(tempStr);
		tempStr += j;
		memcpy((void*)tempStr,(void*)yxProtocolPkg.yeSms,yxProtocolPkg.yeLength);//utf8
		tempStr += yxProtocolPkg.yeLength;
		strcpy(tempStr,"\r\n");
		tempStr += 2;
		yxProtocolPkg.isYe = 2;
	}
	//end
	i = (U16)(strlen((S8*)(sendBuf+4))+6);
	sendBuf[0] = (U8)(i&0x00FF);
	sendBuf[1] = (U8)(i>>8);
	sendBuf[2] = 0;
	sendBuf[3] = 0;
	checkSum = YxProtocolCheckSum(sendBuf,i);
	sendBuf[i-2] = (U8)(checkSum&0x00FF);
	sendBuf[i-1] = (U8)(checkSum>>8);
	return i;
}

static S32 YxAppSendUnloadData(U8 *sendBuf,U16 maxLen)
{
	S32   datLength = YxAppGetUnsendDataLength(),i = 0,j = 0,bufLen = 0;
	U8    *tempBuf = sendBuf;
	U16   checkSum = 0;
	YXSYSTEMPARAM     *setParam = YxAppGetSystemParam(0);
	if((datLength<4)||(datLength > YX_UNSEND_PKG_FILE_LEN))
		return 0;
	if(datLength<=maxLen-YX_UNSEND_PKG_HEADER_LEN)  /**对于文件大小<=maxLen-YX_UNSEND_PKG_HEADER_LEN*/
		bufLen = maxLen - YX_UNSEND_PKG_HEADER_LEN;
	else
	{
		tempBuf = yxProtocolPkg.bigBuffer;
		bufLen = YX_UNSEND_PKG_FILE_LEN;
	}
	i = 4;
	sprintf((char*)(tempBuf+4),"%c%c%05d%s",'5',PROTOCOL_VER,CUSTOMER_ID,setParam->imei);
	j = strlen((S8*)(tempBuf+i));
	i += j;
	tempBuf[i++] = (U8)datLength;
	tempBuf[i++] = (U8)(datLength>>8);
	tempBuf[i++] = (U8)(datLength>>16);
	tempBuf[i++] = (U8)(datLength>>24);
	YxAppGetUnsendDataFromFile(tempBuf+i,bufLen,0);
	i += datLength;
	i += 2;//crc
	tempBuf[0] = (U8)(i&0x00FF);
	tempBuf[1] = (U8)(i>>8);
	tempBuf[2] = (U8)(i>>16);
	tempBuf[3] = (U8)(i>>24);
	checkSum = YxProtocolCheckSum(tempBuf,i);
	tempBuf[i-2] = (U8)(checkSum&0x00FF);
	tempBuf[i-1] = (U8)(checkSum>>8);
	return i;
}

S32 YxAppSockConnectedCallback(U8 *sendBuf,U16 maxLen)//maxlen:YX_SOCK_BUFFER_LEN,连接服务器成功时的回调函数,可以在里面准备要上传的数据,并上传到服务器上.返回>0:代表要发送数据到服务器,0则不要
{
	U16  flag = YxAppGetRunFlag(1);
	YxProtocolFreeRes();
	if(flag & YX_UPLOAD_KIND_GPS)
	{
		S32   datLength = YxAppSendUnloadData(sendBuf,maxLen);
		if(datLength>0)
			return datLength;
		else
			return YxAppBuildTxtCmdGpsUploadPackage(0,sendBuf,maxLen);
	}
	else
		return YxAppBuildTxtCmdHeartUploadPackage(sendBuf,maxLen);
}

static U16 YxAPPCommandProc(S8 *cmdStr,U16 length,U8 *resCmd)
{
#define MAX_CMD_LIST_NUM    22
	S8     cmdKind = 0,find = 0;
	U16    i = 0;
	const  S8 *cmdList[MAX_CMD_LIST_NUM]=
	{
		"+IKQXHM=",
		"+IKBMD=",
		"+IKFHQ=",
		"+IKMDR=",
		"+IKSET=",
		"+IKDT=",
		"+IKDZWL=",
		"+IKALM=",
		"+IKRES=",
		"+IKBT=",
		"+IKOFF=",
		"+IKSWT=",
		"+IKDIAL=",
		"+IKSMS=",
		"+IKLCDT=",
		"+IKSCRK=",
		"+IKRPS=",
		"+IKPIC=",
		"+IKSLP=",
		"+IKDTK=",
		"+IKYE=",
		"+IKJL="
	};
	while(cmdKind<MAX_CMD_LIST_NUM)
	{
		if(strncmp(cmdStr,(char*)cmdList[cmdKind],strlen((char*)cmdList[cmdKind]))==0)
		{
			find = 1;
			break;
		}
		cmdKind++;
	}
	if(find==0)
	{
		i = 0;
		while(i<length)
		{
			if((cmdStr[i]==0x0D)&&(cmdStr[i+1]==0x0A))
			{
				i += 2;
				break;
			}
			i++;
		}
		return i;
	}
	*resCmd = 0;
	switch(cmdKind)
	{
	case 0://+IKQXHM=爸爸&#1232123131231|妈妈&#1231233322\r\n
	case 1://+IKBMD=叔叔&#1232123131231|大伯父&#1231233322\r\n
		{
			char num[YX_APP_CALL_NUMBER_MAX_LENGTH+1],j = 0,k = 0,names[(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<2];
			U16  oldi = 0;
			if(cmdKind==0)
				cmdStr += 8;
			else
				cmdStr += 7;
			while((cmdStr[i]!=0x0D)&&(cmdStr[i+1]!=0x0A))
			{
				j = 0;
				oldi = i;
				while((cmdStr[i]!='&')&&(cmdStr[i+1]!='#'))
				{
					j++;
					i++;
				}
				if(j>0)
					YxAppUTF8ToUincode((U8*)(cmdStr+oldi),j,(U8*)names,(U16)((YX_APP_CALL_NAMES_MAX_LENGTH+1)<<2));
				else
				{
					names[0] = 0;
					names[1] = 0;
				}
				i += 2;
				j = 0;
				while((cmdStr[i]!='|')&&(cmdStr[i]!='\r'))
				{
					num[j++] = cmdStr[i++];
					if(j>=YX_APP_CALL_NUMBER_MAX_LENGTH)
						break;
				}
				num[j++] = 0x00;
				if(cmdKind==0)
					YxAppSetMonitorInfor(k,num,(U8*)names);
				else
					YxAppSetFirewallInfor(k,num,(U8*)names);
				k++;
				if(cmdStr[i]=='|')
					i++;
			}	
			if(cmdKind==0)
			{
				i += 10;
				while(k<YX_MAX_MONITOR_NUM)
				{
					YxAppSetMonitorInfor(k,NULL,NULL);
					k++;
				}
				YxAppSaveMonitorList();
			}
			else
			{
				i += 9;
				while(k<YX_WHITE_NUM_LIST_MAX)
				{
					YxAppSetFirewallInfor(k,NULL,NULL);
					k++;
				}
				YxAppSaveFirewallList();
			}
		}
		return i;
	case 2://+IKFHQ=ON\r\n
		cmdStr += 7;
		if(strncmp((S8*)cmdStr,"ON",2)==0)
		{
			if(YxAppFirewallReadWriteStatus(3)!=1)
			{
				YxAppFirewallReadWriteStatus(1);
				YxAppSaveFirewallList();
			}
		}
		else//off
		{
			if(YxAppFirewallReadWriteStatus(3)!=0)
			{
				YxAppFirewallReadWriteStatus(0);
				YxAppSaveFirewallList();
			}
		}
		while((cmdStr[i]!=0x0D)&&(cmdStr[i+1]!=0x0A))
		{
			i++;
		}
		i += 9;
		return i;
	case 3://+IKMDR=08301150,14301750,00000000\r\n
		{
			U16    sTime[3],eTime[3];
			cmdStr += 7;
			sTime[0] = (((cmdStr[0]-'0')*10 + (cmdStr[1]-'0'))<<8)|((cmdStr[2]-'0')*10 + (cmdStr[3]-'0'));
			eTime[0] = (((cmdStr[4]-'0')*10 + (cmdStr[5]-'0'))<<8)|((cmdStr[6]-'0')*10 + (cmdStr[7]-'0'));
			sTime[1] = (((cmdStr[9]-'0')*10 + (cmdStr[10]-'0'))<<8)|((cmdStr[11]-'0')*10 + (cmdStr[12]-'0'));
			eTime[1] = (((cmdStr[13]-'0')*10 + (cmdStr[14]-'0'))<<8)|((cmdStr[15]-'0')*10 + (cmdStr[16]-'0'));
			sTime[2] = (((cmdStr[18]-'0')*10 + (cmdStr[19]-'0'))<<8)|((cmdStr[20]-'0')*10 + (cmdStr[21]-'0'));
			eTime[2] = (((cmdStr[22]-'0')*10 + (cmdStr[23]-'0'))<<8)|((cmdStr[24]-'0')*10 + (cmdStr[25]-'0'));
			if(YxAppFirewallReadWriteTime(0,sTime,eTime)==1)
				YxAppSaveFirewallList();
			while((cmdStr[i]!=0x0D)&&(cmdStr[i+1]!=0x0A))
			{
				i++;
			}
			i += 9;
		}
		return i;
	case 4://+IKSET=BAT:20,PRO:1,GPRS:00001,GPS:00020,GSEN:1,SBT:0\r\n
		{
			char   number[7],bat=0,pro=0,*odlStr = cmdStr,gsensor = 0,searchBt = 0;
			U16    gprs = 0,gps = 0;
			YXSYSTEMPARAM *setParam = YxAppGetSystemParam(0);
			cmdStr += 7;
			while((cmdStr[0]!=0x0D)&&(cmdStr[1]!=0x0A))
			{
				cmdStr = strstr(cmdStr,"BAT:");
				if(cmdStr)
				{
					cmdStr += 4;
					number[0] = cmdStr[0];
					number[1] = cmdStr[1];
					number[2] = 0;
					bat = (char)atoi(number);
					cmdStr += 3;
				}
				else
					bat = 0;
				cmdStr = strstr(cmdStr,"PRO:");
				if(cmdStr)
				{
					cmdStr += 4;
					pro = cmdStr[0];
					cmdStr += 2;
				}
				else
					pro = 0;
				cmdStr = strstr(cmdStr,"GPRS:");
				if(cmdStr)
				{
					cmdStr += 5;
					number[0] = cmdStr[0];
					number[1] = cmdStr[1];
					number[2] = cmdStr[2];
					number[3] = cmdStr[3];
					number[4] = cmdStr[4];
					number[5] = 0;
					gprs = (U16)atoi(number);
					cmdStr += 6;
				}
				else
					gprs = 0;
				cmdStr = strstr(cmdStr,"GPS:");
				if(cmdStr)
				{
					cmdStr += 4;
					number[0] = cmdStr[0];
					number[1] = cmdStr[1];
					number[2] = cmdStr[2];
					number[3] = cmdStr[3];
					number[4] = cmdStr[4];
					number[5] = 0;
					gps = (U16)atoi(number);
					cmdStr += 5;
				}
				else
					gprs = 0;
				cmdStr = strstr(cmdStr,"GSEN:");
				if(cmdStr)
				{
					cmdStr += 5;
					if(cmdStr[0]=='1')
						gsensor = 1;
					cmdStr++;
				}
				else
					gsensor = 0;
				cmdStr = strstr(cmdStr,"SBT:");
				if(cmdStr)
				{
					cmdStr += 4;
					if(cmdStr[0]=='1')
						searchBt = 1;
					cmdStr++;
				}
				else
				{
					searchBt = 0;
					break;
				}
			}
			if((pro>='1')&&(YxAppGetCurrentProfile()!=pro))
				YxAppSetCurrentProfile(pro);
			if((bat>0)||(setParam->uploadTick != gprs)||(setParam->gpsTimes != gps)||(setParam->gsensor != gsensor)||(setParam->searchBt != searchBt))
			{
				char needSave = 0;
				if(gprs>=YX_HEAT_TIME_MIN && gprs<=YX_HEAT_TIME_MAX)
				{
					needSave = 1;
					setParam->uploadTick = gprs;
				}
				if(gps>=YX_GPS_TIME_MIN && gps<=YX_GPS_TIME_MAX)
				{
					needSave = 2;
					setParam->gpsTimes = gps;
				}
				if(bat>0)
					needSave = 3;
				if(setParam->gsensor != gsensor)
				{
					needSave = 4;
					YxAppSendMsgToMMIMod(YX_MSG_GSENSOR_CTRL,(gsensor==1)?1:0,0);
				}
				if(setParam->searchBt != searchBt)
					needSave = 5;
				if(needSave)
				{
					if(YxAppSystemReadWriteLowBattery(0,bat)==0)
						YxAppSaveSystemSettings();
				}
			}
			cmdStr += 2;
			i = (U16)(cmdStr - odlStr);
		}
		return i;
	case 5://+IKDT=20150713202458\r\n
		{
			MYTIME  oldtime;
			cmdStr += 6;
			oldtime.nSec = (cmdStr[12]-'0')*10+(cmdStr[13]-'0');
			oldtime.nMin = (cmdStr[10]-'0')*10+(cmdStr[11]-'0');
			oldtime.nHour = (cmdStr[8]-'0')*10+(cmdStr[9]-'0');
			oldtime.nDay = (cmdStr[6]-'0')*10+(cmdStr[7]-'0');
			oldtime.nMonth = (cmdStr[4]-'0')*10+(cmdStr[5]-'0');
			oldtime.nYear = (cmdStr[0]-'0')*1000+(cmdStr[1]-'0')*100+(cmdStr[2]-'0')*10+(cmdStr[3]-'0');
			if(oldtime.nYear>=2015)
			{
				mmi_dt_set_dt((const MYTIME*)&oldtime, NULL, NULL);
				YxAppGpsSetServerTimeStatus();
			}
			i += 22;
		}
		return i;
	case 6://+IKDZWL=N:名称&#,T:2,LON:114.233344,LAT:23.444232,RAD:1500|N:名称&#,T:1,LON:114.233344,LAT:23.444232,RAD:1500|N:名称&#,T:0,LON:114.233344,LAT:23.444232,RAD:1500\r\n
		{
			S8   number[13],k = 0,names[(YX_EFENCE_NAME_LENGTH+1)<<2],*odlStr = cmdStr,type = '0',j = 0;
			double  lon = 0.0,lat = 0.0,rad = 0.0;
			cmdStr += 8;
			while((cmdStr[0]!=0x0D)&&(cmdStr[1]!=0x0A))
			{
				cmdStr = strstr(cmdStr,"N:");
				names[0] = 0;
				names[1] = 0;
				if(cmdStr)
				{
					j = 0;
					cmdStr += 2;
					while((cmdStr[j]!='&')&&(cmdStr[j+1]!='#'))
					{
						j++;
					}
					if(j>0)
						YxAppUTF8ToUincode((U8*)cmdStr,j,(U8*)names,(U16)((YX_EFENCE_NAME_LENGTH+1)<<2));
					j += 3;
					cmdStr += j;
				}
				cmdStr = strstr(cmdStr,"T:");
				if(cmdStr)
				{
					cmdStr += 2;
					type = cmdStr[0];
					cmdStr += 2;
				}
				else
					type = 0;
				cmdStr = strstr(cmdStr,"LON:");
				if(cmdStr)
				{
					j = 0;
					cmdStr += 4;
					while(cmdStr[j]!=',')
					{
						number[j] = cmdStr[j];
						j++;
						if(j>=12)
							break;
					}
					number[j++] = 0;
					lon = atof(number);
					cmdStr += j;
				}
				else
					lon = 0.0;
				cmdStr = strstr(cmdStr,"LAT:");
				if(cmdStr)
				{
					j = 0;
					cmdStr += 4;
					while(cmdStr[j]!=',')
					{
						number[j] = cmdStr[j];
						j++;
						if(j>=12)
							break;
					}
					number[j++] = 0;
					lat = atof(number);
					cmdStr += j;
				}
				else
					lat = 0.0;
				cmdStr = strstr(cmdStr,"RAD:");
				if(cmdStr)
				{
					j = 0;
					cmdStr += 4;
					while((cmdStr[j]!='|')&&(cmdStr[j]!='\r'))
					{
						number[j] = cmdStr[j];
						j++;
						if(j>=12)
							break;
					}
					number[j] = 0;
					rad = atof(number);
					if(cmdStr[j]=='|')
						j++;
					cmdStr += j;
				}
				else
				{
					rad = 0.0;
					break;//error
				}
				if(names[0]==0 && names[1]==0)
				{
					k++;
					continue;
				}
				else
				{
					YxAppEfenceUpdate(k,(U8*)names,type,lon,lat,rad);
					k++;
				}
			}
			cmdStr += 2;
			while(k<YX_EFENCE_MAX_NUM)
			{
				YxAppEfenceUpdate(k,NULL,0,0.0,0.0,0.0);
				k++;
			}
			YxAppEfenceSaveData();
			i = (U16)(cmdStr-odlStr);
		}
		return i;
	case 7://+IKALM=T:0810,ON:1,F:0,DAY:00|T:0810,ON:1,F:1,DAY:00|T:0810,ON:1,F:2,DAY:7F\r\n
		{
			S8  k = 0,hour = 0,min = 0,status = 0,freq = 0,*odlStr = cmdStr;
			U8  days = 0;
			cmdStr += 7;
			while((cmdStr[0]!=0x0D)&&(cmdStr[1]!=0x0A))
			{
				cmdStr = strstr(cmdStr,"T:");
				if(cmdStr)
				{
					cmdStr += 2;
					hour = (cmdStr[0]-'0')*10+(cmdStr[1]-'0');
					min = (cmdStr[2]-'0')*10+(cmdStr[3]-'0');
					cmdStr += 5;
				}
				else
				{
					hour = 0;
					min = 0;
				}
				cmdStr = strstr(cmdStr,"ON:");
				if(cmdStr)
				{
					cmdStr += 3;
					status = cmdStr[0]-'0';
					cmdStr += 2;
				}
				else
					status = 0;
				cmdStr = strstr(cmdStr,"F:");
				if(cmdStr)
				{
					cmdStr += 2;
					freq = cmdStr[0];
					cmdStr += 2;
				}
				else
					freq = 0;
				cmdStr = strstr(cmdStr,"DAY:");
				if(cmdStr)
				{
					S8     numb[3];
					int    tv = 0;
					cmdStr += 4;
					numb[0] = cmdStr[0];
					numb[1] = cmdStr[1];
					numb[2] = 0;
					sscanf(numb,"%x",&tv);
					days = (U8)(tv&0x7F);
					cmdStr += 2;
					if(cmdStr[0]=='|')
						cmdStr++;
				}
				else
				{
					days = 0;
					break;
				}
				YxAppSetAlarm(k,hour,min,status,freq,days);
				k++;
			}
			cmdStr += 2;
			while(k<YX_MAX_ALARM_NUM)
			{
				YxAppSetAlarm(k,0,0,0,0,0);
				k++;
			}
			i = (U16)(cmdStr - odlStr);
		}
		return i;
	case 8://+IKRES=1\r\n
		cmdStr += 7;
		if(cmdStr[0]=='1' || cmdStr[0]=='4')//ok
		{
			if(yxProtocolPkg.sendOk==0)
				yxProtocolPkg.sendOk = 1;
			if(yxProtocolPkg.isFirst==2)
				yxProtocolPkg.isFirst = 0;
			if(yxProtocolPkg.isYe==2)
			{
				yxProtocolPkg.isYe = 0;
				yxProtocolPkg.yeLength = 0;
			}
			if(yxProtocolPkg.isSos & 0x10)
				yxProtocolPkg.isSos = 0;
			if(cmdStr[0]=='4')
			{
				YxAppSetLogSentStatus();
				*resCmd = 4;//close socket
			}
		}
		else if(cmdStr[0]=='2')//ok,过去包数据OK
			*resCmd = 1;//需要回复LOG
		i += 10;
		return i;
	case 9://+IKBT=1\r\n
		cmdStr += 6;
		if(cmdStr[0]=='1')//ok
			*resCmd = 2;//需要回复BT mac
		i += 9;
		return i;
	case 10://+IKOFF=1\r\n
		cmdStr += 7;
		if(cmdStr[0]=='1')//ok
			*resCmd = 3;//不需要回复远程关机OK，直接CLOSE
		i += 10;
		return i;
	case 11://+IKSWT=1\r\n
		cmdStr += 7;
		if(cmdStr[0]=='1')//ok
			YxAppSearchWtPlayTone();
		i += 10;
		return i;
	case 12://+IKDIAL=ON:1,N:%d\r\n
		{
			S8  *callN = YxAppGetCallNumberBuffer();
			cmdStr += 11;
			memset((void*)callN,0,YX_APP_CALL_NUMBER_MAX_LENGTH+1);
			if(cmdStr[0]=='1')//语音监听
				*resCmd = 5;
			else //打指定号码电话
				*resCmd = 6;
			cmdStr = strstr(cmdStr,"N:");
			if(cmdStr)
			{
				i = 0;
				cmdStr += 2;
				while(cmdStr[i]!='\r')
				{
					if(i<YX_APP_CALL_NUMBER_MAX_LENGTH)
						callN[i] = cmdStr[i];
					i++;
				}
				i += 2;
			}
			i += 15;
		}
		return i;
	case 13://+IKSMS=S:发送者&#文本字符&#\r\n,暂时不支持
		cmdStr += 7;
		while(!(cmdStr[i]=='&'&&cmdStr[i+1]=='#'&&cmdStr[i+2]=='\r'&&cmdStr[i+3]=='n'))
		{
			i++;
		}
		i += 4;
		return i;
	case 14://+IKLCDT=T:%c\r\n,背光时间(0:5S,1:15S,2:30S,3:45S,4:60S)
		cmdStr += 10;
		YxAppSetBacklightTime((S8)cmdStr[0]);
		return 13;
	case 15://+IKSCRK=K:%c\r\n,暂时不支持
		return 13;
	case 16://+IKRPS=1\r\n,实时定位指令
		cmdStr += 7;
		if(cmdStr[0]=='1')//ok
			*resCmd = 7;//回复OK,再等服务器CLOSE后,运行定位包
		return 10;
	case 17://+IKPIC=1\r\n,实时拍照指令,暂时不支持
		return 10;
	case 18: //+IKSLP=ON:%c,T:%02d%02d%02d%02d\r\n,睡眠检测设置,暂时不支持
		return 24;
	case 19://+IKDTK=1\r\n,1:24小时制,0:12小时制
		cmdStr += 7;
		YxAppSetTimeFormat((S8)cmdStr[0]);
		return 10;
	case 20://+IKYE=1\r\n,查询话费
		if(yxProtocolPkg.isYe==0)
		{
			yxProtocolPkg.isYe = 1;
			yxProtocolPkg.yeLength = 0;
			YxAppSmsSendGetMoney();
		}
		return 9;
	case 21://+IKJL=01\r\n,//爱心奖励,数字固定为两位,不足补0,+IKJL=%02d\r\n
		{
			YXSYSTEMPARAM *setParam = YxAppGetSystemParam(0);
			cmdStr += 6;
			i = (cmdStr[0]-'0')*10 + (cmdStr[1]-'0');
			if(i != setParam->heartNum)
			{
				setParam->heartNum = (U8)i;
				YxAppSaveSystemSettings();
			}
		}
		return 10;
	default:
		return 0;
	}
}

static U16 YxAppShortCmdBuild(char kind,U8 *sendBuffer)
{
	U16  i = 0,checkSum = 0;
	YXSYSTEMPARAM *setParam = YxAppGetSystemParam(0);
	sprintf((char*)(sendBuffer+4),"%c1%05d%s+IKRES=%c\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei,kind);
	i = (U16)(strlen((char*)(sendBuffer+4)) + 6);
	sendBuffer[0] = (U8)(i&0x00FF);
	sendBuffer[1] = (U8)(i>>8);
	sendBuffer[2] = 0;
	sendBuffer[3] = 0;
	checkSum = YxProtocolCheckSum(sendBuffer,i);
	sendBuffer[i-2] = (U8)(checkSum&0x00FF);
	sendBuffer[i-1] = (U8)(checkSum>>8);
	return i;
}

static U16 YxAppTxtCmdProcMain(U8 *data,U16 dataLen,U8 *sendBuff,U16 sendBufMaxLen)
{
	YXSYSTEMPARAM    *setParam = YxAppGetSystemParam(0);
	S8    *tempStr = (S8*)(data+26),kind='1';
	U8    resCmd = 0,saveFlag = 0;
	U16   j = 0,checkSum = 0,i = 0;
	dataLen -= 26;
	while(i<dataLen)
	{
		j = YxAPPCommandProc(tempStr,(U16)(dataLen-i),&resCmd);
		if(resCmd != 0)
			saveFlag = resCmd;
		tempStr += j;
		i += j;
	}
	if(saveFlag==1)//log
	{
		YxAppDeleteUnsendPkgFile();
		return YxAppBuildTxtCmdGpsUploadPackage(0,sendBuff,sendBufMaxLen);
	}
	else if(saveFlag==2)//bt mac
	{
		S8   btMac[18];
		if(YxAppGetLocalBtMac(NULL,btMac)==1)
			sprintf((char*)(sendBuff+4),"%c1%05d%s+IKBT=%s\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei,btMac);
		else
			sprintf((char*)(sendBuff+4),"%c1%05d%s+IKBT=\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei);
		kind = 0;
	}
	else if(saveFlag==3)//远程关机OK,主动CLOSE
	{
		YxAppAutoShutdown();
		return 0;
	}
	else if(saveFlag==4)//收到通讯完毕指令,close socket
		return 0;
	else if(saveFlag==5)//语音监听,主动CLOSE
	{
		YxAppMakeWithCallType(YX_CALL_LISTEN,NULL);
		return 0;
	}
	else if(saveFlag==6)//回拨指定电话,主动CLOSE
	{
		YxAppMakeWithCallType(YX_CALL_NORMAL,YxAppGetCallNumberBuffer());
		return 0;
	}
	else if(saveFlag==7)//实时定位,先回复OK,等服务器发结束通讯指令后,再运行定位包
		YxAppSetOntimeCallGpsRunFlag();
	if(kind)
		sprintf((char*)(sendBuff+4),"%c1%05d%s+IKRES=%c\r\n",PROTOCOL_VER,CUSTOMER_ID,setParam->imei,kind);
	i = (U16)(strlen((char*)(sendBuff+4)) + 6);
	sendBuff[0] = (U8)(i&0x00FF);
	sendBuff[1] = (U8)(i>>8);
	sendBuff[2] = 0;
	sendBuff[3] = 0;
	checkSum = YxProtocolCheckSum(sendBuff,i);
	sendBuff[i-2] = (U8)(checkSum&0x00FF);
	sendBuff[i-1] = (U8)(checkSum>>8);
	return i;
}

static U16 YxAppVoiceCmdProcMain(U8 *data,U16 dataLen,U8 *sendBuff,U16 sendBufMaxLen)
{//not surpport now
	return YxAppShortCmdBuild('0',sendBuff);
}

static U16 YxAppImageCmdProcMain(U8 *data,U16 dataLen,U8 *sendBuff,U16 sendBufMaxLen)
{//not surpport now
	return YxAppShortCmdBuild('0',sendBuff);
}

static U16 YxAppAgpsCmdProcMain(U8 *data,U16 dataLen,U8 *sendBuff,U16 sendBufMaxLen)
{
	U16    length = 0;
	U8     *tempStr = data+26;
	tempStr += 8;
	length = (tempStr[1]<<8)|tempStr[0];
	tempStr += 2;
#if (YX_GPS_USE_AGPS==1)
	YxAgpsUpdateData(tempStr,length);
#endif
	return YxAppShortCmdBuild('1',sendBuff);//answer ok,ack
}

static U16 YxAppProtocolDataProcMain(U8 *data,U16 dataLen,U8 *sendBuff,U16 sendBufMaxLen)
{
	S8   idNum[6];
	U16  i = YxProtocolCheckSum(data,dataLen),cid = 0;
	memcpy((void*)idNum,(void*)(data+6),5);
	idNum[5] = 0;
	cid = atoi(idNum);
	if((i==((data[dataLen-1]<<8)|data[dataLen-2]))&&(data[4]==PROTOCOL_VER)&&(cid!=CUSTOMER_ID))//crc and version ok
	{
		if(data[5]=='1')//txt cmd
			return YxAppTxtCmdProcMain(data,dataLen,sendBuff,sendBufMaxLen);
		else if(data[5]=='2')//voice
			return YxAppVoiceCmdProcMain(data,dataLen,sendBuff,sendBufMaxLen);
		else if(data[5]=='3')//image
			return YxAppImageCmdProcMain(data,dataLen,sendBuff,sendBufMaxLen);
		else //if(data[5]=='4')//AGPS data
			return YxAppAgpsCmdProcMain(data,dataLen,sendBuff,sendBufMaxLen);
	}
	return 0;
}

S32 YxAppSockReadDataCallback(U8 *readBuf,U16 readLen,U8 *sendBuffer)//readBuf:从服务器读取的数据,readLen:数据长度,可以在此处理你们的数据协议,sendBuffer:如果有数据要上传到服务器上,请填写数据到此BUFFER中,注意,此BUFFER的最大长度为YX_SOCK_BUFFER_LEN,返回值为0时,不要上传数据，按SENDBUFFER的实际长度字节返回
{
#if 1
	if(yxProtocolPkg.totalLen==0)
	{
		yxProtocolPkg.dataLen = 0;
		yxProtocolPkg.totalLen = (readBuf[3]<<24)|(readBuf[2]<<16)|(readBuf[1]<<8)|readBuf[0];
		if(yxProtocolPkg.totalLen==0)
			return 0;
		else if(yxProtocolPkg.totalLen>YX_BIG_MEMERY_LEN)
			return YxAppShortCmdBuild('3',sendBuffer);
	}
	if(yxProtocolPkg.totalLen<=YX_SOCK_READ_BUFFER_LEN)//1 one get all
	{
		U16 res = YxAppProtocolDataProcMain(readBuf,(U16)yxProtocolPkg.totalLen,sendBuffer,YX_SOCK_BUFFER_LEN);
		yxProtocolPkg.totalLen = 0;
		return res;
	}
	else
	{
		if(yxProtocolPkg.dataLen + readLen <= yxProtocolPkg.totalLen)
		{
			memcpy((void*)(yxProtocolPkg.bigBuffer+yxProtocolPkg.dataLen),(void*)readBuf,readLen);
			yxProtocolPkg.dataLen += readLen;
			if(yxProtocolPkg.dataLen>=yxProtocolPkg.totalLen)//end
			{
				U16 res = YxAppProtocolDataProcMain(yxProtocolPkg.bigBuffer,(U16)yxProtocolPkg.totalLen,sendBuffer,YX_SOCK_BUFFER_LEN);
				yxProtocolPkg.totalLen = 0;
				yxProtocolPkg.dataLen = 0;
				return res;
			}
			return -1;//continue read data from socket
		}
	}
	return 0;
#else
#if(YX_IS_TEST_VERSION!=0)
	U8     bufft[201];
#endif
#if(YX_IS_TEST_VERSION!=0)
	memset((void*)bufft,0,201);
	if(readLen>=200)
		memcpy((void*)bufft,(void*)readBuf,200);
	else// if(readLen<200)
		memcpy((void*)bufft,(void*)readBuf,readLen);
	YxAppTestUartSendData(bufft,strlen((char*)bufft));
#endif
	if(readLen>0)
	{
		char *resOk = "HTTP/1.1 200 OK\r\n";
		if(strncmp((char*)readBuf,resOk,strlen(resOk))==0)
		{
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData("send ok\r\n",9);
#endif
			return 0;//上传成功,关闭SOCKET
		}
	}
	return 0;//以实际SENDBUFFER的长度返回
#endif
}

PU8 YxProtocolGetBigBuffer(void)
{
	return yxProtocolPkg.bigBuffer;
}

void YxProtocolSetSosStatus(void)
{
	yxProtocolPkg.isSos &= 0x10;
	yxProtocolPkg.isSos |= 0x01;
}

void YxProtocolInition(void)
{
	memset((void*)&yxProtocolPkg,0,sizeof(YXPROTOCOLPARAM));
	yxProtocolPkg.bigBuffer = YxAppMemMalloc(YX_BIG_MEMERY_LEN);
	yxProtocolPkg.isFirst = 1;
}
#endif