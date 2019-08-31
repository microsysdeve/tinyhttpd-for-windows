
#include "stdafx.h"
#include "windows.h"
#include "stdlib.h"
#include <time.h>	 
#include <Mmsystem.h>		//timeGetTime()
//#include "modbusframe.hpp"
#include <string.h>
#include "Getopt.h"
#include "argv_analyse.h"
#include "sversion.hpp"
#include  "messageout.h"
#include "modbusframe.hpp"
#include "modbusfuncode.hpp"
#include "modbus_outmessage.hpp"
#include "modbuscmdlist.h"

struct STMODBUSCOMLIST *sGlobWorkp = NULL;

const char sDebugHead[] = "Function argv_analyse";

const wchar_t *swAppName = L"COMSET";
const wchar_t *swCOMPORT = L"COMPORT";
const wchar_t *swBPS = L"BPS";
const wchar_t *swPARITY = L"PARITY";
const wchar_t *swSTOPBIT = L"STOPBIT";
const wchar_t *swSENDTYPE = L"SENDTYPE";
const wchar_t *swDEBUG = L"DEBUG";
const wchar_t *swBYTESIZE = L"BYTESIZ";
const wchar_t *swTIMESHOW = L"TIMESHOW";
const wchar_t *swRECETIMEOUT = L"RECETIMEOUT";
const wchar_t *swSTATION = L"STATION";

 

extern enum ENUMERRORLIST enError;
int ascbuf2hex (char *sbuf, int ilen, char *desp, int deslen);

void
stconfig_init (struct STCONFIG *stp)
{
  memset ((char *) stp, 0, sizeof (struct STCONFIG));
  stp->cDec = 1;
  stp->cReadFile = 0;
  stp->stmodbusframs.cStation = 1;		//       站号
  stp->cDebugSet = 0;
  stp->iBps = 9600;		//      BPS
  stp->stmodbusframs.lDataByteLen = 0;
  stp->cUprogUnitRetry = 2;
  stp->stmodbusframs.cUnitlen = 0xf0;
  stp->iReceTimeout = 500;
  stp->cLineFormatLen = 0x40;
  strcpy (stp->sParivaty, "NOPARITY");
  stp->cBinFormat = 0;
}

#define		_fileget( des ,sou, fp , err,openmode)   {   strncpy(des, sou, sizeof (des));fp = fopen(des, openmode);  \
if (NULL == fp) enError = err;  		debuglog("open statu(%s) fileinname=%s\n", (NULL == fp) ? "error" : "ok", des); }

#define		_rfileget( des ,sou, fp , err) { _fileget ( des ,sou, fp , err,"rb") ;if (fp) {fseek(fp,0,0) ;}}
#define		_wfileget( des ,sou, fp , err) _fileget ( des ,sou, fp , err,"wb")

int
Message_Help_Show (void)
{
  showversion ();
  showlog ("This is a modbus communication program \n");
  showlog ("-AXXX			modbus reg addr\n");
  showlog
    ("-BXXX			comport bps(1200,2400,4800,9600,19200,28800,38400,57600),Default:9600\n");
  showlog ("-CXXX			modbus CMD\n");
  showlog ("                               %s==%s\n", stmodbuscomlist_get (0),
	   stmodbuscomlist_getexplain (0));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (1),
	   stmodbuscomlist_getexplain (1));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (2),
	   stmodbuscomlist_getexplain (2));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (3),
	   stmodbuscomlist_getexplain (3));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (4),
	   stmodbuscomlist_getexplain (4));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (5),
	   stmodbuscomlist_getexplain (5));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (6),
	   stmodbuscomlist_getexplain (6));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (7),
	   stmodbuscomlist_getexplain (7));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (8),
	   stmodbuscomlist_getexplain (8));
  showlog ("                               %s==%s\n", stmodbuscomlist_get (9),
	   stmodbuscomlist_getexplain (9));
  showlog ("                               %s==%s\n",
	   stmodbuscomlist_get (10), stmodbuscomlist_getexplain (10));
  showlog ("                               %s==%s\n",
	   stmodbuscomlist_get (11), stmodbuscomlist_getexplain (11));
  showlog ("                               %s==%s\n",
	   stmodbuscomlist_get (12), stmodbuscomlist_getexplain (12));
  showlog ("                               %s==%s\n",
	   stmodbuscomlist_get (13), stmodbuscomlist_getexplain (13));
  showlog ("-DXXXX			writedata,HEX no space\n");
  showlog
    ("-EXXX			comport  parivaty( NOPARITY,ODDPARITY,EVENPARITY,MARKPARITY,SPACEPARITY),Default:NOPARITY\n");
  showlog ("-F			modbus reg addr NO auto dec 1,Default:1\n");
  showlog ("-GXXX			modbus station(1--255),Default:1\n");
  showlog ("-H			Printf this message\n");
  showlog ("-IXXXX.XXX		input file\n");
  showlog ("-JXXXX.XXX		upfilename\n");
  showlog ("-K			read file must set\n");
  showlog ("-LXXXX			RegDataLen\n");
  showlog ("-MX			ComPortNum(1...10)\n");
  showlog ("-NXXXX.XXX		inifilename\n");
  showlog ("-OXXXX.XXX		outputfilename\n");
  showlog ("-PXX			comtimeoutset(MS),Default:500\n");
  showlog ("-Q			out file format(BIN,TEXT),Default:TEXT\n");
  showlog ("-UXXXX.XXX		upfilename\n");
  showlog ("-R			Bit Res\n");
  showlog ("-S			Bit Set\n");
  showlog ("-YXX			Com fail retry num,Default:2\n");
  showlog ("-Z			debug info output\n");
  return 0;
}

int
argv_analyse (int argc, char **sargv)
{
  extern char *gOptArg;		/* argument associated with option */
  extern char *gOptPlace;	/* saved position in an arg */
  struct STCONFIG *stp = &stconfig;
  int opt;
  char stemp[1024];

  if (1 == argc)
    {
      enError = _Error_argv_input_error_;
      return enError;
    }

  GetoptReset ();
  enError = _Error_no_;
  stconfig_init (stp);

  while ((opt =
	  Getopt (argc, sargv,
		  "A:B:C:D:E:G:FHI:J:KL:M:N:O:P:QR:S:U:T:Y:Z")) >= 0)
    {
      //  if (strchr("aiup:rd:g:cmCfGRn:z", opt) == NULL)
      if (stp->fpinifile)
	break;
      {
	switch (opt)
	  {
	  case 'A':
	    stp->stmodbusframs.iAddr = atoi (gOptArg);
	    debuglog ("Addr=%d\n", stp->stmodbusframs.iAddr);
	    if (((stp->stmodbusframs.iAddr) > 65535) || (stp->stmodbusframs.iAddr < 0))
	      enError = _Error_addr_input_;
	    break;

	  case 'B':
	    stp->iBps = atoi (gOptArg);
	    debuglog ("bps=%d\n", stp->iBps);
	    break;

	  case 'C':
	    sGlobWorkp = Modbus_Cmdlist_NameText_Search (gOptArg);
	    if (sGlobWorkp)
	      stp->stmodbusframs.cFun = sGlobWorkp->cFun;
	    debuglog ("cFun=%d(%s)\n", stp->stmodbusframs.cFun, sGlobWorkp->sNameExplain);
	    break;

	  case 'D':
	    strcpy (stemp, gOptArg);
	    stp->stmodbusframs.lDataByteLen =
	      ascbuf2hex (gOptArg, strlen (gOptArg), stp->stmodbusframs.sData, stp->stmodbusframs.sDatalen);
	    break;

	  case 'E':
	    strncpy (stp->sParivaty, gOptArg, sizeof (stp->sParivaty));
	    debuglog ("sParivaty=%s\n", stp->sParivaty);
	    break;

	  case 'F':
	    stp->cDec = 0;
	    debuglog ("cDec=%d\n", stp->cDec);
	    break;

	  case 'G':
	    stp->stmodbusframs.cStation = atoi (gOptArg);
	    debuglog ("Station = %d\n", stp->stmodbusframs.cStation);
	    break;

	  case 'H':
	    Message_Help_Show ();
	    enError = _Error_nohelpexit_;
	    return enError;

	  case 'I':
	    _rfileget (stemp, gOptArg, stp->fpin, _Error_infile_open_);
	    break;

	  case 'J':
	    stp->iUpProgAddr = atoi (gOptArg);
	    debuglog ("UpProgAddr=%d\n", stp->iUpProgAddr);
	    break;

	  case 'K':
	    stp->cReadFile = 1;
	    debuglog ("cReadFile=%d\n", stp->cReadFile);
	    break;

	  case 'L':
	    stp->stmodbusframs.lDataByteLen = atol (gOptArg) * 2;
	    break;

	  case 'M':
	    stp->cComPortNum = atoi (gOptArg);
	    debuglog ("ComPortNum=%d\n", stp->cComPortNum);
	    if (((stp->cComPortNum) > 205) || (stp->stmodbusframs.iAddr < 0))
	      enError = _Error_comno_input_;
	    break;

	  case 'N':
	    stp->sInifilename = gOptArg;
	    break;

	  case 'O':
	    _wfileget (stemp, gOptArg, stp->fpout, _Error_outfile_open_);
	    break;

	  case 'P':
	    stp->iReceTimeout = atoi (gOptArg);
	    debuglog ("iReceTimeout = %d\n", stp->iReceTimeout);
	    if (((stp->iReceTimeout) > 65535) || (stp->iReceTimeout < 0))
	      enError = _Error_len_input_;
	    break;

	  case 'Q':
	    stp->cBinFormat = 1;
	    break;

	  case 'U':
	    _rfileget (stemp, gOptArg, stp->fuprog, _Error_upfile_open_);
/*
		{   
			 
			strncpy(stemp , gOptArg, sizeof(stemp)); 
			stp->fuprog = fopen(stemp,"rb");
			fseek(stp->fuprog, 0, 0);
			if (NULL == stp->fuprog) enError = _Error_upfile_open_;
			debuglog("open statu(%s) fileinname=%s\n", (NULL == stp->fuprog) ? "error" : "ok", stemp);
		}
		*/

	    break;

	  case 'R':
	    stp->stmodbusframs.cOper = 0;
	    debuglog ("cOper=%OFF\n");
	    break;

	  case 'S':
	    stp->stmodbusframs.cOper = 1;
	    debuglog ("cOper=%ON\n");
	    break;

	  case 'Y':
	    stp->cUprogUnitRetry = atoi (gOptArg);
	    debuglog ("cUprogUnitRetry=%d\n", stp->cUprogUnitRetry);
	    break;

	  case 'Z':
	    stp->cDebugSet = 1;
	    break;

	  default:
	    break;
	  }
      }
    }
  debuglog ("RegLen=%d(DataBytelen =%d)\n", stp->stmodbusframs.lDataByteLen / 2,
	    stp->stmodbusframs.lDataByteLen);
  debuglog ("File format =%s\n", (stp->cBinFormat) ? "BIN" : "TEXT");
  if (stp->stmodbusframs.lDataByteLen <= 0)
    enError = _Error_data_input_;
  return enError;
}

struct STERRORMESSAGE sterrormessage[_Error_end_] = {
  //{ _Error_no_, 0, "ok" },
  {_Error_nohelpexit_, 1, "\n"},
  {_Error_argv_input_error_, 1, "no argv,please -H\n"},
  {_Error_addr_input_, 1, "addr error\n"},
  {_Error_station_input_, 1, "station error\n"},
  {_Error_comno_input_, 0, "com port error\n"},
  {_Error_len_input_, 0, "ok"},
  {_Error_station_input_, 0, "ok"},
  {_Error_data_input_, 0, "ok"},
  {_Error_infile_open_, 0, "infile error\n"},
  {_Error_outfile_open_, 0, "outfile error\n"},
  {_Error_inifile_open_, 0, "inifile error\n"},
  {_Error_upfile_open_, 1, "open file error"},
  {_Error_upfile_comerror_, 0, "ok"},	//      
  {_Error_rece_noanswer_, 0, "com no answer\n"},	//       接收帧校验错误
  {_Error_receframe_checkerror_, 0, "modbus frame check error\n"},	//      接收帧校验错误
  {_Error_receapp_checkerror_, 0, "answer app error\n"},	//       接收帧应用错误
  {_Error_receapp_errorMax_, 0, "answer error\n"},	//   接收帧应用错误
};

struct STERRORMESSAGE *
errormessage_get (int ino)
{
  struct STERRORMESSAGE *stp = sterrormessage;
  int i;

  for (i = 0; i < _Error_end_; i++, stp++)
    {
      if (stp->err == ino)
	return stp;
    }
  return NULL;
}

int
error_fun (struct STERRORMESSAGE *stp)
{
  if (stp)
    {
      showversion ();
      if (stp->message)
	errorlog ("%s\n", stp->message);
      if (stp->cExit)
	return -1;
    }
  return 0;
}

int
Inifile_wide_to_schar (WCHAR * swIniFileName, WCHAR * swAppName,
		       WCHAR * swKeyName, WCHAR * swDef, char *sDesp,
		       unsigned short ilen)
{
  DWORD dResult;
  WCHAR swParityIniData[1024];
  GetPrivateProfileString ((LPCWSTR) swAppName, (LPCWSTR) swKeyName,
			   (LPCWSTR) swDef, swParityIniData,
			   sizeof (swParityIniData) /
			   sizeof (swParityIniData[0]), swIniFileName);

  WideCharToMultiByte (CP_ACP, 0, swParityIniData,
		       sizeof (swParityIniData) / sizeof (swParityIniData[0]),
		       sDesp, ilen, NULL, (LPBOOL) & dResult);
  return dResult;
}

char
ONOFF_State_Check (char *sSet)
{
  char cResult = strcmp (sSet, "ON");
  return (cResult ? 0 : 1);
}

int
inifile_analyse (char *sIniFileName)
{
  //swIniFileName;  ini配置文件

  struct STCONFIG *stp = &stconfig;
  WCHAR swIniFileName[256];
  char stemp[256];

  MultiByteToWideChar (CP_ACP, 0, sIniFileName,
		       strlen (sIniFileName) + 1, swIniFileName,
		       sizeof (swIniFileName) / sizeof (swIniFileName[0]));

  Inifile_wide_to_schar (swIniFileName, (WCHAR *) swAppName,
			 (WCHAR *) swDEBUG, (WCHAR *) "OFF", stemp,
			 sizeof (stemp));
  stp->cDebugSet = ONOFF_State_Check (stemp);

  Inifile_wide_to_schar (swIniFileName, (WCHAR *) swAppName,
			 (WCHAR *) swTIMESHOW, (WCHAR *) "OFF", stemp,
			 sizeof (stemp));

  stp->cTimeshow = ONOFF_State_Check (stemp);

  stp->stmodbusframs.cStation =
    (unsigned char) GetPrivateProfileInt ((LPCWSTR) swAppName,
					  (LPCWSTR) swSTATION, 1,
					  swIniFileName);

  stp->iReceTimeout =
    (UINT) GetPrivateProfileInt ((LPCWSTR) swAppName, (LPCWSTR) swRECETIMEOUT,
				 2410, swIniFileName);

  stp->cComPortNum =
    (unsigned char) GetPrivateProfileInt ((LPCWSTR) swAppName,
					  (LPCWSTR) swCOMPORT, 1,
					  swIniFileName);

  stp->cStopbit =
    (unsigned char) GetPrivateProfileInt ((LPCWSTR) swAppName,
					  (LPCWSTR) swSTOPBIT, 1,
					  swIniFileName);

  stp->iBps =
    (UINT) GetPrivateProfileInt ((LPCWSTR) swAppName, (LPCWSTR) swBPS, 2410,
				 swIniFileName);

  Inifile_wide_to_schar (swIniFileName, (WCHAR *) swAppName,
			 (WCHAR *) swPARITY, (WCHAR *) "NOPARITY",
			 stp->sParivaty, sizeof (stp->sParivaty));

  return 0;
}
