#include "tvutils.h"


#ifdef UNIX
	#include <sys/time.h>
#endif


// calc_deltasec() - calculates difference between
// two timevals, in seconds, and stores the high
// precision result as a double

double calc_deltasec(struct timeval *tv1, struct timeval *tv2) {
  uint64_t t1, t2;
  t1 = ((uint64_t)tv1->tv_sec) * ((uint64_t)1000000) +
       ((uint64_t)tv1->tv_usec);
  t2 = ((uint64_t)tv2->tv_sec) * ((uint64_t)1000000) +
       ((uint64_t)tv2->tv_usec);
  if (t1 > t2)
    return (((double)(t1-t2))/((double)1000000));
  else
    return (((double)(t2-t1))/((double)1000000));
}


// tvcompare() - compare two timevals
// returns 1 if tv1 is bigger than tv2
// returns -1 if tv2 is bigger than tv1
// returns 0 if they are equal

int tvcompare(struct timeval *tv1, struct timeval *tv2) {
  if (tv1->tv_sec > tv2->tv_sec) return 1;
  if (tv2->tv_sec > tv1->tv_sec) return -1;
  if (tv1->tv_usec > tv2->tv_usec) return 1;
  if (tv2->tv_usec > tv1->tv_usec) return -1;
  return 0;
}

void tvsubtract(struct timeval *tv1, struct timeval *tv2) {
	tv1->tv_usec -= tv2->tv_usec;
	tv1->tv_sec -= tv2->tv_sec;
	if (tv1->tv_usec < 0) {
		tv1->tv_sec -= 1;
		tv1->tv_usec += 1000000;
	}
}

#ifdef __WIN32

/*
 * Init gettimeofday() variables
 * Returns 1 if system supports high-resolution timer
 */
/*
int Init_gettimeofday()
{
        if (!QueryPerformanceFrequency((LARGE_INTEGER*)&performance_frequency))
                return 0;
        QueryPerformanceCounter((LARGE_INTEGER *)&counter_start_value);
        start_sec = time(NULL);
        return 1;
}
*/
// Faked gettimeofday() function for Windows, using the
// Win32 performance counter
/*
void gettimeofday(struct timeval *tv, void *tz)
{
        _int64 time_now, dtime;
        int dsecs, dnsecs;
        QueryPerformanceCounter((LARGE_INTEGER *)&time_now);

        dtime = time_now - counter_start_value;
        dsecs = (int)(dtime / performance_frequency);
        dnsecs = (int)( ((double)(dtime % performance_frequency)) / 
		((double)performance_frequency) * 1000000.0);

        tv->tv_sec = start_sec + dsecs;
        tv->tv_usec = dnsecs;
}
*/
#endif
