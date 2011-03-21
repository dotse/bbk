#ifndef _BATCH_H_
#define _BATCH_H_

#include "executor.h"

struct batch_arg_struct {
  /* parameters */
  struct thread_arg_struct **thread_args;
  unsigned int number_thread_args;
};

struct batch_arg_struct * new_batch_arg_struct();
void batch_add_thread_arg(struct batch_arg_struct *ba, struct thread_arg_struct *ta);
void delete_batch_arg_struct(struct batch_arg_struct *p);
void * batch_executor(void *arg);

#endif
