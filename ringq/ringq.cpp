#include "stdafx.h"


#include<stdio.h>
#include "ringq.h"
//#include "stm32f1xx_hal.h"

int
Fiforing_init (struct STFIFORING *p_queue, char *sbuf, int buflen)
{

  p_queue->size = buflen;
  p_queue->space = sbuf;
  p_queue->head = 0;
  p_queue->tail = 0;
  p_queue->tag = _fiforing_empty_;

  return 0;
}
int
Ffiforing_free (struct STFIFORING *p_queue)
{
  return 0;
}

 /*入列 */
int				// 中断态执行
Fiforing_push (struct STFIFORING *p_queue, char cData)
{
  print_ringq (p_queue);

  if (_Fiforing_is_full (p_queue))
    {
      debug_printf ("ringq is full\n");
      return -1;
    }
  p_queue->space[p_queue->tail] = cData;
  p_queue->tail = (p_queue->tail + 1) % p_queue->size;
  /* 这个时候一定队列满了 */
  if (p_queue->tail == p_queue->head)
    p_queue->tag = _fiforing_full_;
  else
    p_queue->tag = _fiforing_normal_;

  return p_queue->tag;
}


 /*出列 */
int				// 普通态执行
Fiforing_poll (struct STFIFORING *p_queue1, char *p_data)
{
   struct STFIFORING stbak;
  struct STFIFORING *p_queue  = &stbak;

//opt  USART_ITConfig (USART2, USART_IT_RXNE, DISABLE);

  memcpy ((char *) p_queue, (char *) p_queue1, sizeof (struct STFIFORING));
  print_ringq (p_queue);
  if (_Fiforing_is_empty (p_queue))
    {
      debug_printf ("ringq is empty\n");
   //   USART_ITConfig (USART2, USART_IT_RXNE, ENABLE);
      return _fiforing_empty_;		//-1;
    }

  *p_data = p_queue->space[p_queue->head];
  p_queue->head = (p_queue->head + 1) % p_queue->size;

  /*这个时候一定队列空了 */
  if (p_queue->tail == p_queue->head)
    p_queue->tag = _fiforing_empty_;
  else
    p_queue->tag = _fiforing_normal_;

  memcpy ((char *) p_queue1, (char *) p_queue, sizeof (struct STFIFORING));

  //opt USART_ITConfig (USART2, USART_IT_RXNE, ENABLE);
  return _fiforing_normal_ ;//p_queue->tag;
}

int
rignq_GetData (struct STFIFORING *p_queue, char *p_data, int iGetno)
{
  struct STFIFORING stringqbak;
  int iResult, i;
  char cData;

  memcpy ((char *) &stringqbak, (char *) p_queue, sizeof (stringqbak));

  for (i = 0; i < iGetno; i++)
    iResult = Fiforing_poll ((struct STFIFORING *) &stringqbak, &cData);
  return iResult;
}

int
ring_GetDataNum (struct STFIFORING *p_queue, int iStart, int iEnd)
{
	 if ( _fiforing_empty_ == p_queue->tag )
				return 0;
	 else  if ( _fiforing_full_ == p_queue->tag )
			return p_queue->size;
    else
		if (iStart > iEnd)
			iEnd += p_queue->size;

  return (iEnd - iStart);
}

//              出列指定个数
int
Fiforing_poll_mem (struct STFIFORING *p_queue, char *p_data, int len)
{
  int i, iResult;

  for (i = 0; i < len; i++)
    {
      iResult = Fiforing_poll (p_queue, (p_data));
      if (_fiforing_empty_ == iResult)
	break;
    }
  return i;
}
/*
//              出列至指定字符序列
int
Fiforing_poll_mem_stopstr (struct STFIFORING *p_queue, char *p_data, int buflen , char *sstopstr,int *poplen )
{  
	char 		clinework = 0;
	
		
	for ( *poplen  =0 ;*poplen  <buflen ;*poplen ++)
	{
			if (_fiforing_empty_ == Fiforing_poll (p_queue, (p_data+*poplen )))
       			return -1;
			else
			{
					(*poplen) ++	;
					if  ( SUCCESS == strfiter (sstopstr,&(clinework),*(p_data+*poplen ))) 
						  return  1 ;
			 }						
		}
	return -1 ;
  
}
*/