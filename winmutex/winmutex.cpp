#include "stdafx.h"


#include<stdio.h>
#include <iostream>
#include <windows.h>
#include "winmutex.h"
using namespace std;



 
HANDLE hMutex;

int hMutex_Init(void)
{
	// 若不存在名为"pmutex"的互斥量则创建它；否则获取其句柄
	hMutex = CreateMutex(NULL, false, ( LPCWSTR)"pmutex");
	if (NULL == hMutex)
		return  _enMutex_Error_;
	return _enMutex_Success_;
}


int  hMutex_P(void)
{
	
		// 申请对互斥量的占有
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
	
			// 操作完毕，释放对互斥量的占有
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

