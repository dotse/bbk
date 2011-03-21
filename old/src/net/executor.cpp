#include "executor.h"

void thread_exit() {
#ifdef WIN32
	ExitThread(0);
#endif
#ifdef UNIX
	pthread_exit(0);
#endif
}

struct thread_arg_struct * new_thread_arg_struct() {
  struct thread_arg_struct *ret;
  ret = (thread_arg_struct*)calloc(sizeof(struct thread_arg_struct), 1);
  ret->executing = false;
  ret->die = false;
  ret->execute = true;
  ret->started = false;
  ret->completion = false;
  ret->progress = 0;
  ret->progress_str[0] = '\0';
  ret->error = 0;
  ret->thread_args = NULL;
  return ret;
}



