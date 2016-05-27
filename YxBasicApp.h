#ifndef __YX_BASIC_APP__
#define __YX_BASIC_APP__

#ifdef __MMI_BT_SUPPORT__
#include "BtcmSrvGprot.h"
#endif
#include "SmsSrvGprot.h"
#include "Unicodexdcl.h"
#include "OslMemory_Int.h"
#include "app2soc_struct.h"

#ifdef __cplusplus
extern "C"
{
#endif
/***************************************************具体根据你们的需要修改相关宏**************************************************/
//debug
#define  YX_IS_TEST_VERSION          2               //是否是测试版本,0:量产版本,1:串口1调试并打印输出,2:USB调试并打印输出,USB打印时
                                                     //电脑端工具要用catcher,同时代码里custom\common\hal\nvram\nvram_data_items.c
                                                     //中static port_setting_struct const NVRAM_EF_PORT_SETTING_DEFAULT[]数组里,tst-ps uses uart_port_null(value is 99),99改为4即可
                                                     //在catcher的FILLTER里选只一个MOD即MOD_BT就可以看到我们的打印输出信息

//GPS串口配置
#define  YXAPP_USE_UART              uart_port2      //配置你要用的串口号,默认为uart2
#define  YX_GPS_UART_BAUDRATE        UART_BAUD_115200  //GPS默认的波特率,支持MT3337,UBOX-7020与U8,以及采用NMEA0183协议的GPS芯片,UART_BAUD_9600,UART_BAUD_115200,(MT3337:115200,ublox:9600)
#define  YX_GPS_IS_MTK_CHIP          1               //是否是MTK的GPS芯片,1:是,0:不是
#define  YXAPP_UART_TASK_OWNER_WAY   0               //配置你的串口属于那个TASK,0:MOD_YXAPP, 1:MOD_MMI
#define  YXAPP_GPS_KIND              0               //你的GPS芯片是否支持三种系统,如果只支持GPS请用0,如果三(GP,BD,GL)种都支持,就用3,3:GN多模定位,2:GL,1:只支持BD,0:只支持GPS
//注意GPS还有一个电源控制脚在驱动中,需要根据原理图修改一下,给它供电.API名称为:YXAppGpsPowerCtrl

//WIFI ESP8266EX uart config
#ifdef IK188_BB
#ifndef __MMI_WLAN_FEATURES__
#define  YXAPP_WIFI_UART             uart_port1
#endif
#define  YXAPP_WIFI_UART_BAUD        UART_BAUD_9600 //old version is UART_BAUD_9600,new verisn : UART_BAUD_115200
#define  YXAPP_WIFI_UART_MOD_WAY     0   //0:MOD_MMI         
#define  YXAPP_WIFI_AP_NUMBER        10
#define  YXAPP_WIFI_AP_TIME_OUT      50000
#endif
//服务器主机与端口,目前只支持TCP方式,HTTP即是TCP方式
#define  YX_NET_WAY                  MAPN_NET        //MAPN_NET,MAPN_WAP,上网方式,NET与WAP两种,区别WAP方式上网是通过移动或联通网关
	                                                 //10.0.0.172代理上网的,此方式只能连HTTP服务器,并且端口一定要为80,否则网关不会去联
                                                     //NET方式就不是通过代理上,是直接可以连接服务器的,没有端口限制
#define  YX_HOST_NAME_MAX_LEN        40              //域名最大字符数

#if 0//需要更改成你们的服务器IP与端口
#define  YX_DOMAIN_NAME_DEFAULT      "192.168.1.1" //服务器域名或是ip地址,如:192.186.1.1
#define  YX_SERVER_PORT              5088            //服务器port,如果是HTTP服务器,端口默认为80,如果是通过CMWAP方式上网,则只能用HTTP服务器,并且端口一定要为80
#else
#define  YX_DOMAIN_NAME_DEFAULT      "www.163.com" //服务器域名或是ip地址,如:192.186.1.1
#define  YX_SERVER_PORT              80            //服务器port,如果是HTTP服务器,端口默认为80,如果是通过CMWAP方式上网,则只能用HTTP服务器,并且端口一定要为80
#endif
#define  YX_CONN_SERVER_TIME_OUT     60000           //SOCKET read write time out,second
#define  YX_MIN_UPLOAD_TIME          1               //心跳包时间间隔,秒为单位,默认为10秒,系统秒的计时是以每10秒一个TICK
#define  YX_HEAT_TIME_MIN            1               //最小心跳包间隔
#define  YX_HEAT_TIME_MAX            10              //最大心跳包间隔
#define  YX_GPS_UPLOAD_TIME          20              //定位包上传间隔,分钟为单位,默认为20分钟
#define  YX_GPS_TIME_MIN             5               //定位包最小时间间隔
#define  YX_GPS_TIME_MAX             60              //定位包最大时间间隔
#define  YX_GPRS_RECONNECT_COUNT     3               //当与服务器通讯中断或没连接到服务器时，最大重新连接次数
#define  YX_GPS_CONVERT_DM2DD        1               //需要不需要把GPS的坐标DDMMMMMM转成DDDDDD,1:需要,0:不需要

#define  YX_GPS_DISPLAY_UI           0               //是否显示GPS图标,0:不显示
#define  YX_GPS_USE_AGPS             1               //是否支持AGPS,0:不支持
#define  YX_GSENSOR_SURPPORT         2               //GSENSOR,1:采用中断方式,2:TIMER方式,0:不支持
#define  YX_GSENSOR_DEFAULT_VALUE    0               //计步默认关闭,1:打开,0:关闭
#define  YX_PROTOCOL_KIND            1               //通讯协议种类,
#define  YX_NO_SIMCARD_GET_DATA      1               //没有插入SIM卡时,同样采集数据并保存起来,等有卡时,再上传到服务器,1:支持,0:不支持
#define  YX_SMS_CMD_DONT_USE         1               //不需要短信指令
#define  YX_SECOND_TICK_WAY          2               //秒的计时方式,1:为stack_timer,2:starttimer
#if(YX_GPS_USE_AGPS==1)
#define  YX_GPS_SAMPLE_DATA_TIME     60              //GPS采集坐标最长时间,30秒
#else
#define  YX_GPS_SAMPLE_DATA_TIME     60              //GPS采集坐标最长时间,50秒
#endif
#define  YX_HEART_TICK_UNIT          10000           /**心跳包每10秒计一次单位*/
#define  YX_USE_PROXIMITY_IC         1               //光感,0:不支持,1:支持
#define  YX_GPS_INROOM_SEARCH_TIME   38              //室内找GPS最大时间为40秒
#define  YX_NOGPS_MAX_COUNT          3               //找不到GPS最大次数
/***************************************************以下部分的宏定义不需要去改动**************************************************/

#define  YX_APP_MAX_CELL_INFOR_NUM   6               //最大基站数,MTK最大为6,不要更改
#define  YX_APP_MAX_SMS_CHARS        70              //一条短信最大字符数,不要更改
#define  YX_SOCK_BUFFER_LEN          2048            //从SOCKET中一次读取或发送的最大字节数
#define  YX_SOCK_READ_BUFFER_LEN     2048            

#define  YX_FILE_FOLD_NAME           "yxdat"         //根目录名称
#define  YX_EFENCE_MAX_NUM           3              //最大电子围栏个数
#define  YX_EFENCE_NAME_LENGTH       10             //最大电子围栏名称长度,(unicode)
#define  YX_MAX_MONITOR_NUM          4              //最大监护人名单,00:dady,1:monmy
#define  YX_WHITE_NUM_LIST_MAX       10             //白名单列表最大个数
#define  YX_LOGS_MAX_NUM             (YX_EFENCE_MAX_NUM+YX_MAX_MONITOR_NUM+2+2)
#define  YX_BT_MAC_BUFFER_LEN        18             //BT mac字符BUFFER长度,包括了结尾0
#define  YX_MAX_ALARM_NUM            3              //最大闹钟数
#define  YX_BIG_MEMERY_LEN           30*1024        //最大可收发网络数据的内存
#define  YX_UNSEND_PKG_HEADER_LEN    28             //过去包包头固定大小
#define  YX_UNSEND_PKG_FILE_LEN      (YX_BIG_MEMERY_LEN-YX_UNSEND_PKG_HEADER_LEN)      //最大未上传包文件大小
#if (YX_GPS_USE_AGPS==1)
#define  AGPS_MAX_RECEIVE_BUFFER_LEN   4*1204       //AGPS文件大小
#endif

#define  YX_SOCKET_TIMER_ID          JDD_TIMER_04

#define  YX_APN_USE_NEW_ONE
#define  YX_APP_DNS_REQUEST_ID		0x055A5121

#define  YX_APP_SIM1          1
#define  YX_APP_SIM2          2

#define  MAPN_WAP             0
#define  MAPN_NET             1
#define  MAPN_WIFI            2

#define MSIM_OPR_NONE         0
#define MSIM_OPR_CMCC         1
#define MSIM_OPR_UNICOM       2
#define MSIM_OPR_TELCOM       3
#define MSIM_OPR_WIFI         4
#define MSIM_OPR_UNKOWN       5

#define PF_INET                      SOC_PF_INET 
#define SOCK_STREAM                  SOC_SOCK_STREAM

//msg command kind    
#define  YX_MSG_BLUETOOTH_CTRL       1
#define  YX_MSG_WLAN_POWER_CTRL      2 //1:power on and scan,0:power off
#define  YX_MSG_LBS_CELL_CTRL        3 //1:get cell infors,0:cancel
#define  YX_MSG_CALL_CTRL            4 //1:make call,0 cancel
#define  YX_MSG_SMS_CTRL             5
#define  YX_MSG_GSENSOR_CTRL         6
#define  YX_MSG_MAINAPP_TICK_CTRL    7 //分钟TICK
#define  YX_MSG_MAINAPP_SECOND_CTRL  8 //秒钟TICK
#define  YX_MSG_START_POS_CTRL       9 //立即定位
#define  YX_MSG_ALL_PS_DEV_CTRL      11

//msg user data kind
//bt operation
#define  YX_DATA_BTSETNAME            1
#define  YX_DATA_BTSEARCH             2
#define  YX_DATA_BT_ABORT_SEARCH      3
#define  YX_DATA_BT_POWER_CTRL        4
#define  YX_DATA_BT_SET_VISIBILITY    5
//lbs operation
#define  YX_DATA_LBS_START            1
#define  YX_DATA_LBS_CANECL           0
//call operation
#define  YX_DATA_MAKE_CALL            1
#define  YX_DATA_REFUSE_INCALL        0
//sms operation
#define  YX_DATA_SEND_SMS             1
//gprs operation
#define  YX_DATA_GPRS_RECONNECT       1
//call type
#define  YX_CALL_NORMAL               0
#define  YX_CALL_LISTEN               1
#define  YX_CALL_CIRCYLE              2
#define  YX_CALL_SEND1                3
#define  YX_CALL_SEND2                4
//on/off
#define  YX_DEV_START                 1
#define  YX_DEV_STOP                  0

//run kind
#define  YX_RUNKIND_WIFI         0x0001
#define  YX_RUNKIND_LBS          0x0002
#define  YX_RUNKIND_GPS          0x0004
#define  YX_RUNKIND_GSENSOR      0x0008
#define  YX_RUNKIND_BT_SEARCH    0x0010
#define  YX_RUNKIND_OWNER_GPS    0x8000  //period upload data by timer
#define  YX_RUNKIND_OWNER_HEART  0x4000  //心跳包
//max 

#define  YX_APP_CALL_NUMBER_MAX_LENGTH   20
#define  YX_APP_CALL_NAMES_MAX_LENGTH    10

#define  YX_CHECK_MONITOR_INFOR(a,n) (!a || !a||(strlen((S8*)a)<5)||(strlen((S8*)a)>YX_APP_CALL_NUMBER_MAX_LENGTH)||(n[0]==0&&n[1]==0))

#ifdef __MMI_WLAN_FEATURES__
typedef struct
{
	kal_uint8               bssid[WNDRV_MAC_ADDRESS_LEN]; /* MAC address */
    kal_uint8               ssid[WNDRV_SSID_MAX_LEN+1]; 
	kal_int32               rssi;
}WLANMACPARAM;
#endif
#ifndef SRV_BT_CM_BD_FNAME_LEN
#define SRV_BT_CM_BD_FNAME_LEN 100
#endif
#ifdef __MMI_BT_SUPPORT__
#define  YXAPP_BT_MAX_SEARCH_DEVICES  8

typedef struct
{
	U8     name[SRV_BT_CM_BD_FNAME_LEN+1];
    U32    lap;    /* Lower Address Part 00..23 */
    U8     uap;    /* upper Address Part 24..31 */
    U16    nap;    /* Non-significant    32..47 */
}YXBTPARAM;
#endif

#ifdef __NBR_CELL_INFO__
typedef struct{
  //  U16    arfcn;           /*ARFCN*/
 //   U8     bsic;            /*BSIC*/
    U8     rxlev;           /*Received signal level*/
    U16    mcc;             /*MCC 国家代码 460*/
    U16    mnc;             /*MNC 运营商代码*/
    U16    lac;             /*LAC 位置码*/
    U16    ci;              /*CI 小区号*/
}yxapp_cell_info_struct;
typedef struct{
	char   isRunning;
	U8     cellNum;
	yxapp_cell_info_struct cellInfor[YX_APP_MAX_CELL_INFOR_NUM+1];//0:是当前所在小区的信息,后面6个为找到的小区
}YXAPPCELLPARAM;
#endif
typedef struct{
	S8     number[SRV_SMS_MAX_ADDR_LEN+1];
	S8     storeNumber[SRV_SMS_MAX_ADDR_LEN+1];
	U16    content[YX_APP_MAX_SMS_CHARS+1];
}YXAPPSMSPARAM;

#define    YX_NET_STATUS_CONN_HOST_BY_NAME        1
#define    YX_NET_STATUS_CONN_CONNECTTING         2
#define    YX_NET_STATUS_CONN_CONNECTED           3
#define    YX_NET_STATUS_CONN_STOP                4
#define    YX_NET_STATUS_CONN_TIME_OUT            5
typedef struct
{
    kal_uint8 address[4];   /* resolved IP address for queried domain name */
}YXSOCDNSASTRUCT;

typedef struct 
{
    kal_int8   sock;                    /**< socket 句柄*/
	S8         apn;
	U8         appid;
    U16        port;                    /**< socket 远程端口*/
    U8         socket_state;            /**< socket 链路的状态*/
    U32        account_id;
	S8         hostName[YX_HOST_NAME_MAX_LEN+1];               //保存的域名或IP
	soc_dns_a_struct  dnsIplist;
}YXAPPSOCKCONTEXT;

typedef struct
{
	U8         sendCount;              /*最大重发次数,为FF时,不需要重发*/
	U8         sendBuf[YX_SOCK_BUFFER_LEN];
	U8         readBuffer[YX_SOCK_READ_BUFFER_LEN];
	S32        sendLen;
	S32        curSendLen;
}YXAPPSOCKDATAPARAM;

typedef struct
{
	S8         number[YX_APP_CALL_NUMBER_MAX_LENGTH+1];//ascii
	U8         names[(YX_APP_CALL_NAMES_MAX_LENGTH+1)<<1];//unicode
}YXMONITORPARAM;

#define     YX_GPS_KIND_GP   0
#define     YX_GPS_KIND_BD   1
#define     YX_GPS_KIND_GL   2
#define     YX_GPS_KIND_GN   3

#define     YX_GPS_FLAG_LATITUDE     0x01
#define     YX_GPS_FLAG_LONGITITUDE  0x02
#define     YX_GPS_FLAG_SPEED        0x04
#define     YX_GPS_FLAG_DATE         0x08
#define     YX_GPS_FLAG_TIME         0x10
#define     YX_GPS_FLAG_INVALID_DATE 0x20

#define     YX_DATA_UPDATE_BD        0x01
#define     YX_DATA_UPDATE_GP        0x02
#define     YX_DATA_UPDATE_GL        0x04
#define     YX_DATA_UPDATE_GN        0x08

#define     YX_CAN_READ_DATA         0x02
#define     YX_GETTING_SAMPLE_DATA   0x04

#define     YX_GPS_GSA_POS_FLAG      0x06

//设备开启与关闭状态
#define     YX_DEVICE_BT_ON_FLAG     0x01
#define     YX_DEVICE_WIFI_ON_FLAG   0x02

typedef struct
{
	char    isopen;
#ifdef __USE_YX_AUTO_TEST__
	char    gpsIsOk; //for auto test
#endif
	char    serverSetTime;
	char    noGpsCount;
	U8      gpshour[4];
	U8      gpsminute[4];
	U8      gpssecond[4];
	U8      gpsyear[4];
	U8      gpsmonth[4];
	U8      gpsday[4];
	U8      gpsKind;         //所选用的GPS种类,0:BD,1GP,2:GN
	U8      updateKind;
	U8      gpstatus[4];    //卫星状态,GSA相关值,低3位是定位种类0:自动,1手动,定位类型,1未定位,2二维定位,3三维 高5位:用到的卫星数最大12颗
#if (YX_GPS_USE_AGPS==1)
	U8      yxAgpsDataBuf[AGPS_MAX_RECEIVE_BUFFER_LEN];
	U16     yxAgpsDatLen;
#endif
	U8      gpsTimerVal;     //GPS最大采集数据时间,YX_GPS_SAMPLE_DATA_TIME
	U8      yxGpsStartTick;
	U16     gpsCircle;      //GPS工作周期
	double  longitudev[4];   //经度,负数为西,0:BD,1:GP,2:GN
	double  latitudev[4];   //纬度,负数为南,0:BD,1:GP,2:GN
	double  hightv[4];       //高度,单位为米
	double  gpsangle[4];    //地面方位角真北参系,0:BD,1:GP,2:GN
	double  gpsrspeed[4];   //速度公里,0:BD,1:GP,2:GN
	double  gpsanglem[4];    //地面方位角磁北参系,0:BD,1:GP,2:GN
	double  gpsrspeedn[4];   //速度海里,0:BD,1:GP,2:GN
}YXGPSPARAMSTR;

typedef struct
{
	U8      lowBattery;    //低电量报警门槛,值只取低7位,第8位为是否发送了短信
	U8      gsensor;       //0:gsensor off,1:gsensor on
	//U8      lastBt[15];    //要找的BT设备地址,要转成16进制ASCII
	U8      heartNum;
	U8      imei[16];      //15 imei
	U8      deviceFlag;    //设备开启与关闭状态
	U8      allowOff;      //是否允许关机,1:ok,0:no
	U8      searchBt;      //是否要搜索周边的BT设备的MAC地址,上传到服务器上
	U16     uploadTick;    //上传数据间隔时间,分钟为单位
	U16     gpsTimes;      //GPS工作周期
	U16     systemTick;    //系统秒
	U16     minuteTick;    //系统分
#if(YX_GSENSOR_SURPPORT!=0)
	U16     todaySteps;
	U16     dateSteps;   //save step's date,hight 8bits is motth,low 8bits is day
#endif
}YXSYSTEMPARAM;

#define  YX_UPLOAD_KIND_HEART        0x01  //心跳包
#define  YX_UPLOAD_KIND_GPS          0x02  //定位包
#define  YX_UPLOAD_NEED_HEART        0x04
#define  YX_UPLOAD_NEED_GPS          0x08
#define  YX_UPLOAD_NEED_PS           0x10

typedef struct
{
	char    sleepdisable;
	char    uploadStart;
	char    idleRun;
	U8      sleephandle;
	U8      allIsReady;
	U8      heartTick;     //心跳包计时
	U8      gpsTick;       //定位包计时
}YXAPPPARAM;

#if(YX_GSENSOR_SURPPORT!=0)
typedef struct
{
	kal_uint8   stepReset;  //0:no reset,1:reset
	kal_uint8   eqcount;    //
	kal_uint8   timerCount; 
	kal_uint16  stepCount;
	kal_uint16  oldStep; 
	kal_uint16  lastOldStep;
	kal_uint16  stepDisplay;
	kal_uint16  sleepCount;
	kal_uint16  sleepStartStep;
#if(YX_GSENSOR_SURPPORT==2)
	kal_uint8   sample_period;
#else
	kal_uint8   isReady;
#endif
}YXGSENSORPARAM;
#endif
#if (YX_PROTOCOL_KIND==1)//IK protocol
#define  YX_APP_YES_SMS_LENGTH   ((YX_APP_MAX_SMS_CHARS+1)<<2)
typedef struct
{
	kal_uint8  sendOk;//0:no,1:ok
	kal_uint8  *bigBuffer;
	kal_uint8  isSos;
	kal_uint8  isFirst; /**是否为开机的第一次传,1:是*/
	kal_uint8  isYe;
	kal_uint8  yeSms[YX_APP_YES_SMS_LENGTH];
	kal_uint16 yeLength;
	kal_int32  sendLen;
	kal_int32  dataLen;
	kal_int32  totalLen;
}YXPROTOCOLPARAM;
#endif
//make call
typedef struct
{
	S8         number[YX_APP_CALL_NUMBER_MAX_LENGTH+1]; //用于监听与回拨电话
	kal_uint8  callKind;//0:normal call,1:listen call,2:circle call
	kal_uint8  callIndex;
	YXMONITORPARAM yxMonitors[YX_MAX_MONITOR_NUM];
}YXCALLPARAM;
//firewall
typedef struct
{
	kal_uint8  status;//on/off,0:off,1:on
	kal_uint16 startTime[3];//hour|min
	kal_uint16 endTime[3];//hour|min
	YXMONITORPARAM yxWhiteList[YX_WHITE_NUM_LIST_MAX];
}YXFIREWALLPARAM;
//efence
typedef struct
{
	kal_uint8  names[(YX_EFENCE_NAME_LENGTH+1)<<1];
	kal_uint8  kind;//0:out,1:in,2:both
	kal_uint8  lastKind;
	double     log;
	double     lat;
	double     radius;//meter
}YXEFENCEPARAM;
//logs
#define     LOG_EFENCE     '1'  //此LOG会有多个,action:'0':out,'1':in
#define     LOG_LOWBAT     '2'  //此LOG只有一个,以最新的一个为准
#define     LOG_PHONE      '3'  //开关机,action:'1':开,'0':关,此LOG只有一个,以最新的一个为准
#define     LOG_CALL       '4'  //电话记录,此LOG只有一个,以最新的一个为准,ACT:0 out call,1:in call
#define     LOG_OFFHANDS   '5'  //防脱报警,action:'0'取下,'1'戴上

typedef struct
{
	kal_int8     kind;//0:no,1:efence,2:lowbattery,3:listennig,4:mobile
	kal_uint8    sent;//0:no,1:sending,2:sent it out
	kal_uint16   date;//month|day
	kal_uint16   time;//hour|min
	kal_uint8    action;
	kal_uint16   value;//efence的序号,or,低电量的值or语音监听的号码值,固定为4位,不足补0
}YXLOGPARAM;

#ifdef YXAPP_WIFI_UART
#define   WIFI_AP_NAME_LEN     50
#define   WIFI_MAC_STRING_LEN  17
#define   WIFI_RF_STR_LEN      3
typedef struct
{
	S8    apName[WIFI_AP_NAME_LEN+1];
	S8    macString[WIFI_MAC_STRING_LEN+1];
	S8    rfStr[WIFI_RF_STR_LEN+1];
}YXWIFIAPPARAM;
typedef struct
{
	kal_uint8    wifiReady;
	kal_uint8    cmdIdx;
	YXWIFIAPPARAM  apList[YXAPP_WIFI_AP_NUMBER];
}YXWIFIPARAM;
#endif
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//basic api
extern void *YxAppMemMalloc(kal_int32 size);
extern void YxAppMemFree(void *pptr);
extern void YxAppSendMsgToMMIMod(U8 command,U8 param1,U8 param2);
extern void YxAppDisableSleep(void);
extern void YxAppEnableSleep(void);
extern char YxAppGetSimOperator(char simId);
extern void YxAppGetImeiNumber(void);
extern S8 *YxAppGetImeiCodeString(void);
extern U8 YxAppGetSignalLevelInPercent(void);
extern U8 YxAppBatteryLevel(void);
extern void YxAppGetSystemDateTimeStr(char *strBuf);//buf len need 20
extern S32 YxAppUCSStrlen(U16 *string);
extern void YxAppMMiRegisterMsg(void);

//gps uart api
extern U16 YxAppUartReadData(U8 *buff,U16 Maxlen);
extern U16 YxAppUartSendData(U8 *buff,U16 len);
extern void YxAppUartTxFlush(void);
extern void YxAppUartRxFlush(void);
#if(YX_IS_TEST_VERSION!=0)
extern void YxAppTestUartSendData(U8 *dataBuf,U16 dataLen);
#endif
extern void YxAppUartSetBaudrate(UART_baudrate baudrate);
extern void YxAppGpsUartOpen(void);
extern void YxAppGpsUartClose(void);
extern void YXAppGpsPowerCtrl(char on);//driver power ctrl,1,power on,0,power off
#if(YXAPP_UART_TASK_OWNER_WAY==1)||defined(YXAPP_WIFI_UART)
extern void YxAppRegisterUartMsgToMMiTask(void);//if gps uart is uart1,need use this
#endif

#ifdef __MMI_WLAN_FEATURES__
extern void YxAppWlanPowerScan(char scan);
extern WLANMACPARAM *YxAppGetWlanScanInfor(U8 index);
extern U8 *YxAppSetAndGetLocalMac(U8 *macBuf);
#endif

//wifi uart
#ifdef YXAPP_WIFI_UART
extern void YxAppWifiPowerCtrl(char on);
extern void YxAppWifiOpen(void);
extern void YxAppWifiClose(void);
extern void YxAppWifiUartTxFlush(void);
extern void YxAppWifiUartRxFlush(void);
extern U16 YxAppWifiUartSendData(U8 *buff,U16 len);
extern U16 YxAppWifiUartReadData(U8 *buff,U16 Maxlen);
extern YXWIFIAPPARAM *YxAppGetWlanApListInfor(U8 index);
#endif

//proximity
#if(YX_USE_PROXIMITY_IC==1)
extern void YxPsAlsInition(void);
extern kal_uint8 YxPsReadProximityValue(void);
extern char YxPsCheckStatus(void);
extern void YxAppPsPollingByMinuteTick(void);
#endif
//gsensor
extern char YxAppGsensorStatus(void);
#if(YX_GSENSOR_SURPPORT!=0)
extern void YxAppGsensorPowerControl(kal_int8 on);
extern char YxAppGsensorSampleData(kal_int16 *x_adc, kal_int16 *y_adc, kal_int16 *z_adc);//in driver
extern void YxAppGsensorInition(void);//in driver
//mmi
extern void YxAppGsensorProcInit(void);
extern void YxAppGsensorPedometerStart(U8 start);
extern void YxAppGsensorCheckCurrentTime(U8 hour,U8 min);
extern U16 YxAppGsensorGetStep(char sleep);
extern void IdleShowPedometerSteps(char clearbox);
extern void YxAppGsensorIdleTimerCheck(char start);
extern void YxAppMinTickCheckSteps(void);
extern void YxAppSaveTodaySteps(void);
extern void YxAppStepRestartByScreenLightOn(char kind);
#endif

#ifdef __MMI_BT_SUPPORT__
extern void YxAppBTPowerControl(char on);
extern char YxAppGetBtStatus(void);
extern char YxAppGetLocalBtMac(U8 *nameBuf,U8 *addressBuf);
extern void YxAppSetBtDeviceName(char *nameBuf);
extern void YxAppGetLocalBtNameMac(PU8 *name,PU8 *mac);
extern void YxAppBtSearchDevices(void);
extern void YxAppBtAbortSearch(void);
extern void YxAppBtSetNotify(U32 event_mask);
extern void YxAppBtSetVisibility(char on);
extern char YxAppBtGetDevicesAddress(U8 index,char *addressStr);//buffer is 15
#endif
#ifdef __NBR_CELL_INFO__
extern void YxAppCellRegReq(void);
extern void YxAppCellDeregReq(void);
extern yxapp_cell_info_struct *YxAppLbsGetData(char idx);
extern U8 YxAppGetLBSInfor(char *lineBuffer,char idx);//lineBuffer:min length is 25
#endif

//call apis
extern void YxAppCircyleCallInit(void);
extern void YxAppCallStopCircyle(void);
extern S8 YxAppMakeWithCallType(S8 callType,S8 *number);
//extern void YxAppEndIncommingCall(void);
extern void YxAppEndCallCb(void);
extern void YxAppCallEndProc(void);
extern void YxAppCallSetAudioMute(char onoff);
//sms apis
extern void YxAppSmsCheckDateTime(MYTIME *timeStamp);
extern void YxAppSetSmsNumber(S8 *number,U8 length);
extern void YxAppSendSms(S8* number, U16* content,char gsmCode);//号码为ASCII码,内容则要为UNICODE码,低字节在前,高字节在后,最大长度为YX_APP_MAX_SMS_CHARS,gsmCode:此为发送编码格式为GSM7,为0时,则为SRV_SMS_DCS_UCS2
//gprs apis
extern U32 YxAppDtcntMakeDataAcctId(char simId,char *apnName,char apnType,U8 *appId);
extern void YxAppUploadDataFinishCb(void);
extern S32 YxappAddOneApn(char simId,const U8 *apnName,const U8 *accountName,char *userName,char *password);

//socket apis
extern U16 YxAppGetHostNameAndPort(S8 **hostName);
extern S8 YxAppTcpConnectToServer(const S8* host, U16 port, S8 apn);//apn:only:MAPN_WAP,MAPN_NET
extern S32 YxAppTcpSockRead(U8 *buf,S32 len);
extern S32 YxAppTcpSockWrite(U8 *buf, S32 len);
extern void YxAppCloseSocket(char reconnect);//reconnect,为1:需要重新连接,为0:不需要,直接关闭
extern S8 YxAppConnectToMyServer(void);
extern void YxAppRestartServerTimer(void);
extern void YxAppReconnectServer(void);
extern void YxAppCloseNetwork(void);
extern S8 *YxAppGetServerName(U16 *port);
//file apis
extern void YxAppSetServerParams(char *host,U16 port);//保存服务器有关参数
extern YXMONITORPARAM *YxAppGetMonitorList(U8 index,S8 loadFisrt);//获取监护者信息
extern void YxAppSaveMonitorList(void);//保存取监护者信息
extern char YxAppSetMonitorInfor(U8 idx,S8 *number,U8 *names);
extern char YxAppGetMonitorNameByCallNumber(S8 *number,U8 *nameBuf);
extern void YxAppPrintToFile(U8 *buffer,U16 length);
extern S8 *YxAppGetCallNumberBuffer(void);
//white name list
extern YXMONITORPARAM *YxAppGetFirewallItemBy(U8 idx);
extern char YxAppGetFirewallNameByCallNumber(S8 *number,U8 *nameBuf);
extern char YxAppSetFirewallInfor(U8 idx,S8 *number,U8 *names);
extern void YxAppSaveFirewallList(void);
extern U16 YxAppSearchNumberFromList(S8 *number);
extern U8 YxAppGetNumberItem(U8 index,PU8 name,PU8 usc2_number);
//firewall
extern U8 YxAppFirewallReadWriteStatus(U8 status);
extern char YxAppFirewallReadWriteTime(char read,U16 *startTime,U16 *endTime);
extern YXSYSTEMPARAM *YxAppGetSystemParam(S8 loadFile);
extern void YxAppSaveSystemSettings(void);
extern char YxAppCheckTimeIsHiddenTime(void);
//gps apis
extern void YxAppGpsControl(char operation);
extern void YxAppSetGpsKind(U8 mode);
extern void YxGpsCheckFiveMinuteTimeOut(void);
extern void YxAppGpsSetServerTimeStatus(void);
extern char YxAppGpsIsInRoom(void);
extern void YxAppGpsInition(U8 defaultGps,U16 gpsTimes);
extern void GpsGetTimeValue(U8 *hour,U8 *minute,U8 *second);
extern char GpsGetDateValue(U16 *year,U8 *month,U8 *day);
extern double ConvertJwDdmm2Dd(double ll); //经纬度转成地球常用坐标点
extern double ConvertSpeedKnotToKm(double speed);//海里转换成公里
extern void GpsTaskDealWithMessage(void);//处理从GPS芯片读到的数据
extern double GpsGetLongitudeV(void);
extern double GpsGetLatitudeV(void);
extern double GpsGetHightV(void);
extern double GpsGetSpeedV(void);
extern double GpsGetAngleV(void);
#if (YX_GPS_USE_AGPS==1)
extern void YxAgpsUpdateData(U8 *buffer,U16 length);
extern char YxAgpsDataWriteToIc(U8 *buffer,U16 DataLen);
extern char YxAgpsGetDataTime(U8 *buffer);
//files
extern U16 YxAppGetAgpsData(kal_uint8 *data,U16 maxLen);
extern void YxAppSaveAgpsData(kal_uint8 *data,U16 dataLen);
#endif
extern double YxAppGetGpsTwoPointDistance(double lat1, double long1, double lat2, double long2);
//profile
extern char YxAppGetCurrentProfile(void);
extern void YxAppSetCurrentProfile(char value);
//battery low shreshold
extern char YxAppSystemReadWriteLowBattery(char read,char value);
extern void YxAppCheckLowBatteryShreshold(U8 percent);
//efence
extern U8 YxAppGetEfenceNumber(void);
extern void YxAppCheckEfence(double log,double lat);
extern char YxAppEfenceUpdate(kal_uint8 i,kal_uint8 *eNames,kal_uint8 kind,double lon,double lat,double radius);
extern void YxAppEfenceSaveData(void);
//logs
extern void YxAppLogAdd(U8 kind,U8 action,U16 value);
extern YXLOGPARAM *YxAppGetLogByIndex(char index);
extern void YxAppSetLogSentStatus(void);
extern void YxAppClearAllLogs(void);
extern void YxAppSaveLogData(void);
//alarms
extern U8 YxAppGetActiveAlarmNumber(void);
extern void YxAppSetAlarm(U8 index,U8 hour,U8 min,S8 status,S8 freq,U8 days);
//date time
extern char YxAppSystemDtIsDefault(void);
extern void YxAppSetTimeFormat(char kind);
//shutdown
extern void YxAppSetAllowShutdown(void);
extern void YxAppRunShutdownCmd(void);
extern void YxAppAutoShutdown(void);
//find watch
extern void YxAppSearchWtPlayTone(void);
extern void YxAppCloseSearchWtTone(void);
//backlight
extern void YxAppSetBacklightTime(char kind);
//heart numbers
extern U8 YxAppGetHeartNumbers(void);
//imei
extern void SSCHandleIMEI(void);
extern void YxAppImeiKeyPress(void);
//all devices
extern void YxAppStartAllPsDevices(void);
extern void YxAppAllPsDevicesStartContrl(void);

//app
extern U16 YxAppUTF8ToUincode(U8 *inbuf,U16 lenin,U8 *outbuf,U16 lenout);
extern U16 YxAppUincodeToUTF8(U8 *inbuf,U16 lenin,U8 *outbuf,U16 lenout);
extern U16 YxAppGetRunFlag(char kind);
extern void YxAppStepRunMain(U16 runKind);
extern void YxAppInitionParam(void);

//data proc callback
extern U16 YxAppBuildTxtCmdGpsUploadPackage(char savepkg,U8 *sendBuf,U16 maxLen);
extern U16 YxAppBuildTxtCmdHeartUploadPackage(U8 *sendBuf,U16 maxLen);
extern S32 YxAppSockConnectedCallback(U8 *sendBuf,U16 maxLen);
extern S32 YxAppSockReadDataCallback(U8 *readBuf,U16 readLen,U8 *sendBuffer);
extern void YxAppSaveUnsendTxtcmdPackage(void);
extern void YxAppDeleteUnsendPkgFile(void);
extern S32 YxAppGetUnsendDataLength(void);
extern S32 YxAppGetUnsendDataFromFile(U8 *buffer,S32 maxLen,S32 offset);

//timer tick
extern void YxAppUploadDataByTickProc(U8 hour,U8 min);
extern void YxAppStartPosAtOnceProc(char kind);
extern void YxAppSecondTickProc(void);
extern void YxAppStartSecondTimer(char start);
extern void YxAppHeartTickProc(void);
//app main
extern void YxAppUploadHeartProc(void);
extern void YxAppUploadGpsProc(void);
extern void YxAppSetOntimeCallGpsRunFlag(void);
extern U16 YxAppGetSystemTick(char second);
extern void YxAppEndAllAction(void);//关机调用
#if (YX_PROTOCOL_KIND==1)
extern void YxProtocolInition(void);
extern void YxProtocolSetSosStatus(void);
extern PU8 YxProtocolGetBigBuffer(void);
#endif
extern void YxAppIdleInition(void);
extern void YxAppIdleRunFunctions(void);

#ifdef __cplusplus
}
#endif
#endif