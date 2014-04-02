/*
 * -----------------------------------------------------------------------------
 * -----                           TEST_QUEUE.H                            -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *  This is the test program for queue.c
 *
 * Compiler:
 *  GCC
 *
 * Revision History:
 *  Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 *  Mar. 18, 2013      Nnoduka Eruchalu     Updated to use test framework
 */

#include <string.h>
#include "../queue.h"
#include "../general.h"
#include "test_general.h"

#define QUEUE_VALUE(i) ((i*2) % 256) /* value must always fit within a byte */


void PrintQueue(queue *q)
{
  int i;
  for(i=QUEUE_SIZE-1 ; i >= 0; i--) {
    if (i==QUEUE_SIZE-1)
      printf("          _____ \n");
    
    if (i== q->tail)
      printf("tail --> ");
    else
      printf("         ");
        
    if( ( (q->head< q->tail) && (i >= q->head) &&  (i< q->tail) ) ||
        ( (q->head>=q->tail) && ((QueueEmpty(q) == FALSE) && ((i>=q->head) ||
                                                              (i<q->tail))) )
        
        )
      printf("|__%d__|", q->array[i]);
    else
      printf("|_____|");
    
    if (i==q->head)
      printf(" <-- head");
    
    printf("\n");
  }
}

void test_queue(void)
{
  queue q;
  unsigned char val; /* use bytes because that's all this queue should hold */
  int i;
  
  QueueInit(&q);
  
  assert_equal_bool(TRUE, QueueEmpty(&q), "Queue: Wrong Queue Empty 1");
  assert_equal_bool(FALSE, QueueFull(&q), "Queue: Wrong Queue Full 1");
  
  for (i=0; i < (QUEUE_SIZE-1); i++) {
    Enqueue(&q,QUEUE_VALUE(i));
  }
  
  assert_equal_bool(FALSE, QueueEmpty(&q), "Queue: Wrong Queue Empty 2");
  assert_equal_bool(TRUE, QueueFull(&q), "Queue: Wrong Queue Full 2");
  
  for (i=0; i < (QUEUE_SIZE-1); i++) {
    val = Dequeue(&q);
    assert_equal_int(QUEUE_VALUE(i), val, "Queue: Wrong Queue Operation");
  }
  
  assert_equal_bool(TRUE, QueueEmpty(&q), "Queue: Wrong Queue Empty 3");
  assert_equal_bool(FALSE, QueueFull(&q), "Queue: Wrong Queue Full 3");
  
}
