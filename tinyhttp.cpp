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

#include "ringq.h"
#pragma comment(lib, "wsock32.lib")
#pragma warning(disable : 4267)

#define ISspace(x) isspace((int)(x))
#define SERVER_STRING "Server: tinyhttp /0.1.0\r\n"
// -------------------------------------------------------------------------

struct SharedData
{
	int			PORTA;
	int			PORTB;
	struct STFIFORING  STPORTA;
	struct STFIFORING  STPORTB;
	char		sPORTAIN[256];
	char		sPORTBIN[256];
};

void stSharedData_init(struct SharedData *stp)
{
	
		Fiforing_init(&(stp->STPORTA), stp->sPORTAIN, sizeof(stp->sPORTAIN));
		Fiforing_init(&(stp->STPORTB), stp->sPORTBIN, sizeof(stp->sPORTBIN));

}

struct STSHAREMEMORYCONTROL stover = {
  _sharememory_error_null_,	//enum        ENUMSHAREMEMORY_ERROR  enAppError;
  _sharememory_error_null_,	//DWORD nErrno;
  NULL,				//HANDLE hMapFile;
  _T("Global\\MyFileMappingObj"),	//TCHAR szName[256];
  sizeof(struct SharedData),	// unsigned int              iBUF_SIZE;
  NULL,				//char  *pSd;      
};
struct STSHAREMEMORYCONTROL *stsharememoryp = &stover;
// -------------------------------------------------------------------------
// 类名		: CTinyHttp
// 功能		: 
// 附注		: 
// -------------------------------------------------------------------------
class CTinyHttp
{
public:
	typedef struct tagSocketContext
	{
		SOCKET socket_Client;
		tagSocketContext() : socket_Client(-1) {}
	} SOCKET_CONTEXT, *PSOCKET_CONTEXT;

/**********************************************************************/  
/* A request has caused a call to accept() on the server port to 
 * return.  Process the request appropriately. 
 * Parameters: the socket connected to the client */  
/**********************************************************************/  
void accept_request(nilstruct&, SOCKET_CONTEXT& socket_context)
{
	printf("Tid[%u] accept_request\n", (unsigned int)::GetCurrentThreadId());

#ifdef _DEBUG
	// 测试是否可以并发
	::Sleep(200);
#endif

	char buf[1024] = {0};
	int numchars = 0;
	char method[255] = {0};
	char url[255] = {0};
	char path[512] = {0};
	int i = 0, j = 0;
	struct stat st;
	int cgi = 0;      /* becomes true if server decides this is a CGI program */
	char* query_string = NULL;
	SOCKET client = socket_context.socket_Client;


	numchars = get_li111ne(client, buf, sizeof(buf));

	// 获取HTTP的请求方法名
	while (j < numchars && !ISspace(buf[j]) && (i < sizeof(method) - 1))
	{
		method[i] = buf[j];
		i++; j++;
	}
	method[i] = '\0';

	if (_stricmp(method, "GET") != 0 && _stricmp(method, "POST"))      // 只处理GET请求
	{
		if (numchars > 0)
		{
			discardheaders(client);
		}

		unimplemented(client);
		closesocket(client);
		return;
	}

	if (_stricmp(method, "POST") == 0)
		cgi = 1; // POST请求，当成CGI处理

	// 获取到URL路径，存放到url字符数组里面
	i = 0;
	while (ISspace(buf[j]) && (j < sizeof(buf)))
	{
		j++;
	}
	
	while (!ISspace(buf[j]) && (i < sizeof(url) - 1) && (j < sizeof(buf)))
	{
		url[i] = buf[j];
		i++;
		j++;
	}
	url[i] = '\0';

	if (_stricmp(method, "GET") == 0)
	{
		query_string = url;
		while ((*query_string != '?') && (*query_string != '\0'))
			query_string++;

		if (*query_string == '?')
		{
			// URL带参数，当成CGI处理
			cgi = 1;
			*query_string = '\0';
			query_string++;
		}
	}

	sprintf_s(path, 512, "htdocs%s", url);
	if (path[strlen(path) - 1] == '/')
	{
		// 补齐
		strcat_s(path, 512, "index.html");
	}
	
	if (stat(path, &st) == -1)
	{
		// 文件不存在
		if (numchars > 0)
		{
			discardheaders(client);
		}

		not_found(client);
	}
	else
	{
		// 如果是文件夹则补齐
		if ((st.st_mode & S_IFMT) == S_IFDIR)
			strcat_s(path, 512, "/index.html");

		if (st.st_mode & S_IEXEC)
			cgi = 1; // 具有可执行权限

		if (!cgi)
		{
			serve_file(client, path);
		}
		else
		{
			execute_cgi(client, path, method, query_string);
		}
	}

	closesocket(client);
}

/**********************************************************************/
/* Execute a CGI script.  Will need to set environment variables as
 * appropriate.
 * Parameters: client socket descriptor
 *             path to the CGI script */
/**********************************************************************/
void execute_cgi(SOCKET client, const char *path, const char* method, const char* query_string)
{
	char buf[1024] = {0};
	int cgi_output[2] = {0};
	int cgi_input[2] = {0};

	int i = 0;
	char c = 0;
	int numchars = 1;
	int content_length = -1;

	 



	buf[0] = 'A'; buf[1] = '\0';
	if (_stricmp(method, "GET") == 0)
	{
		discardheaders(client);
	}
	else    /* POST */
	{
		numchars = get_line(client, buf, sizeof(buf));
		while ((numchars > 0) && strcmp("\n", buf))
		{
			buf[15] = '\0';
			if (_stricmp(buf, "Content-Length:") == 0)
			{
				content_length = atoi(&(buf[16]));
			}

			numchars = get_line(client, buf, sizeof(buf));
		}

		if (content_length == -1)
		{
			bad_request(client);
			return;
		}
	}

	CWinCGI cgi;
	if (!cgi.Exec(path, query_string))
	{
		bad_request(client);
		return;
	}

	//SOCKET client, const char *path, const char* method, const char* query_string
	if (_stricmp(method, "POST") == 0)
	{
		for (i = 0; i < content_length; i++)
		{
			recv(client, &c, 1, 0);
			cgi.Write((PBYTE)&c, 1);
		}

		c = '\n';
		cgi.Write((PBYTE)&c, 1);
	}

	cgi.Wait();
	char outBuff[2048] = {0};
	cgi.Read((PBYTE)outBuff, 2047);
	send(client, outBuff, strlen(outBuff), 0);
}

/**********************************************************************/
/* Put the entire contents of a file out on a socket.  This function
 * is named after the UNIX "cat" command, because it might have been
 * easier just to do something like pipe, fork, and exec("cat").
 * Parameters: the client socket descriptor
 *             FILE pointer for the file to cat */
/**********************************************************************/
void cat(SOCKET client, FILE *resource)
{
	char buf[1024] = {0};

	do 
	{
		fgets(buf, sizeof(buf), resource);
		size_t len = strlen(buf);
		if (len > 0)
		{
			send(client, buf, len, 0);
		}
	} while (!feof(resource));
}

/**********************************************************************/
/* Print out an error message with perror() (for system errors; based
 * on value of errno, which indicates system call errors) and exit the
 * program indicating an error. */
/**********************************************************************/
void error_die(const char *sc)
{
	perror(sc);
	exit(1);
}  
  
/**********************************************************************/
/* Get a line from a socket, whether the line ends in a newline,
 * carriage return, or a CRLF combination.  Terminates the string read
 * with a null character.  If no newline indicator is found before the
 * end of the buffer, the string is terminated with a null.  If any of
 * the above three line terminators is read, the last character of the
 * string will be a linefeed and the string will be terminated with a
 * null character.
 * Parameters: the socket descriptor
 *             the buffer to save the data in
 *             the size of the buffer
 * Returns: the number of bytes stored (excluding null) */
/**********************************************************************/
int get_li111ne(SOCKET sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;
	struct STFIFORING *stin = &(((struct SharedData *)(stsharememoryp->pSd))->STPORTA);
	struct STFIFORING *stout = &(((struct SharedData *)(stsharememoryp->pSd))->STPORTB);


		while (1)
		{
			n = recv(sock, &c, 1, 0);
			if (n > 0)
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				if (SOCKET_ERROR == n)
				{
				}else

				if (n > 0)
					Fiforing_push(stin, c);
			}
			if (_fiforing_normal_ ==  Fiforing_poll(stout, &c))
						send(sock, &c, 1, 0);
				
		
		}
	
}


int get_line(SOCKET sock, char *buf, int size)
{
	int i = 0;
	char c = '\0';
	int n;

 

	while ((i < size - 1) && (c != '\n'))
	{
		n = recv(sock, &c, 1, 0);
		/* DEBUG printf("%02X\n", c); */
		if (n > 0)
		{
			if (c == '\r')
			{
				n = recv(sock, &c, 1, MSG_PEEK);
				/* DEBUG printf("%02X\n", c); */
				if ((n > 0) && (c == '\n'))
				{
					recv(sock, &c, 1, 0);
				}
				else
				{
					c = '\n';
				}
			}
			buf[i] = c;
			i++;
		}
		else
		{
			c = '\n';
		}
	}
	buf[i] = '\0';

	return(i);
}  
  
/**********************************************************************/
/* Return the informational HTTP headers about a file. */
/* Parameters: the socket to print the headers on
 *             the name of the file */
/**********************************************************************/
void headers(SOCKET client, const char *filename)
{
	(void)filename;

	char* pHeader = "HTTP/1.0 200 OK\r\n"\
		SERVER_STRING \
		"Content-Type: text/html\r\n\r\n";

	send(client, pHeader, strlen(pHeader), 0);
}  
  
/**********************************************************************/
/* Give a client a 404 not found status message. */
/**********************************************************************/
void not_found(SOCKET client)  
{
	char* pResponse = "HTTP/1.0 404 NOT FOUND\r\n"\
		SERVER_STRING \
		"Content-Type: text/html\r\n\r\n"\
		"<HTML><TITLE>Not Found</TITLE>\r\n"\
		"<BODY><P>The server could not fulfill\r\n"\
		"your request because the resource specified\r\n"\
		"is unavailable or nonexistent.\r\n"\
		"</BODY></HTML>\r\n";

	send(client, pResponse, strlen(pResponse), 0);
}

/**********************************************************************/
/* Inform the client that the requested web method has not been
 * implemented.
 * Parameter: the client socket */
/**********************************************************************/
void unimplemented(SOCKET client)
{
	char* pResponse = "HTTP/1.0 501 Method Not Implemented\r\n"\
		SERVER_STRING \
		"Content-Type: text/html\r\n\r\n"\
		"<HTML><HEAD><TITLE>Method Not Implemented\r\n"\
		"</TITLE></HEAD>\r\n"\
		"<BODY><P>HTTP request method not supported.</P>\r\n"\
		"</BODY></HTML>\r\n";

	send(client, pResponse, strlen(pResponse), 0);
}

/**********************************************************************/
/* Inform the client that a CGI script could not be executed.
 * Parameter: the client socket descriptor. */
/**********************************************************************/
void cannot_execute(SOCKET client)
{
	char* pResponse = "HTTP/1.0 500 Internal Server Error\r\n"\
		"Content-Type: text/html\r\n\r\n"\
		"<P>Error prohibited CGI execution.</P>\r\n";

	send(client, pResponse, strlen(pResponse), 0);
}

/**********************************************************************/
/* Inform the client that a request it has made has a problem.
 * Parameters: client socket */
/**********************************************************************/
void bad_request(SOCKET client)
{
	char* pResponse = "HTTP/1.0 400 BAD REQUEST\r\n"\
		"Content-Type: text/html\r\n\r\n"\
		"<P>Your browser sent a bad request, such as a POST without a Content-Length.</P>\r\n";

	send(client, pResponse, strlen(pResponse), 0);
}

/**********************************************************************/
/* Send a regular file to the client.  Use headers, and report
 * errors to client if they occur.
 * Parameters: a pointer to a file structure produced from the socket
 *              file descriptor
 *             the name of the file to serve */
/**********************************************************************/
void serve_file(SOCKET client, const char *filename)
{
	FILE *resource = NULL;
	discardheaders(client);

	fopen_s(&resource, filename, "r");
	if (resource == NULL)
	{
		not_found(client);
	}
	else  
	{
		headers(client, filename);
		cat(client, resource);
	}
	fclose(resource);
}

// -------------------------------------------------------------------------
// 函数		: discardheaders
// 功能		: 清除http头数据（从网络中全部读出来）
// 返回值	: void 
// 参数		: SOCKET client
// 附注		: 
// -------------------------------------------------------------------------
void discardheaders(SOCKET client)
{
	char buf[1024] = {0};
    int numchars = 1;
    while ((numchars > 0) && strcmp("\n", buf))  /* read & discard headers */
	{
        numchars = get_line(client, buf, sizeof(buf));
	}
}

/**********************************************************************/
/* This function starts the process of listening for web connections
 * on a specified port.  If the port is 0, then dynamically allocate a
 * port and modify the original port variable to reflect the actual
 * port.
 * Parameters: pointer to variable containing the port to connect on
 * Returns: the socket */
/**********************************************************************/
SOCKET startup(u_short* port)
{
	SOCKET httpd = 0;
	struct sockaddr_in name = {0};

	httpd = socket(AF_INET, SOCK_STREAM, 0);
	if (httpd == INVALID_SOCKET)
	{
		error_die("startup socket");
	}
	
	name.sin_family = AF_INET;
	name.sin_port = htons(*port);
	name.sin_addr.s_addr = inet_addr("127.0.0.1");
	if (bind(httpd, (struct sockaddr *)&name, sizeof(name)) < 0)  
	{
		error_die("startup bind");
	}
	
	if (*port == 0)  /* if dynamically allocating a port */  
	{
		int namelen = sizeof(name);
		if (getsockname(httpd, (struct sockaddr *)&name, &namelen) == -1)
		{
			error_die("getsockname");
		}

		*port = ntohs(name.sin_port);
	}

	if (listen(httpd, 5) < 0)
	{
		error_die("listen");
	}

	return httpd;
}

}; // End Class CTinyHttp

#define		BUF_SIZE   256
int _tmain(int argc, _TCHAR* argv[])
{
	WSADATA wsd;
	SOCKET sServer;
	SOCKET sClient;
	int retVal;
	char buf[BUF_SIZE];

	//初始化Socket
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
	addrServ.sin_port = htons(9990);
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
	sockaddr_in addrClient;
	int addrClientlen = sizeof(addrClient);
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
		ZeroMemory(buf, BUF_SIZE);
		retVal = recv(sClient, buf, BUFSIZ, 0);
		if (SOCKET_ERROR == retVal)
		{
			int err = WSAGetLastError();
			if (err == WSAEWOULDBLOCK)
			{
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
		//获取系统时间
		SYSTEMTIME st;
		GetLocalTime(&st);
		char sDateTime[30];
		sprintf_s(sDateTime, "%4d-%2d-%2d %2d:%2d:%2d", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
		//打印输出信息
		printf("%s,Recv From Client [%s:%d]:%s\n", sDateTime, inet_ntoa(addrClient.sin_addr), addrClient.sin_port, buf);
		//如果客户端发送“quit”字符串，则服务器退出
		if (strcmp(buf, "quit") == 0)
		{
			retVal = send(sClient, "quit", strlen("quit"), 0);
			break;
		}
		else
		{
			char msg[BUF_SIZE];
			sprintf_s(msg, "Message received - %s", buf);
			while (true)
			{
				retVal = send(sClient, msg, strlen(msg), 0);
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
// -------------------------------------------------------------------------
// $Log: $