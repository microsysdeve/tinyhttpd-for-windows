/* -------------------------------------------------------------------------
//	文件名		：	tinyhttp.cpp
//	创建者		：	magictong
//	创建时间	：	2016/11/16 17:13:55
//	功能描述	：	support windows of tinyhttpd, use mutilthread...
//
//	$Id: $
// -----------------------------------------------------------------------*/

/* J. David's webserver */
/* This is a simple webserver.
 * Created November 1999 by J. David Blackstone.
 * CSE 4344 (Network concepts), Prof. Zeigler
 * University of Texas at Arlington
 */
/* This program compiles for Sparc Solaris 2.6.
 * To compile for Linux:
 *  1) Comment out the #include <pthread.h> line.
 *  2) Comment out the line that defines the variable newthread.
 *  3) Comment out the two lines that run pthread_create().
 *  4) Uncomment the line that runs accept_request().
 *  5) Remove -lsocket from the Makefile.
 */ 

#include "stdafx.h"
#include "windowcgi.h"
#include "ThreadProc.h"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <WinSock2.h>
#include "sharememory.h"
#include "winmutex.h"
#include "ringq.h"
#pragma comment(lib, "wsock32.lib")
#pragma warning(disable : 4267)

	
struct SharedData
{
	int			PORTA;
	int			PORTB;	
	struct STFIFORING  STPORTA;
	struct STFIFORING  STPORTB;	
	char		sPORTAIN[256];
	char		sPORTBIN[256];
};


enum  ENUMMODE
{
		_ain_,
		_bin_,
};
enum  ENUMMODE  enMode= _ain_;

#define			_IsAin()			( _ain_ == enMode )
#define			_IsBin()			( !(_IsAin()))
#define			Mode_Con(mode)		{ enMode = mode ;}
#define			Mode_SetAin()		Mode_Con(_ain_)
#define			Mode_SetBin()		Mode_Con(_bin_)

const		int			iAportnum = 8923;
const		int			iBportnum = 8765;
char	*sFifoWorkp;
char sRecebuf[256];
struct STSHAREMEMORYCONTROL stover = {
  _sharememory_error_null_,	//enum        ENUMSHAREMEMORY_ERROR  enAppError;
  _sharememory_error_null_,	//DWORD nErrno;
  NULL,				//HANDLE hMapFile;
  _T("Global\\MyFileMappingObj"),	//TCHAR szName[256];
  (sizeof(struct SharedData)/ _iShareMemDataUnitLen_ +1)*_iShareMemDataUnitLen_,	// unsigned int              iBUF_SIZE;
  NULL,				//char  *pSd;      
};
struct STSHAREMEMORYCONTROL *stsharememoryp = &stover;
struct SharedData   *stsharedatap;

void stSharedData_init(struct SharedData *stp, enum  ENUMMODE entemp)
{

	if (_ain_ == entemp)
	{
		Fiforing_init(&(stp->STPORTA), stp->sPORTAIN, sizeof(stp->sPORTAIN));

	}
	else
		Fiforing_datap_init(&(stp->STPORTA), stp->sPORTAIN, sizeof(stp->sPORTAIN));

	if (_bin_ == entemp)
		Fiforing_init(&(stp->STPORTB), stp->sPORTBIN, sizeof(stp->sPORTBIN));
	else
		Fiforing_datap_init(&(stp->STPORTB), stp->sPORTBIN, sizeof(stp->sPORTBIN));

}

#define			debugprintf



int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsd;
	SOCKET sServer;
	SOCKET sClient;
	int retVal;
	
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);


	SYSTEM_INFO   sinf;
	GetSystemInfo(&sinf);
	DWORD   dwAllocationGranularity = sinf.dwAllocationGranularity;

	//  共享内存句柄
	if (_IsAin())
	{
		if (_sharememory_returnstate_Succ_ == stsharemem_init_App(stsharememoryp))
		{
			stSharedData_init((struct SharedData *)stsharememoryp->pSd, enMode);
			stsharedatap = ((struct SharedData *)(struct SharedData *)stsharememoryp->pSd);
		}
		else
		{
			printf("recv failed!\n");
			closesocket(sServer);
			closesocket(sClient);
			WSACleanup();
			return -1;
			return 0;
		}
	}
	else {
		if (_sharememory_returnstate_Succ_ == stsharemem_Get_App(stsharememoryp))
		{
			stSharedData_init((struct SharedData *)stsharememoryp->pSd, enMode);
			stsharedatap = ((struct SharedData *)(struct SharedData *)stsharememoryp->pSd);
		}
		else
		{
			printf("recv failed!\n");
			closesocket(sServer);
			closesocket(sClient);
			WSACleanup();
			return -1;
			return 0;
		}
	}
	hMutex_Init();
a1:

	//初始化Socket
	if (_IsAin())
		fprintf(stdout, "a in \r\n");
	else 
		fprintf(stdout, "b in \r\n");
	if (WSAStartup(MAKEWORD(2, 2), &wsd) != 0)
	{
		printf("WSAStartup failed!\n");
		return -1;
	}
	//创建用于监听的Socket
	sServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (INVALID_SOCKET == sServer)
	{
		printf("socket failed!\n");
		WSACleanup();
		return -1;
	}
	//设置Socket为非阻塞模式
	int iMode = 1;
	retVal = ioctlsocket(sServer, FIONBIO, (u_long FAR*)&iMode);
	if (retVal == SOCKET_ERROR)
	{
		printf("ioctlsocket failed!\n");
		WSACleanup();
		return -1;
	}
	//设置服务器Socket地址
	sockaddr_in addrServ;
	addrServ.sin_family = AF_INET;
	addrServ.sin_port = htons(_IsAin()?iAportnum:iBportnum);
	addrServ.sin_addr.S_un.S_addr = htonl(INADDR_ANY);
	//绑定Socket Server到本地地址
	retVal = bind(sServer, (const struct sockaddr*)&addrServ, sizeof(sockaddr_in));
	if (retVal == SOCKET_ERROR)
	{
		printf("bind failed!\n");
		closesocket(sServer);
		WSACleanup();
		return -1;
	}
	//监听
	retVal = listen(sServer, 1);
	if (retVal == SOCKET_ERROR)
	{
		printf("listen failed!\n");
		closesocket(sServer);
		WSACleanup();
		return -1;
	}
	//接受客户请求
	printf("TCP Server start...\n");

	

	//循环等待
	while (true)
	{
		sClient = accept(sServer, (sockaddr FAR*)&addrClient, &addrClientlen);
		if (INVALID_SOCKET == sClient)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				Sleep(100);
				continue;
			}
			else
			{
				printf("accept failed!\n");
				closesocket(sServer);
				WSACleanup();
				return -1;
			}
		}
		break;

	}
	


	//循环接受客户端的数据，直到客户端发送quit命令后退出
	while (true)
	{
		struct STFIFORING *stin = &(((struct SharedData *)(stsharememoryp->pSd))->STPORTA);
		struct STFIFORING *stout = &(((struct SharedData *)(stsharememoryp->pSd))->STPORTB);

		stin = (_IsAin())? &(((struct SharedData *)(stsharememoryp->pSd))->STPORTA): &(((struct SharedData *)(stsharememoryp->pSd))->STPORTB);
		stout = (_IsBin()) ? &(((struct SharedData *)(stsharememoryp->pSd))->STPORTA):&(((struct SharedData *)(stsharememoryp->pSd))->STPORTB);
		

		ZeroMemory(sRecebuf, sizeof(sRecebuf));
		retVal = recv(sClient, sRecebuf, sizeof(sRecebuf), 0);
		if (SOCKET_ERROR == retVal)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
				goto a2;
				Sleep(100); 
				continue;
			}
			else if (err == WSAETIMEDOUT || err == WSAENETDOWN)
			{
				printf("recv failed!\n");
				closesocket(sServer);
				closesocket(sClient);
				WSACleanup();
				return -1;
			}
		}
		if (0 == retVal)
		{
			closesocket(sServer);
			closesocket(sClient);
			WSACleanup();
			goto a1;
		}
		if (retVal > 0)
		{
			int i;
			sFifoWorkp = _IsAin() ? stsharedatap->sPORTAIN : stsharedatap->sPORTBIN;
			hMutex_P();
			for (i = 0; i < retVal; i++)
			{
				fprintf(stdout, "%c chanel in %d(0x%x)\r\n", _IsAin() ? 'a' : 'b', sRecebuf[i], sRecebuf[i]);
				Fiforing_push(stin, sRecebuf[i]);
			}
			hMutex_V();
		}
	
	
		a2:
		//获取系统时间
		/*SYSTEMTIME st;
		GetLocalTime(&st);
		char sDateTime[30];
		sprintf_s(sDateTime, "%4d-%2d-%2d %2d:%2d:%2d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		//打印输出信息
		printf("%s,Recv From Client [%s:%d]:%s\n", sDateTime, inet_ntoa(addrClient.sin_addr), addrClient.sin_port, sRecebuf);
		//如果客户端发送“quit”字符串，则服务器退出
		*/
		if  ( 0 && (strcmp(sRecebuf, "quit") == 0))
		{
			retVal = send(sClient, "quit", strlen("quit"), 0);
			break;
		}
		else
		{
			char		cout;
			while (true)
			{
				int			iResult;
				sFifoWorkp = _IsBin() ? stsharedatap->sPORTAIN : stsharedatap->sPORTBIN;
				
				hMutex_P();
				iResult=Fiforing_poll(stout, &cout);
				hMutex_V();
				if (_fiforing_normal_ == iResult)
				{
					retVal = send(sClient, &cout, 1, 0);
					fprintf(stdout, "%c chanel out  %d(0x%x)\r\n", _IsAin() ? 'a' : 'b', cout, cout);
					if (retVal < 0)
					{
						closesocket(sServer);
						closesocket(sClient);
						WSACleanup();
						goto a1;
					}
				}
				else
					retVal = SOCKET_ERROR + 1;
				
				if (SOCKET_ERROR == retVal)
				{
					int err = WSAGetLastError();
					if (err == WSAEWOULDBLOCK)
					{
						Sleep(100);
						continue;
					}
					else
					{
						printf("send failed!\n");
						closesocket(sServer);
						closesocket(sClient);
						WSACleanup();
						return -1;
					}

				}
				break;
			}
		}
	}

	//释放Socket
	closesocket(sServer);
	closesocket(sClient);
	WSACleanup();

	system("pause");
	return 0;
}
#ifdef _DDD
int _tmain(int argc, _TCHAR* argv[])
{
	SOCKET server_sock = INVALID_SOCKET;
	//u_short port = 0;
	u_short port = 80;
	struct sockaddr_in client_name = {0};
	int client_name_len = sizeof(client_name);
	typedef CMultiTaskThreadPoolT<CTinyHttp, CTinyHttp::SOCKET_CONTEXT, nilstruct, 5, CComMultiThreadModel::AutoCriticalSection> CMultiTaskThreadPool;
	CTinyHttp tinyHttpSvr;

	// init socket
	WSADATA wsaData = {0};
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	server_sock = tinyHttpSvr.startup(&port);
	printf("httpd running on port: %d\n", port);
	CMultiTaskThreadPool m_threadpool(&tinyHttpSvr, &CTinyHttp::accept_request);

	if ( _sharememory_returnstate_Succ_ == stsharemem_init_App(stsharememoryp))
	{
	stSharedData_init((struct SharedData *)stsharememoryp->pSd);
	}
	else
	{
		closesocket(server_sock);
		WSACleanup();
		return 0;
	}


	while (1)
	{
		CTinyHttp::SOCKET_CONTEXT socket_context;
		socket_context.socket_Client = accept(server_sock, (struct sockaddr *)&client_name, &client_name_len);
		if (socket_context.socket_Client == INVALID_SOCKET)
		{
			tinyHttpSvr.error_die("accept");
		}

		printf("Tid[%u] accetp new connect: %u\n", (unsigned int)::GetCurrentThreadId(), (unsigned int)socket_context.socket_Client);
		m_threadpool.AddTask(socket_context);
	}

	// can not to run this
	m_threadpool.EndTasks();
	closesocket(server_sock);
	WSACleanup();
	return 0;
}




#endif
int			checktcp(int sock)
{
	struct tcp_info info;
	int len = sizeof(info);
	memset ( (char *)&info,0,sizeof(info))；
	getsockopt(sock, IPPROTO_TCP, TCP_INFO, &info, (socklen_t *)&len);
	if ((info.tcpi_state == TCP_ESTABLISHED))
		return SUCCESS; else
		return  ERROR;
}






// -------------------------------------------------------------------------
// $Log: $