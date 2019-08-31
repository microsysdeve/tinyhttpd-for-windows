#ifndef		_argv_analyse_HPP
#define		_argv_analyse_HPP

#include "stdafx.h"
#include "windows.h"
#include "stdlib.h"
#include <time.h>		//time_t time()  clock_t clock()
#include <Mmsystem.h>		//timeGetTime()
#include "Getopt.h"
#include <string.h>
#include "sversion.hpp"
#include "modbusframe.hpp"

extern struct STCONFIG stconfig;
extern const char sDebugHead[];




struct STCONFIG
{
	struct STMODBUS_FRAME  stmodbusframs;
  char cParivaty;		//       
  char cDebugSet;
  char cComPortNum;
  char cStopbit;
  char cTimeshow;		//              timeshow
  char cReadFile;		//       读出文件
  unsigned char cDec;
  unsigned char cUprogUnitRetry;		//       下载程序重试次数
  unsigned char cBinFormat;		//      
  unsigned char cLineFormatLen;
  //char cStation;		//       站号
//char cFun;			//       操作类型,
//char cComType;		//      ASC ,RTU 
//char cOper;			//       置位，复位
  //unsigned char cUnitlen;
  //unsigned short iAddr;		//        操作地址
   //unsigned int lDataByteLen;	//       用于指示寄存器的数目
  //char sData[1024];
  unsigned int iBps;		//      BPS 
  unsigned int iReceTimeout;	//
  unsigned int iUpProgAddr;  
  char sSend[512];
  char sRece[512];
  char *sInifilename;		//      配置文件名
  char sParivaty[16];
  FILE *fpin;
  FILE *fpout;
  FILE *fpinifile;
  FILE *fuprog;			//       写入程序  
};

enum ENUMMODBUSOPER
{
  _modbus_fun_1_ = 1,
  _modbus_fun_5_ = 5,
};

enum ENUMBITCONFIG
{
  _bitSet_ = 9,
  _bitRes_,

};
enum ENUMERRORLIST
{
  _Error_no_ = 0,
  _Error_nohelpexit_,
  _Error_argv_input_error_,
  _Error_comno_input_,
  _Error_addr_input_,
  _Error_len_input_,
  _Error_station_input_,
  _Error_data_input_,
  _Error_infile_open_,
  _Error_outfile_open_,
  _Error_inifile_open_,
  _Error_upfile_open_,
  _Error_upfile_comerror_,	//      
  _Error_rece_noanswer_,	//       接收帧校验错误
  _Error_receframe_checkerror_,	//       接收帧校验错误
  _Error_receapp_checkerror_,	//          接收帧应用错误
  _Error_receapp_errorMax_,	//    接收帧应用错误
  _Error_makeframe_error_,	//       
  _Error_end_,
};

struct STERRORMESSAGE
{
  enum ENUMERRORLIST err;
  char cExit;
  char *message;
};

int inifile_analyse (char *sIniFileName);
int debuginfo_out (struct STCONFIG *stp);

#define  _iGetPercent(curr,total) (curr * 100 / total)
#define  showpercent(curr,total)  { unsigned short  itemp ; itemp =_iGetPercent(curr,total);  if ( (unsigned short)iPerCent !=(unsigned short)itemp)	{	iPerCent =itemp ; ;fprintf(stdout, "==%d==\n", iPerCent);}}
int argv_analyse (int argc, char **sargv);
struct STERRORMESSAGE *errormessage_get (int ino);
int error_fun (struct STERRORMESSAGE *stp);
#endif
