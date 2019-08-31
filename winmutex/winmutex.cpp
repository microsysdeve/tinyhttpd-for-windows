#include "stdafx.h"


#include<stdio.h>
#include <iostream>
#include <windows.h>
#include "winmutex.h"
using namespace std;



 
HANDLE hMutex;

int hMutex_Init(void)
{
	// ����������Ϊ"pmutex"�Ļ������򴴽����������ȡ����
	hMutex = CreateMutex(NULL, false, ( LPCWSTR)"pmutex");
	if (NULL == hMutex)
		return  _enMutex_Error_;
	return _enMutex_Success_;
}


int  hMutex_P(void)
{
	
		// ����Ի�������ռ��
		DWORD  d = WaitForSingleObject(hMutex, INFINITE);
		if (WAIT_OBJECT_0 == d)
		{
			return _enMutex_PSuccess_;
		}
		if (WAIT_ABANDONED == d)
		{
			return _enMutex_PWait_;
		}
		if (WAIT_FAILED == d)
		{
			return _enMutex_Error_;
		}
		return _enMutex_Error_;	
}

int  hMutex_V(void)
{
	
			// ������ϣ��ͷŶԻ�������ռ��
			if (ReleaseMutex(hMutex) != 0)
			{
				return _enMutex_Success_;
			}
			else
			{
				return _enMutex_Error_ ;
			}
	

}

int  hMutex_Close(void)
{
	CloseHandle(hMutex);
	hMutex = NULL;
	return _enMutex_Success_;
}

