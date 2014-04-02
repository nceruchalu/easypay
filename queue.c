/*
 * -----------------------------------------------------------------------------
 * -----                             QUEUE.C                               -----
 * -----                             EASYPAY                               -----
 * -----------------------------------------------------------------------------
 *
 * File Description:
 *   This is a library of functions for implementing a circular FIFO array with 
 *   one empty slot.
 *
 *
 * ===================== LONG EXPLANATION OF QUEUES HERE =======================
 Queues have 2 pointers, a head pointer and a tail pinter. Add at the tail ptr
 and remove at the head ptr.
 
 You can make either the head or tail ptr fixed and
 use a simple array to hold the queue. But if you do that then every time you
 add or take away an entry (depending on which end is fixed) you have to shift
 the entire queue contents --> very time consuming.
 
 Instead you generally use a circular array to implement the queue. With a
 circular array/queue you have to worry about the pointers wrapping around the
 array/queue. To handle the wrap, all queue pointer arithmetic is done modulo
 the array/queue size. Typically you pick a power of 2 as the queue size because
 modulo 2^n is the same as AND 2^n -1.
 Also you have to keep a "hole" in a circular queue (an empty location).
 Otherwise you can't tell the difference between an empty and a full queue.
 Finally the updates on the pointers can be pre- or post-increment.
 
 Below is a table giving the pointer updates for full and empty queue conditions
 
 Head Ptr   Tail Ptr          Full                     Empty
 pre        pre               head == (tail+1)         head == tail
 pre        post              head == tail             (head+1) == tail
 post       pre               head == (tail+2)         head == (tail+1)
 post       post              head == (tail+1)         head == tail

 Note that the +1 and +2 are modulo queue size.
 
 Since these checks will often be done in an interrupt routine and we, ideally,
 want interrupt routines to be as fast and as simple as possible. Ideally that
 means we would use case 1 for a transmit queue (where we worry about being
 empty in the interrupt handler) and case 2 for a receive queue (where we worry
 about the queue filling in the interrupt handler). Since its kind of a hassle
 to do the two queues differently, you may just pick one method (not case 3) and
 use that for both transmit and receive queues and not worry about overhead.
 
 I chose to use post-increment on both head and tail.

 The queue array size is of size QUEUE_SIZE
 So the array has QUEUE_SIZE-1 usable slots.
 (Remember a circular queue needs 1 empty slot? Well better remember!)
 
 Let QUEUE_SIZE = 4
 Then Below is my array:
   _____
  |_____|
  |_____|
  |_____|
  |_____|

 So if my head and tail pointer both start at the same location this means my
 queue is empty. Remember for post-post the empty check is if head==tail.
            _____
           |_____|
           |_____|
           |_____|
  tail --> |_____| <-- head

 If I enqueue the value 0, my queue now looks like this:
            _____
           |_____|
           |_____|
  tail --> |_____|
           |__0__| <-- head
 Remember I am doing post-incremement and enqueue at the TAIL.
 
 Now, enqueue 1 and 2:
            _____
  tail --> |_____|
           |__2__|
           |__1__|
           |__0__| <-- head

 So now I have used up 3 slots of my queue of QUEUE_SIZE=4, which means the 
 queue should be full. Again remember a circular queue always has an empty slot.
 So in this case, (tail+1) == head.
 
 So now if I decide to dequeue 1 element from the queue, it will look like this:
            _____
  tail --> |_____|
           |__2__|
           |__1__| <-- head
           |_____|
 Remember we dequeue from the head!

 Now the queue is no longer full so I can enqueue 3.
            _____
           |__3__|
           |__2__|
           |__1__| <-- head
  tail --> |_____|
 
 Notice how the tail wrapped around? It went from index 3 to 0.
 
 Implement this modulo QUEUE_SIZE = 2^n by ANDing with (QUEUE_SIZE-1)
 *
 *
 * Table of Contents:
 *   QueueInit  -initializes the queue at the address of passed in queue pointer
 *   QueueEmpty  -checks if the queue is empty
 *   QueueFull  -checks if the queue is full
 *   Dequeue    -remove a byte from head of the queue
 *   Enqueue    -add a byte to tail of the queue
 *
 * Limitations:
 *   - The queue array is a predefined size of QUEUE_SIZE and the queue is a
 *     circular array with one empty slot (wich can move around the queue). 
 *     Thus, the maximum queue size available to the user is QUEUE_SIZE-1.
 *   - QUEUE_SIZE has to be 2^n. This makes MODULUS simply an AND with
 *     (QUEUE_SIZE - 1).
 *
 * Compiler:
 *   HI-TECH C Compiler for PIC18 MCUs (http://www.htsoft.com/)
 *
 * Revision History:
 *   Dec. 19, 2012      Nnoduka Eruchalu     Initial Revision
 */

#include "general.h"
#include "queue.h"

/*
 * QueueInit
 * Description:
 *  Initializes the queue at the passed address. This procedure resets the queue
 *  to an empty state, so it is ready to accept values. 
 *  
 * Operation:
 *  Initializes the queue which is implemented as a struct.
 *  Since this queue uses post-incrementation of the head and tail, it is set
 *  to empty by setting head and tail pointers to the beginning of the array.
 *  Thus head == tail == 0. Even though it is a circular array, it is only
 *  natural to initialize the pointers to 0. This value works with any
 *  QUEUE_SIZE
 *
 * Limitations:
 *  - Size of the queue array is fixed at QUEUE_SIZE
 *
 * Revision History:
 *  Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 */
void QueueInit(queue *q)
{
  q->head = q->tail = 0; /* set queue to empty state and use a value that */
                         /* works for all QUEUE_SIZE values */
}


/* QueueEmpty 
 * Description: This function is called with address of the queue to be checked 
 *              and returns with TRUE if the queue is empty and with FALSE 
 *              otherwise.
 *
 * Arguments:   q: ptr to queue
 * Return:      boolean of queue empty status (TRUE/FALSE)
 *
 * Operation:   Since we are using post-post, the empty check is head == tail.
 * 
 * Limitations: None
 *
 * Revision History:
 *   Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char QueueEmpty(queue *q)
{
  return (q->head == q->tail); /* empty if head == tail */
}


/* QueueFull
 * Description: This function is called with address of the queue to be checked 
 *              and returns  with TRUE if the queue is full and with FALSE 
 *              otherwise.
 *
 * Arguments:   q: ptr to queue
 * Return:      boolean of queue full status (TRUE/FALSE)
 *
 * Operation:   Since we are using post-post, the empty check is: 
 *                head == (tail+1).
 * 
 * Limitations: None
 *
 * Revision History:
 *   Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char QueueFull(queue *q)
{
  return (q->head == ((q->tail +1) & (QUEUE_SIZE-1)));/*full if head==(tail+1)*/
}


/* Dequeue
 * Description: This function removes and returns a byte value from the head of 
 *              the queue.
 *              If the queue is empty it waits until the queue has a value to be
 *              removed and returned. It is a blocking function.
 *
 * Arguments:   q: ptr to queue
 * Return:      dequeued byte.
 *
 * Operation:   The function waits for the queue to be non-empty before it can 
 *              remove a value. This is done in a while loop. When the loop is 
 *              finally not empty, the function retrieves a value from the head 
 *              of the queue and increments the head pointer.
 * 
 * Limitations: Size of the queue array has to be a constant 2^n to enable 
 *              MODULUS with AND 2^n -1
 *
 * Revision History:
 *   Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 */
unsigned char Dequeue(queue *q) {
  unsigned char val;            /* dequeued value */
  while (QueueEmpty(q))         /* wait for Queue to be not-empty */
    continue;
  
  val = q->array[q->head];      /* dequeue value from head, and post-INC head */
  q->head = (q->head + 1) & (QUEUE_SIZE-1); /* head = [(head+1) % QUEUE_SIZE] */
  
  return val;                   /* return dequeued value */
}


/* Enqueue
 * Description: This function adds the passed in byte value to the tail of the 
 *              queue. If the queue is full it waits until the queue has an open
 *              slot to  add the value.
 *              It does not return until the value is added to the queue. It is 
 *              a blocking function.
 *
 * Arguments:   q: ptr to queue
 *              b: byte to be enqueued
 * Return:      None
 * 
 * Operation:   The function waits for the queue to be not full before it adds a
 *              value. This blocking wait is accomplished via a while loop. When
 *              the value is added, the tail pointer is incremented.
 * 
 * Limitations: Size of the queue array has to be a constant 2^n to enable 
 *              MODULUS with AND 2^n -1
 *
 * Revision History:
 *   Dec. 20, 2012      Nnoduka Eruchalu     Initial Revision
 */
void Enqueue(queue *q, unsigned char b)
{ 
  while (QueueFull(q))          /* wait for Queue to be not-full */
    continue;
  
  q->array[q->tail] = b;        /* enqueue at tail, and post-INC tail */
  q->tail = (q->tail + 1) & (QUEUE_SIZE-1); /* tail = [(tail+1) % QUEUE_SIZE] */

  return;
}
