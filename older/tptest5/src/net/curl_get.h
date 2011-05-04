#ifndef _CURL_GET_H_
#define _CURL_GET_H_

#include "valuestruct.h"

struct curl_get_arg_struct {
  /* parameters */
  unsigned int number_urls;
  char **url_list;
  /* general results */
  unsigned int connects;
  double max_speed;
  double min_speed;
  double avg_speed;
  /* individual results */
  struct valuestruct *values;
  unsigned int no_values;
  void *userdata;
};

struct curl_get_arg_struct * new_curl_get_arg_struct();
void delete_curl_get_arg_struct(struct curl_get_arg_struct *p);
void * curl_get_executor(void *arg);
bool curl_download( char *url );

#endif
