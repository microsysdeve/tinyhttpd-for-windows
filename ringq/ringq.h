#ifndef __STFIFORING_HPP_
#define __STFIFORING_HPP_

 #include<stdio.h>

/*
#ifdef __cplusplus
extern "C"
{
#endif
*/
  struct STFIFORING
  {
	  volatile int  Lock;
    volatile int head;		/* ͷ���������з��� */
    volatile int tail;		/* β��������з��� */
    int size;			/* �����ܳߴ� */
    char *space;		//int space[QUEUE_MAX]; /* ���пռ� */  
    volatile short tag;		/* Ϊ�ջ���Ϊ���ı�־ */

  };

  enum
  {
    _fiforing_empty_,
    _fiforing_normal_,
    _fiforing_full_,
  };
/*  
     ��head == tail ʱ,tag = 0 Ϊ��,���� = 1Ϊ��
*/

  // extern int ringq_init ( struct STRINGGQ * p_queue);

  int Fiforing_init (struct STFIFORING *q_queue, char *sbuf, int buflen);
  int Fiforing_datap_init(struct STFIFORING *p_queue, char *sbuf, int buflen);
  int Fiforing_free (struct STFIFORING *p_queue);


/* �������ݵ����� */
  extern int Fiforing_push (struct STFIFORING *p_queue, char data);

/* �Ӷ���ȡ����*/
  int Fiforing_poll (struct STFIFORING *p_queue, char *p_data);
  int rignq_GetData (struct STFIFORING *p_queue, char *p_data, int iGetno);

#define Fiforing_GetFirst ( fifoq , datap ) 		rignq_GetData(fifoq,datap,0);
#define Fiforing_GetSecond( fifoq , datap ) 		rignq_GetData(fifoq,datap,1);

  //      ȡ�õ�ǰβ
#define   Fiforing_GetTail(fifoq)				(fifoq.tail)

  //���п�
#define _Fiforing_is_empty(q) ( (q->head == q->tail) && (q->tag == _fiforing_empty_))

//������
#define _Fiforing_is_full(q) ( (q->head == q->tail) && (q->tag == _fiforing_full_))

#define print_ringq(q) {}	//printf("ring head %d,tail %d,tag %d\n", q->head,q->tail,q->tag);
#define debug_printf(q)  {}

  int Fiforing_poll_mem (struct STFIFORING *p_queue, char *p_data, int len);

  int ring_GetDataNum (struct STFIFORING *p_queue, int iStart, int iEnd);

//�������  
#define  _ring_GetLen(p_queue)     ring_GetDataNum(p_queue,p_queue->head ,p_queue->tail )

int
Fiforing_poll_mem_stopstr (struct STFIFORING *p_queue, char *p_data, int buflen , char *sstopstr,int *poplen ) ;
/*
#ifdef __cplusplus
}
#endif
*/





extern struct STRINGGQ stringq;

#endif /* __STRINGGQ_H__ */
