/*
------------------------------------------------------------------------------
rand.h: My random number generator, ISAAC.
(c) Bob Jenkins, March 1996, Public Domain
You may use this code in any way you wish, and it is free.  No warrantee.
* May 2008 -- made it not depend on standard.h
------------------------------------------------------------------------------
*/
extern void RAND_bytes(unsigned char *buf, int num);
