#ifndef _WINNUTEX_HPP_
#define _WINNUTEX_HPP_

 #include<stdio.h>

enum   ENUMHMUTEX
{
	_enMutex_Success_ = 0,
	_enMutex_Error_,
	_enMutex_PSuccess_,
	_enMutex_PWait_,
	_enMutex_End_,

};

extern HANDLE hMutex;

int hMutex_Init(void);

int  hMutex_P(void);
int  hMutex_V(void);

int  hMutex_Close(void);

#endif /* __STRINGGQ_H__ */
