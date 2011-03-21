// curltest.cpp : Defines the entry point for the console application.
//

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <string.h>
#include <iostream>
#include <tchar.h>
#include "curl.h"
#include "curl_get.h"
#include "executor.h"


char *newstr(char *s) {
  char *ret = (char *)malloc(strlen(s)+1);
  strcpy(ret, s);
  return ret;
}

int _tmain(int argc, _TCHAR* argv[])
{
	CURL *curl;
	CURLcode res;
	double spd;

	// thread arg struct and curl_get arg struct
	struct curl_get_arg_struct *ca = new_curl_get_arg_struct();
	struct thread_arg_struct *ta = new_thread_arg_struct();
	ta->thread_args = (void *)ca;

	HANDLE		m_thread;
	DWORD		m_threadid;
	LPTHREAD_START_ROUTINE m_test_executor = (LPTHREAD_START_ROUTINE)curl_get_executor;

	ca->number_urls = 3;
	ca->url_list = (char **) malloc(sizeof(char *) * ca->number_urls);
	ca->url_list[0] = newstr("ftp://ftp.sunet.se/pub/pictures/fractals/arches.gif");
	ca->url_list[1] = newstr("http://ftp.sunet.se/pub/os/FreeBSD/ports/i386/packages-4.11-release/mail/postfix-2.2.20041030,2.tgz");
	ca->url_list[2] = newstr("http://lonn.org/gurkburk.html");

	printf("Creating thread\n");

	m_thread = CreateThread( NULL, 0, m_test_executor, (PVOID)ta, 0, &m_threadid);

	while (ta->started != true);

	while (ta->executing == true) {
		printf("Progress: %d\n", ta->progress);
		Sleep(500);
	}
	printf("\nResults: %d/%d\n", ca->connects, ca->number_urls);
	printf("Speed min/avg/max: %.0f/%.0f/%.0f\n",
		ca->min_speed, ca->avg_speed, ca->max_speed);

	return 0;
}

