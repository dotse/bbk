#ifndef _EXECUTOR_H_
#define _EXECUTOR_H_

#ifdef UNIX
	#include <pthread.h>
#endif

#ifdef WIN32
#include "win32defs.h"
#endif

#include <stdlib.h>
#include <sys/types.h>

#ifndef INVALID_SOCKET 
	#ifdef UNIX
		#define INVALID_SOCKET (-1)
	#endif
#endif

void thread_exit();

struct thread_arg_struct {

  /* control variables */
  bool die;			/* tells thread to exit */
  bool execute;		/* used to pause thread execution */

  /* information variables (read-only) */
  bool started;		/* true if execution has started */
  bool executing;	/* is the thread running or not */
  bool completion;	/* true indicates success */
  int error;		/* error status in case there was an error */

  unsigned char progress;		/* 0-100 */
  char progress_str[512];		/* informative string describing current action */
  
  /* thread-specific arguments */
  void * thread_args;

  /* thread execution function pointer */
#ifdef WIN32
  LPTHREAD_START_ROUTINE start_routine;
#endif
#ifdef UNIX
  void *(*start_routine)(void *);
#endif
};

struct thread_arg_struct * new_thread_arg_struct();

#endif
