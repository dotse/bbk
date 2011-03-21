#ifndef _TVUTILS_H_
#define _TVUTILS_H_


#ifdef __WIN32

#include "win32defs.h"

void gettimeofday(struct timeval *tv, void *tz);
int Init_gettimeofday();

struct timeval {
  int tv_sec;
  int tv_usec;
};

#endif

#ifdef UNIX

#include <sys/time.h>
#include <sys/types.h>
#include <stdint.h>

#endif


double calc_deltasec(struct timeval *tv1, struct timeval *tv2);
int tvcompare(struct timeval *tv1, struct timeval *tv2);
void tvsubtract(struct timeval *tv1, struct timeval *tv2);



#endif
