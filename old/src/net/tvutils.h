#ifndef _TVUTILS_H_
#define _TVUTILS_H_


#ifdef __WIN32

#include "win32defs.h"

/* This is declared in Winsock2.h
struct timeval {
  int tv_sec;
  int tv_usec;
};
*/

void gettimeofday(struct timeval *tv, void *tz);
int Init_gettimeofday();

#endif

#ifdef UNIX

#include <sys/time.h>

#endif


double calc_deltasec(struct timeval *tv1, struct timeval *tv2);
int tvcompare(struct timeval *tv1, struct timeval *tv2);
void tvsubtract(struct timeval *tv1, struct timeval *tv2);


#endif
