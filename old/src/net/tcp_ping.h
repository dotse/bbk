#ifndef _TCP_PING_H_
#define _TCP_PING_H_

#include "valuestruct.h"

struct tcp_ping_arg_struct {
  /* parameters */
  unsigned int timeout_ms;
  unsigned int number_hosts;
  in_addr *hostlist;
  unsigned short *portlist;
  /* general results */
  unsigned int connects;
  double max_connect_time;
  double min_connect_time;
  double avg_connect_time;
  /* individual results */
  struct valuestruct *values;
  unsigned int no_values;
  void *userdata;
};

struct tcp_ping_arg_struct * new_tcp_ping_arg_struct();
void delete_tcp_ping_arg_struct(struct tcp_ping_arg_struct *p);
void * tcp_ping_executor(void *arg);

#endif
