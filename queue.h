/*
 * -----------------------------------------------------------------------------
 * -----                             QUEUE.H                               -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is the header file for queue.c, the library of functions for
 *   implementing a circular arrray with one empty slot.
 *
 * Assumptions:
 *   None.
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 */

#ifndef QUEUE_H
#define QUEUE_H

#define QUEUE_SIZE  512             /* size of the queue array must be 2^n */

typedef struct {                    /* queue datatype */
  unsigned int head;                /* head pointer of the queue */
  unsigned int tail;                /* tail pointer of the queue */
  unsigned char array[QUEUE_SIZE];  /* circular array represents queue */
} queue;


/* FUNCTION PROTOTYPES */
/* Initialize the passed in queue pointer */
extern void QueueInit(queue *q);

/* checks if the queue is empty */
extern unsigned char QueueEmpty(queue *q);

/* checks if the queue is full */
extern unsigned char QueueFull(queue *q);

/* remove a byte from head of the queue */
extern unsigned char Dequeue(queue *q);

/* add a byte to the tail of the queue */
extern void Enqueue(queue *q, unsigned char b);


#endif                                                             /* QUEUE_H */
