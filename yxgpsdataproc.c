#ifdef __USE_YX_APP_SERVICE__
#include "MMI_features.h"
#include "MMIDataType.h"
#include "kal_public_api.h"
#include "kal_release.h"
#include "Gdi_layer.h"
#include "NVRAMEnum.h"
#include "NVRAMProt.h"
#include "NVRAMType.h"
#include <math.h>
#include "dcl_uart.h"
#include "custom_nvram_editor_data_item.h"
#include "DateTimeGprot.h"
#include "gui.h"
#include "TimerEvents.h"
#include "app_datetime.h"
#include "YxBasicApp.h"
#if (YX_GPS_DISPLAY_UI==1)
#include "gui_typedef.h"
#include "wgui_categories_util.h"
#endif

/////////////////////////////////////////////////////////////////////Global vars/////////////////////////////////////////////////////////////
#define YX_AGPS_IC_NO_RESTART

#define PI  3.1415926
#define YXAPP_EARTH_RADIUS     6378.137

#define YX_GPS_TIMER_ID        AVATAR_DELAY_DECODE_2

static YXGPSPARAMSTR  yxGpsParam;
static U16 yxGpsCurrentCircle = 0;
#if(YX_IS_TEST_VERSION!=0)
static char yxGpsLedFrame = 0;
#endif
#define GetDateTime(t) applib_dt_get_date_time((applib_time_struct *)t)
#define GPS_UART_READBUF_LEN  1024

#if (YX_GPS_CONVERT_DM2DD==1)
static int GpsConvertddmm2dd(const char *ddmm, char *dd);
#endif
#if(YX_IS_TEST_VERSION!=0)
extern void GpsLedSignal(char on);
#endif
static void GpsGetDataTimeOut(void);

/////////////////////////////////////////////////////////////////////ublox gps//////////////////////////////////////////////////

#if(YX_IS_TEST_VERSION!=0)
#ifndef GW02_BB
static void GpsFlashLedAnimation(void)
{
	yxGpsLedFrame++;
	if(yxGpsLedFrame==2)
		GpsLedSignal(0);
	else if(yxGpsLedFrame==3)
		GpsLedSignal(1);
	else if(yxGpsLedFrame>=4)
	{
		GpsLedSignal(0);
		yxGpsLedFrame = 0;
		StopTimer(JDD_TIMER_00);
		return;
	}
	StartTimer(JDD_TIMER_00,800,GpsFlashLedAnimation);
}

static void GpsPlayFlashLed(void)
{
	if(yxGpsLedFrame==0)
	{
		GpsLedSignal(1);
		yxGpsLedFrame = 1;
		StartTimer(JDD_TIMER_00,800,GpsFlashLedAnimation);
	}
}
#endif
#endif
static void GpsResetValues(void)
{
	yxGpsParam.gpstatus[0] = 0;
	yxGpsParam.gpstatus[1] = 0;
	yxGpsParam.gpstatus[2] = 0;

	yxGpsParam.longitudev[0] = 0.0;
	yxGpsParam.longitudev[1] = 0.0;
	yxGpsParam.longitudev[2] = 0.0;

	yxGpsParam.latitudev[0] = 0.0;
	yxGpsParam.latitudev[1] = 0.0;
	yxGpsParam.latitudev[2] = 0.0;

	yxGpsParam.hightv[0] = 0.0;
	yxGpsParam.hightv[1] = 0.0;
	yxGpsParam.hightv[2] = 0.0;

	yxGpsParam.gpsangle[0] = 0.0;
	yxGpsParam.gpsangle[1] = 0.0;
	yxGpsParam.gpsangle[2] = 0.0;

	yxGpsParam.gpsrspeed[0] = 0.0;
	yxGpsParam.gpsrspeed[1] = 0.0;
	yxGpsParam.gpsrspeed[2] = 0.0;

	yxGpsParam.gpsanglem[0] = 0.0;
	yxGpsParam.gpsanglem[1] = 0.0;
	yxGpsParam.gpsanglem[2] = 0.0;

	yxGpsParam.gpsrspeedn[0] = 0.0;
	yxGpsParam.gpsrspeedn[1] = 0.0;
	yxGpsParam.gpsrspeedn[2] = 0.0;
}

static void GpsGetDataTimeOut(void)
{
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"GPS timer out\r\n",15);
#endif
	YxAppGpsControl(0);
}

static char GpsChecksumValue(U8 *buffer,U16 maxLen)
{
	U8   i = 1,rev=0,resum = 0;
	if(buffer[0] != '$')
		return 0;
	if(strncmp((char*)(buffer+1),"BD",2)==0)
		rev = 1;
	else if(strncmp((char*)(buffer+1),"GP",2)==0)
		rev = 1;
	else if(strncmp((char*)(buffer+1),"GN",2)==0)
		rev = 1;
	else if(strncmp((char*)(buffer+1),"GL",2)==0)
		rev = 1;
	if(rev==0)
		return 0;
	while(buffer[i])
	{
		if(i!=1)
			rev ^= buffer[i];
		else
			rev = buffer[i];
		i++;
		if(buffer[i]=='*')
		{
			if(buffer[i+1]>='A')
				resum = (buffer[i+1]-'A'+10)<<4;
			else
				resum = (buffer[i+1]-'0')<<4;
			if(buffer[i+2]>='A')
				resum |= (buffer[i+2]-'A'+10);
			else
				resum |= (buffer[i+2]-'0');
			break;
		}
		if(i>=maxLen)
			break;
	}
	if(resum == rev)
		return 1;
	return 0;
}

static U8 GpsGetJWTimeValue(U8 kind,U8 *buffer,U8 timep,U8 wjpos,U16 len,char validjw)
{
	U8      tembuf[18],j = 0,ret = YX_GPS_FLAG_TIME;
	U16     i = timep;
	double  tempv = 0.0;
#if(YX_IS_TEST_VERSION!=0)
	char    logBuf[40];
#endif
	tembuf[0] = buffer[i++];
	tembuf[1] = buffer[i++];
	tembuf[2] = 0x00;
	yxGpsParam.gpshour[kind] = (U8)atoi((const char*)tembuf);
	tembuf[0] = buffer[i++];
	tembuf[1] = buffer[i++];
	tembuf[2] = 0x00;
	yxGpsParam.gpsminute[kind] = (U8)atoi((const char*)tembuf);
	tembuf[0] = buffer[i++];
	tembuf[1] = buffer[i++];
	tembuf[2] = 0x00;
	yxGpsParam.gpssecond[kind] = (U8)atoi((const char*)tembuf);
#if(YX_IS_TEST_VERSION!=0)
	sprintf(logBuf,"hour:%d minute:%d\r\n",yxGpsParam.gpshour[kind],yxGpsParam.gpsminute[kind]);
	YxAppTestUartSendData((U8*)logBuf,(U16)strlen(logBuf));
#endif
	//weidu,latitude
	i = wjpos;
	j = 0;
	while(buffer[i] !=',')
	{
		tembuf[j++] = buffer[i++];
		if(i >= len)
			break;
	}
	i++;
	tembuf[j] = 0x00;
	if(tembuf[0]>0)
	{
#if (YX_GPS_CONVERT_DM2DD==1)
		char  ddBuf[18];
		GpsConvertddmm2dd((const char*)tembuf,ddBuf);
		tempv = atof((const char*)ddBuf);
#else
		tempv = atof((const char*)tembuf);
#endif
	}
	if(validjw)
	{
		ret |= YX_GPS_FLAG_LATITUDE;
		if(buffer[i]=='S')
			tempv *= -1;
		yxGpsParam.latitudev[kind] = tempv;
		yxGpsParam.serverSetTime |= 0x02;
#if(YX_IS_TEST_VERSION!=0)
		sprintf(logBuf,"latitude:%.6f\r\n",tempv);
		YxAppTestUartSendData((U8*)logBuf,(U16)strlen(logBuf));
#endif
	}
	i += 2;
	j = 0;
	while(buffer[i] !=',')
	{
		tembuf[j++] = buffer[i++];
		if(i >= len)
			break;
	}
	i++;
	tembuf[j] = 0x00;
	if(tembuf[0]>0)
	{
#if (YX_GPS_CONVERT_DM2DD==1)
		char  ddBuf[18];
		GpsConvertddmm2dd((const char*)tembuf,ddBuf);
		tempv = atof((const char*)ddBuf);
#else
		tempv = atof((const char*)tembuf);
#endif
	}
	else
		tempv = 0.0;
	if(validjw)
	{
		ret |= YX_GPS_FLAG_LONGITITUDE;
		if(buffer[i]=='W')
			tempv *= -1;
		yxGpsParam.longitudev[kind] = tempv;
		yxGpsParam.serverSetTime |= 0x02;
#if(YX_IS_TEST_VERSION!=0)
		sprintf(logBuf,"longitude:%.6f\r\n",tempv);
		YxAppTestUartSendData((U8*)logBuf,(U16)strlen(logBuf));
#ifndef GW02_BB
		GpsPlayFlashLed();
#endif
#endif
	}
	return ret;
}

static U8 GpsUartOneLineDataProc(U8 *buffer,U16 len)
{
#if(YX_IS_TEST_VERSION!=0)
	if(len >0 && len<100)
	{
		char  dBuffer[100];
		memcpy((void*)dBuffer,(void*)buffer,len);
		dBuffer[len] = 0x00;
		YxAppTestUartSendData((U8*)dBuffer,len);
	}
#endif
	if(GpsChecksumValue(buffer,len)==1)
	{
		U8      j = 0,kind = 0xff;
		U16     i = 0;
#if(YX_IS_TEST_VERSION!=0)
		char  dblog[70];
#endif
#ifdef __USE_YX_AUTO_TEST__
		yxGpsParam.gpsIsOk = 1;
#endif
		buffer++;
		if(strncmp((char*)buffer,"BD",2)==0)
			kind = YX_GPS_KIND_BD;
		else if(strncmp((char*)buffer,"GP",2)==0)
			kind = YX_GPS_KIND_GP;
		else if(strncmp((char*)buffer,"GN",2)==0)
			kind = YX_GPS_KIND_GN;
		else if(strncmp((char*)buffer,"GL",2)==0)
			kind = YX_GPS_KIND_GL;
		if(kind != 0xff)
			buffer += 2;
		else
			return 0;
		if(strncmp((char*)buffer,"GGA",3)==0)//GGA
		{
			char   validdata = 1;
			buffer += 4;
			while(j<5)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if(i>=len)
					break;
			}
			if(buffer[i]==',')
				return 0;//not ready
			else if(buffer[i]=='0')
				validdata = 0;
			if(validdata)
			{
				j = 0;
				while(j<3)
				{
					if(buffer[i] == ',')
						j++;
					i++;
					if(i>=len)
						break;
				}
				if(buffer[i] == ',')
					yxGpsParam.hightv[kind] = 0.0;
				else
				{
					U8   tembuf[18];
					j = 0;
					while((j<18)&&buffer[i] !=',')
					{
						tembuf[j++] = buffer[i++];
						if(i >= len)
							break;
					}
					tembuf[j] = 0x00;
					if(tembuf[0]>0)
					{
						yxGpsParam.hightv[kind] = atof((const char*)tembuf);
#if(YX_IS_TEST_VERSION!=0)
						sprintf(dblog,"height:%.4f\r\n",yxGpsParam.hightv[kind]);
						YxAppTestUartSendData((U8*)dblog,strlen(dblog));
#endif
					}
				}
			}
			i = 0;
			while(buffer[i] != ',')
			{
				i++;
				if(i >= len)
					break;
			}
			i++;
			return GpsGetJWTimeValue(kind,buffer,0,(U8)i,len,validdata);
		}
		else if(strncmp((char*)buffer,"GLL",3)==0)//GLL
		{
			char  validdata = 0;
			U8    ip = 0;
			buffer += 4;
			while(j<5)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if((!ip)&&(j==4))
					ip = (U8)i;
				if(i>=len)
					break;
			}
			if(buffer[i]==',')
				return 0;//not ready
			else if(buffer[i]=='A')
				validdata = 1;
			return GpsGetJWTimeValue(kind,buffer,ip,0,len,validdata);
		}
		else if(strncmp((char*)buffer,"RMC",3)==0)//RMC
		{
			U8     ip = 0,tempb[20],rest = 0;
			int    datev = 0;
			char   validdata = 1;
			double speadv = 0.0,angdata = 0.0;
			buffer += 4;
			while(j<1)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if(i>=len)
					break;
			}
			if(buffer[i]==',')
				return 0;//not ready
			else if (buffer[i]=='V')
				validdata = 0;
			i += 2;
			ip = (U8)i;
			if(validdata)//get speed and direction
			{
				j = 0;
				while(j<4)
				{
					if(buffer[i] == ',')
						j++;
					i++;
					if(i>=len)
						break;
				}
				j = 0;
				while((j < 19)&&(buffer[i] != ','))
				{
					tempb[j++] = buffer[i++];
				}
				tempb[j++] = 0x00;//speed
				if(tempb[0]>0)
					speadv = atof((const char*)tempb);
				//get direction
				i++;
				j = 0;
				while((j < 19)&&(buffer[i] != ','))
				{
					tempb[j++] = buffer[i++];
				}
				tempb[j++] = 0x00;//direct
				if(tempb[0]>0)
					angdata = atof((const char*)tempb);
			}
			//get date
			j = 0;
			i = ip;
			while(j<6)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if(i>=len)
					break;
			}
			j = 0;
			while(j < 8)
			{
				if(buffer[i] != ',')
					tempb[j++] = buffer[i];
				else
				{
					i++;
					break;
				}
				i++;
			}
			tempb[j] = 0x00;
			datev = atoi((const char*)tempb);
			yxGpsParam.gpsday[kind] = (U8)(datev/10000); 
			datev %= 10000;
			yxGpsParam.gpsmonth[kind] = (U8)(datev/100);
			yxGpsParam.gpsyear[kind] = (U8)(datev%100);
			j = 0;
			while(j<2)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if(i>=len)
					break;
			}
			if(buffer[i] == 'N')
			{
				validdata = 0;
				rest = YX_GPS_FLAG_INVALID_DATE;
			}
			else
				rest = YX_GPS_FLAG_DATE;
			rest |= GpsGetJWTimeValue(kind,buffer,0,ip,len,validdata);
			if(validdata)
			{
				yxGpsParam.gpsrspeedn[kind] = speadv;
				yxGpsParam.gpsangle[kind] = angdata;
				rest |= YX_GPS_FLAG_SPEED;
			}
#if(YX_IS_TEST_VERSION!=0)
			sprintf(dblog,"data,valid:%d,y:%d,m:%d,d:%d,spd:%.4f,angle:%.2f\r\n",validdata,yxGpsParam.gpsyear[kind]+2000,yxGpsParam.gpsmonth[kind],yxGpsParam.gpsday[kind],yxGpsParam.gpsrspeedn[kind],yxGpsParam.gpsangle[kind]);
			YxAppTestUartSendData((U8*)dblog,strlen(dblog));
#endif
			return rest;
		}
		else if(strncmp((char*)buffer,"GSA",3)==0)//GSA
		{
			U8   tempv = 0;
			buffer += 4;
			if(buffer[i]=='M')
				tempv = 1;
			i += 2;
			tempv |= (buffer[i]-'0')<<1;
			yxGpsParam.gpstatus[kind] = tempv;
			if(((tempv&YX_GPS_GSA_POS_FLAG)>>1)!=1)//pos ok
			{
				U8   num = 0;
				i += 2;
				j = 0;
				while(j<12)
				{
					if(buffer[i]!=',')
					{
						num++;
						i += 2;
					}
					i++;
					j++;
				}
				tempv |= num<<3;
				yxGpsParam.serverSetTime |= 0x02;
				yxGpsParam.noGpsCount = 0;
#if(YX_IS_TEST_VERSION!=0)
				sprintf(dblog,"posok:%d\r\n",num);
				YxAppTestUartSendData((U8*)dblog,strlen(dblog));
#endif
			}
#if(YX_IS_TEST_VERSION!=0)
			else
				YxAppTestUartSendData((U8*)"poserr\r\n",8);
#endif
			return 0;
		}
		else if(strncmp((char*)buffer,"VTG",3)==0)//VTG
		{
			char    number[20],res=0;
			double  tempVal = 0.0;
			buffer += 4;
			while((j < 19)&&(buffer[i] != ','))
			{
				number[j++] = buffer[i++];
			}
			number[j++] = 0x00;
			if(number[0]>0)
			{
				tempVal = atof((const char*)number);
				yxGpsParam.gpsangle[kind] = tempVal;
			}
			i += 3;
			j = 0;
			while((j < 19)&&(buffer[i] != ','))
			{
				number[j++] = buffer[i++];
			}
			number[j++] = 0x00;
			if(number[0]>0)
			{
				tempVal = atof((const char*)number);
				yxGpsParam.gpsanglem[kind] = tempVal;
			}
			i += 3;
			j = 0;
			while((j < 19)&&(buffer[i] != ','))
			{
				number[j++] = buffer[i++];
			}
			number[j++] = 0x00;
			if(number[0]>0)
			{
				tempVal = atof((const char*)number);
				yxGpsParam.gpsrspeedn[kind] = tempVal;
			}
			i += 3;
			j = 0;
			while((j < 19)&&(buffer[i] != ','))
			{
				number[j++] = buffer[i++];
			}
			number[j++] = 0x00;
			if(number[0]>0)
			{
				tempVal = atof((const char*)number);
				yxGpsParam.gpsrspeed[kind] = tempVal;
			}
			res |= YX_GPS_FLAG_SPEED;
			i += 3;
			if(buffer[i] == 'N')//invalid data
				res &= ~YX_GPS_FLAG_SPEED;
#if(YX_IS_TEST_VERSION!=0)
			sprintf(dblog,"res:%d,angle:%.2f,anglem:%.2f,speedN:%.4f,speedK:%.4f\r\n",res,yxGpsParam.gpsangle[kind],yxGpsParam.gpsanglem[kind],yxGpsParam.gpsrspeedn[kind],yxGpsParam.gpsrspeed[kind]);
			YxAppTestUartSendData((U8*)dblog,strlen(dblog));
#endif
			return res;
		}
		else if(strncmp((char*)buffer,"GSV",3)==0)//GSV
		{
#if(YX_IS_TEST_VERSION!=0)
			char    number[20];
			buffer += 4;
			while(j<2)
			{
				if(buffer[i] == ',')
					j++;
				i++;
				if(i>=len)
					break;
			}
			j = 0;
			while((j < 19)&&(buffer[i] != ','))
			{
				number[j++] = buffer[i++];
			}
			number[j++] = 0x00;
			if(number[0]>0)
			{
				int num = atoi((const char*)number);
				sprintf(dblog,"sat num:%d\r\n",num);
				YxAppTestUartSendData((U8*)dblog,strlen(dblog));
			}
#endif
			return 0;
		}
		else if(strncmp((char*)buffer,"VER",3)==0)//VER
		{
			return 0;
		}
	}
#if(YX_IS_TEST_VERSION!=0)
	else
		YxAppTestUartSendData((U8*)"gps error\r\n",11);
#endif
	return 0;
}

static U8 GpsAllDataProc(U8 *rxbuffer,U16 len)
{
	U16   i = 0;
	U8    res = 0;
	while(i < len)
	{
		if(rxbuffer[i] == '$')
		{
			U16    k = 0,j = i;
			while((i+1 < len)&&(rxbuffer[i] != 0x0D) && (rxbuffer[i+1] != 0x0A))
			{
				i++;
				k++;
			}
			if((rxbuffer[i] == 0x0D) && (rxbuffer[i+1] == 0x0A))
			{
				res |= GpsUartOneLineDataProc((U8*)(rxbuffer+j),k);
				i++;
			}			
		}
		i++;
	}
	return res;
}

static void GpsAutoSetDateTime(char isInvalid)
{
	U16     gyear = 0;
	U8      gmonth = 0,gday = 0,ghour = 0,gminute = 0,gsecond = 0;
	MYTIME  oldtime;
	GpsGetTimeValue(&ghour,&gminute,&gsecond);
	GpsGetDateValue(&gyear,&gmonth,&gday);
	if(!isInvalid)
		yxGpsParam.serverSetTime |= 0x02;
	if((yxGpsParam.serverSetTime&0x01)||(gyear < 2015)||(gyear == 2080)||(!gmonth)||(!gday))
		return;
	GetDateTime(&oldtime);
	if(((isInvalid)&&(oldtime.nYear > gyear))||(oldtime.nYear == gyear && gmonth <= oldtime.nMonth && gday < oldtime.nDay))
		return;
	if((oldtime.nYear != gyear) || (oldtime.nMonth!=gmonth) || (oldtime.nDay!=gday)||(oldtime.nHour != ghour)||(oldtime.nMin != gminute))
	{
		oldtime.nSec = gsecond;
		oldtime.nMin = gminute;
		oldtime.nHour = ghour;
		oldtime.nDay = gday;
		oldtime.nMonth = gmonth;
		oldtime.nYear = gyear;
		mmi_dt_set_dt((const MYTIME*)&oldtime, NULL, NULL);
		if(!isInvalid)
			yxGpsParam.serverSetTime |= 0x01;
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData((U8*)"set time\r\n",10);
#endif
		return;
	}
}

static char GpsUartReadData(U8 *buffer,U16 readlen)
{
	U8   ret = 0;
	if(readlen==0)
		return 0;
	ret = GpsAllDataProc(buffer,readlen);
	if(ret&YX_GPS_FLAG_DATE)
		GpsAutoSetDateTime(0);
	else if(ret&YX_GPS_FLAG_INVALID_DATE)
		GpsAutoSetDateTime(1);
	if((ret&YX_GPS_FLAG_LATITUDE)&&(ret&YX_GPS_FLAG_LONGITITUDE))//check efence
		YxAppCheckEfence(GpsGetLongitudeV(),GpsGetLatitudeV());
	if((ret&YX_GPS_FLAG_LATITUDE)&&(ret&YX_GPS_FLAG_LONGITITUDE)&&(ret&YX_GPS_FLAG_SPEED)&&(ret&YX_GPS_FLAG_DATE))//end get
		return 1;
	return 0;
}

void GpsTaskDealWithMessage(void)//gps main
{
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
	extern char YxAutoTestCheckNeedTest(void);
#endif
#endif
	U8    rbuffer[GPS_UART_READBUF_LEN];
	U16   rLen = YxAppUartReadData(rbuffer,GPS_UART_READBUF_LEN);
#ifdef __USE_YX_AUTO_TEST__
	char  oldFlag = yxGpsParam.gpsIsOk;
#endif
	YxAppUartRxFlush();
	if((yxGpsParam.isopen & YX_CAN_READ_DATA)==0)
		return;
	if(GpsUartReadData(rbuffer,rLen)==1)//end
	{
		if(yxGpsParam.isopen & YX_GETTING_SAMPLE_DATA)
			YxAppGpsControl(0);
	}
#ifdef __USE_YX_AUTO_TEST__
#ifndef WIN32
	if((YxAutoTestCheckNeedTest()==1)&&(oldFlag==0)&&(yxGpsParam.gpsIsOk==1))
#endif
	{
		YxAppGpsControl(0);
#ifdef IK188_BB
		YxAppWifiOpen();
#endif
	}
#endif
}

void GpsGetTimeValue(U8 *hour,U8 *minute,U8 *second)
{
	if((hour==NULL)||(minute==NULL)||(second==NULL))
		return;
	else
	{
		U8 i = 0,kind[4]={YX_GPS_KIND_GP,YX_GPS_KIND_BD,YX_GPS_KIND_GL,YX_GPS_KIND_GN};
		while(i<4)
		{
			if(yxGpsParam.gpshour[kind[i]]||yxGpsParam.gpsminute[kind[i]]||yxGpsParam.gpssecond[kind[i]])
				break;
			i++;
		}
		if(i>=4)
			i = 0;//use gps
		i = kind[i];
		*hour = (yxGpsParam.gpshour[i]+8) % 24;
		*minute = yxGpsParam.gpsminute[i];
		*second = yxGpsParam.gpssecond[i];
	}
	return;
}

char GpsGetDateValue(U16 *year,U8 *month,U8 *day)
{
	//convert to china date
	U8    dif = 8,kind[4] = {YX_GPS_KIND_GP,YX_GPS_KIND_BD,YX_GPS_KIND_GL,YX_GPS_KIND_GN},i = 0;
	if((year==NULL)||(month==NULL)||(day==NULL))
		return 0;
	while(i<4)
	{
		if(yxGpsParam.gpsyear[kind[i]])
			break;
		i++;
	}
	if(i>=4)
		i = 0;//use gps
	i = kind[i];
	*year = yxGpsParam.gpsyear[i] + 2000;
	*month = yxGpsParam.gpsmonth[i];
	*day = yxGpsParam.gpsday[i];
	if(yxGpsParam.gpshour[i] + dif > 24)
	{
		U8    endday = applib_dt_last_day_of_mon(yxGpsParam.gpsmonth[i],(*year));
		(*day)++;
		if((*day) > endday)
		{
			(*month)++;
			(*day) = 1;
			if((*month)>12)
			{
				(*year)++;
				(*month) = 1;
			}
		}
	}
	return 1;
}

double GpsGetLongitudeV(void)
{
	char  kind = 0;
	if(yxGpsParam.updateKind & YX_DATA_UPDATE_GN)
		kind = YX_GPS_KIND_GN;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_BD)
		kind = YX_GPS_KIND_BD;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_GL)
		kind = YX_GPS_KIND_GL;
	else
		kind = YX_GPS_KIND_GP;
	if(((yxGpsParam.gpstatus[kind]&YX_GPS_GSA_POS_FLAG)>>1)==1)
		return 0.0;
	return yxGpsParam.longitudev[kind];
}

double GpsGetLatitudeV(void)
{
	char  kind = 0;
	if(yxGpsParam.updateKind & YX_DATA_UPDATE_GN)
		kind = YX_GPS_KIND_GN;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_BD)
		kind = YX_GPS_KIND_BD;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_GL)
		kind = YX_GPS_KIND_GL;
	else
		kind = YX_GPS_KIND_GP;
	if(((yxGpsParam.gpstatus[kind]&YX_GPS_GSA_POS_FLAG)>>1)==1)
		return 0.0;
	return yxGpsParam.latitudev[kind];
}

double GpsGetHightV(void)
{
	char  kind = 0;
	if(yxGpsParam.updateKind & YX_DATA_UPDATE_GN)
		kind = YX_GPS_KIND_GN;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_BD)
		kind = YX_GPS_KIND_BD;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_GL)
		kind = YX_GPS_KIND_GL;
	else
		kind = YX_GPS_KIND_GP;
	if(((yxGpsParam.gpstatus[kind]&YX_GPS_GSA_POS_FLAG)>>1)==1)
		return 0.0;
	return yxGpsParam.hightv[kind];
}

double GpsGetSpeedV(void)//km/h
{
	char  kind = 0;
	if(yxGpsParam.updateKind & YX_DATA_UPDATE_GN)
		kind = YX_GPS_KIND_GN;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_BD)
		kind = YX_GPS_KIND_BD;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_GL)
		kind = YX_GPS_KIND_GL;
	else
		kind = YX_GPS_KIND_GP;
	if(((yxGpsParam.gpstatus[kind]&YX_GPS_GSA_POS_FLAG)>>1)==1)
		return 0.0;
	if(yxGpsParam.gpsrspeed[kind]==0.0)
		return ConvertSpeedKnotToKm(yxGpsParam.gpsrspeedn[kind]);
	return yxGpsParam.gpsrspeed[kind];
}

double GpsGetAngleV(void)//真北参考系
{
	char  kind = 0;
	if(yxGpsParam.updateKind & YX_DATA_UPDATE_GN)
		kind = YX_GPS_KIND_GN;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_BD)
		kind = YX_GPS_KIND_BD;
	else if(yxGpsParam.updateKind & YX_DATA_UPDATE_GL)
		kind = YX_GPS_KIND_GL;
	else
		kind = YX_GPS_KIND_GP;
	if(((yxGpsParam.gpstatus[kind]&YX_GPS_GSA_POS_FLAG)>>1)==1)
		return 0.0;
	//if(yxGpsParam.gpsangle[kind]==0.0)
	//	return yxGpsParam.gpsanglem[kind];
	return yxGpsParam.gpsangle[kind];
}

#if (YX_GPS_CONVERT_DM2DD==1)
static int GpsConvertddmm2dd(const char *ddmm, char *dd)
{  
    int lenSrc = strlen(ddmm)+1;
    int lenMm = 0;
    int flag = 1;
    char *pcMm;
    double dMm;
    int iMm;
	if(NULL == ddmm || NULL == dd)
        return -1;
    memcpy(dd,ddmm,lenSrc);
    pcMm = strstr(dd,".");
    if(pcMm == NULL)
    {
        pcMm = dd+strlen(dd)-2;
        iMm = atoi(pcMm);
        dMm = iMm /60.0;
    }
    else
    {
        if(pcMm - dd > 2)
            pcMm = pcMm - 2;
        else
        {
            pcMm = dd;
            flag = 0;
        }
        dMm = atof(pcMm);
        dMm /= 60.0;
    }
    sprintf(pcMm,"%lf",dMm);
    if(flag)
        strcpy(pcMm,pcMm+1);
    pcMm = strstr(dd,".");
    lenMm = strlen(pcMm);
    if(lenMm > (6+2))
        memset(pcMm+6+2,0,lenMm-6-2);
    return 1;
}
#endif

double ConvertJwDdmm2Dd(double ll)
{
	int ill;
	double left;
	ll /= 100;
	ill = (int)ll;
	left = ll - ill;
	left *= 100/60.0;
	return (ill + left);
}

double ConvertSpeedKnotToKm(double speed)
{
#define CONVERT_KNOT_TO_KM(knot)		(1.8519*knot)
	speed = CONVERT_KNOT_TO_KM(speed);
	return speed;
}

#if (YX_GPS_USE_AGPS==1)
#ifndef YX_AGPS_IC_NO_RESTART
static void YxAppGpsRestartTimer(void)
{
	yxGpsParam.isopen |= YX_CAN_READ_DATA;
	yxGpsParam.isopen |= YX_GETTING_SAMPLE_DATA;
	StopTimer(YX_GPS_TIMER_ID);
	StartTimer(YX_GPS_TIMER_ID,yxGpsParam.gpsTimerVal*1000,GpsGetDataTimeOut);
#if(YX_IS_TEST_VERSION!=0)
	YxAppTestUartSendData((U8*)"GPS restart\r\n",13);
#endif
}

static void YxAppGpsPowerOffTimer(void)
{
	YXAppGpsPowerCtrl(0);
	StopTimer(YX_GPS_TIMER_ID);
	StartTimer(YX_GPS_TIMER_ID,2000,YxAppGpsRestartTimer);
}
#endif
#endif

void YxAppGpsControl(char operation)
{
	StopTimer(YX_GPS_TIMER_ID);
	if(operation==1)
	{
#ifdef WIN32
		StartTimer(YX_GPS_TIMER_ID,1000,GpsGetDataTimeOut);
		return;
#endif
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData((U8*)"GPS data start\r\n",16);
#endif
		GpsResetValues();
		YxAppDisableSleep();
		yxGpsParam.isopen &= ~YX_CAN_READ_DATA;
		if(yxGpsParam.isopen==0)
		{
			YXAppGpsPowerCtrl(1);
			yxGpsParam.isopen = 1;
			YxAppGpsUartOpen();
			kal_sleep_task(44);
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData((U8*)"GPS Open\r\n",10);
#endif
		}
#if(YX_GPS_IS_MTK_CHIP==1)
		YxAppUartSendData((U8*)"$PMTK997,1*29\x0d\x0a",15);
		kal_sleep_task(2);
#endif
		if(yxGpsParam.noGpsCount>=YX_NOGPS_MAX_COUNT)
			yxGpsParam.gpsTimerVal = YX_GPS_INROOM_SEARCH_TIME;
		else if(yxGpsParam.gpsTimerVal == YX_GPS_INROOM_SEARCH_TIME)
			yxGpsParam.gpsTimerVal = YX_GPS_SAMPLE_DATA_TIME;
#if(YX_IS_TEST_VERSION!=0)
		if(yxGpsParam.gpsTimerVal == YX_GPS_INROOM_SEARCH_TIME)
			kal_prompt_trace(MOD_BT, "in room:%d\r\n",yxGpsParam.noGpsCount);
		else
			kal_prompt_trace(MOD_BT, "not in room:%d\r\n",yxGpsParam.noGpsCount);
#endif
		yxGpsParam.serverSetTime &= ~0x02;
#if(YX_GPS_USE_AGPS==1)
		if(yxGpsParam.yxAgpsDatLen>20)
		{
			YxAgpsDataWriteToIc(yxGpsParam.yxAgpsDataBuf,yxGpsParam.yxAgpsDatLen);
			yxGpsParam.yxAgpsDatLen = 0;
#ifdef YX_AGPS_IC_NO_RESTART
			yxGpsParam.isopen |= YX_CAN_READ_DATA;
			yxGpsParam.isopen |= YX_GETTING_SAMPLE_DATA;
			StartTimer(YX_GPS_TIMER_ID,yxGpsParam.gpsTimerVal*1000,GpsGetDataTimeOut);
#else
			StartTimer(YX_GPS_TIMER_ID,1500,YxAppGpsPowerOffTimer);
#endif
		}
		else
		{
#endif
		yxGpsParam.isopen |= YX_CAN_READ_DATA;
		yxGpsParam.isopen |= YX_GETTING_SAMPLE_DATA;
		StartTimer(YX_GPS_TIMER_ID,yxGpsParam.gpsTimerVal*1000,GpsGetDataTimeOut);
#if(YX_GPS_USE_AGPS==1)
		}
#endif
#if (YX_GPS_DISPLAY_UI==1)
		wgui_status_icon_bar_set_icon_display(STATUS_ICON_IR);
		wgui_status_icon_bar_update();
#endif
		return;
	}
	else//close
	{
		if(yxGpsParam.gpsCircle!=0xFFFF)
			yxGpsCurrentCircle++;
		if((yxGpsParam.gpsCircle!=0xFFFF)&&(yxGpsCurrentCircle>=yxGpsParam.gpsCircle))
		{
			yxGpsParam.isopen = 0;
			yxGpsCurrentCircle = 0;
#if(YX_GPS_IS_MTK_CHIP==1)
			YxAppUartSendData((U8*)"$PMTK997,0*28\x0d\x0a",15);
			kal_sleep_task(2);
#endif
#ifndef WIN32
			YXAppGpsPowerCtrl(0);
#if(YX_IS_TEST_VERSION!=0)
			YxAppTestUartSendData((U8*)"GPS Close\r\n",11);
#endif
#endif
	//		yxGpsParam.isopen &= ~YX_CAN_READ_DATA;
			YxAppGpsUartClose();
			YxAppEnableSleep();
#if (YX_GPS_DISPLAY_UI==1)
			wgui_status_icon_bar_reset_icon_display(STATUS_ICON_IR);
			wgui_status_icon_bar_update();
#endif
		}
#if(YX_IS_TEST_VERSION!=0)
		YxAppTestUartSendData((U8*)"GPS sample end\r\n",16);
#endif
		if(yxGpsParam.serverSetTime & 0x02)
		{
			yxGpsParam.serverSetTime &= ~0x02;
			yxGpsParam.noGpsCount = 0;
		}
		else
		{
			yxGpsParam.noGpsCount++;
			if(yxGpsParam.noGpsCount>=YX_NOGPS_MAX_COUNT)
				yxGpsParam.noGpsCount = YX_NOGPS_MAX_COUNT;
		}
		yxGpsParam.isopen &= ~YX_GETTING_SAMPLE_DATA;
		YxAppStepRunMain(YX_RUNKIND_GPS);
		return;
	}
}

static double YxAppGpsRad(double d)
{ 
   return d * PI / 180.0;   //角度1? = π / 180
} 

double YxAppGetGpsTwoPointDistance(double lat1, double long1, double lat2, double long2) //距离单位米
{
   double radLat1 = YxAppGpsRad(lat1);
   double radLat2 = YxAppGpsRad(lat2);
   double a = radLat1 - radLat2;
   double b = YxAppGpsRad(long1) - YxAppGpsRad(long2);
   double dst = 2 * asin((sqrt(pow(sin(a / 2), 2) + cos(radLat1) * cos(radLat2) * pow(sin(b / 2), 2))));
   dst *= YXAPP_EARTH_RADIUS;
   dst *= 1000;    /*1km=1000m*/
#ifdef __MTK_TARGET__
   dst = (lround(dst * 10000)) / 10000;
#endif
   return dst;
}

void YxAppSetGpsKind(U8 mode)
{
	if(mode>YX_GPS_KIND_GN)
		return;
	yxGpsParam.gpsKind = mode;
}

#if(YX_GPS_USE_AGPS==1)
void YxAgpsUpdateData(U8 *buffer,U16 length)
{
	if(length<=20||length>AGPS_MAX_RECEIVE_BUFFER_LEN||buffer==NULL)
		return;
	else
	{
		YxAppSaveAgpsData(buffer,length);
		memcpy((void*)yxGpsParam.yxAgpsDataBuf,(void*)buffer,length);
		yxGpsParam.yxAgpsDatLen = length;
		return;
	}
}

char YxAgpsGetDataTime(U8 *buffer)
{
	if(yxGpsParam.yxAgpsDataBuf[0]>='0' && yxGpsParam.yxAgpsDataBuf[0]<='9')
	{
		memcpy((void*)buffer,(void*)yxGpsParam.yxAgpsDataBuf,8);
		return 1;
	}
	return 0;
}
#endif

void YxAppGpsSetServerTimeStatus(void)
{
	yxGpsParam.serverSetTime |= 0x01;
}

char YxAppGpsIsInRoom(void)
{
	if(yxGpsParam.noGpsCount>=YX_NOGPS_MAX_COUNT)
		return 1;
	return 0;
}

void YxAppGpsInition(U8 defaultGps,U16 gpsTimes)
{
	memset((void*)&yxGpsParam,0,sizeof(YXGPSPARAMSTR));
	yxGpsParam.gpsKind = defaultGps;
	switch(defaultGps)
	{
	case YX_GPS_KIND_BD:
		yxGpsParam.updateKind = YX_DATA_UPDATE_BD;
		break;
	case YX_GPS_KIND_GP:
		yxGpsParam.updateKind = YX_DATA_UPDATE_GP;
		break;
	case YX_GPS_KIND_GL:
		yxGpsParam.updateKind = YX_DATA_UPDATE_GL;
		break;
	default:
		yxGpsParam.updateKind = YX_DATA_UPDATE_GN;
		break;
	}
	yxGpsParam.gpsCircle = gpsTimes;
	yxGpsCurrentCircle = 0;
#if (YX_GPS_USE_AGPS==1)
	yxGpsParam.yxAgpsDatLen = YxAppGetAgpsData(yxGpsParam.yxAgpsDataBuf,AGPS_MAX_RECEIVE_BUFFER_LEN);
	if(yxGpsParam.yxAgpsDatLen==0)
	{
		yxGpsParam.yxGpsStartTick = 1;
		yxGpsParam.gpsTimerVal = YX_GPS_SAMPLE_DATA_TIME;
		YXAppGpsPowerCtrl(1);
	}
	else
	{
		yxGpsParam.yxGpsStartTick = 0;
		yxGpsParam.gpsTimerVal = YX_GPS_SAMPLE_DATA_TIME;
		YXAppGpsPowerCtrl(0);
	}
#else
	yxGpsParam.gpsTimerVal = YX_GPS_SAMPLE_DATA_TIME;
	yxGpsParam.yxGpsStartTick = 1;
	YXAppGpsPowerCtrl(1);
#endif
}

void YxGpsCheckFiveMinuteTimeOut(void)
{
	if((yxGpsParam.yxGpsStartTick>0)&&(YxAppGetSystemTick(0)-yxGpsParam.yxGpsStartTick>=2))//开机后,在没有AGPS下,默认把GPS打开2分钟,让它找星
	{
		yxGpsParam.yxGpsStartTick = 0;
		YXAppGpsPowerCtrl(0);
		return;
	}
}

#ifdef __USE_YX_AUTO_TEST__
char YxGpsCheckIcIsOk(void)
{
	if(yxGpsParam.gpsIsOk == 1)
		return 1;
	return 0;
}
#endif
#endif
