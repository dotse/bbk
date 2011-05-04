#include <sys/types.h>

#ifdef UNIX
	#include <sys/socket.h>
	#include <unistd.h>
	#include <netinet/in.h>
#endif

#ifdef WIN32
	#include "win32defs.h"
#endif

#include <stdio.h>
#include <errno.h>
#include <fcntl.h>

#include "tp_test.h"
#include "executor.h"
#include "valuestruct.h"
#include "tvutils.h"
#include "tpengine.h"
#include "tpclient.h"

void * tp_test_executor(void *arg) {
  struct TPEngine *engp;
  struct thread_arg_struct *t;
  int not_started;
  int selectedMode;
  time_t stime;

  t = (struct thread_arg_struct *)arg;
  engp = (struct TPEngine *)t->thread_args;

  t->progress = 0;
  t->completion = false;
  t->executing = true;
  t->started = true;

#ifdef SWEDISH
  strcpy(t->progress_str, "Startar TPTEST test");
#endif
#ifdef ENGLISH
  strcpy(t->progress_str, "Starting TPTEST test");
#endif

  selectedMode = engp->tpMode;
  engp->tpMode = CLM_NONE;

  while (1) {

    /* use AdvanceTest() to set test params and new test mode */
    engp->tpMode = AdvanceTest(engp, selectedMode, engp->tpMode, engp->failCode);
    if (engp->tpMode == CLM_NONE)
      break;

	/* update the progress string */
	switch (engp->tpMode) {
		case CLM_UDP_FDX:
#ifdef SWEDISH 
			sprintf(t->progress_str, "Kör UDP full duplex test mot %s",
				inet_ntoa(engp->hostIP));
#endif
#ifdef ENGLISH
			sprintf(t->progress_str, "Running UDP full duplex test vs %s",
				inet_ntoa(engp->hostIP));
#endif
			break;
		case CLM_UDP_SEND:
#ifdef SWEDISH
			sprintf(t->progress_str, "Kör UDP sändtest mot %s",
				inet_ntoa(engp->hostIP));
#endif
#ifdef ENGLISH
			sprintf(t->progress_str, "Running UDP send test vs %s",
				inet_ntoa(engp->hostIP));
#endif
			break;
		case CLM_UDP_RECV:
#ifdef SWEDISH
			sprintf(t->progress_str, "Kör UDP mottagningstest mot %s",
				inet_ntoa(engp->hostIP));
#endif
#ifdef ENGLISH
			sprintf(t->progress_str, "Running UDP receive test vs %s",
				inet_ntoa(engp->hostIP));
#endif
			break;
		case CLM_TCP_SEND:
#ifdef SWEDISH
			sprintf(t->progress_str, "Kör TCP sändtest (%d bytes) mot %s",
				engp->tcpBytes,
				inet_ntoa(engp->hostIP)
			);
			if (engp->tcpBytes >= 1024)
				sprintf(t->progress_str, "Kör TCP sändtest (%d kB) mot %s",
					engp->tcpBytes / 1024,
					inet_ntoa(engp->hostIP)
				);
			if (engp->tcpBytes >= (1024*1024))
				sprintf(t->progress_str, "Kör TCP sändtest (%d MB) mot %s",
					engp->tcpBytes / (1024*1024),
					inet_ntoa(engp->hostIP)
				);
#endif
#ifdef ENGLISH
			sprintf(t->progress_str, "Running %d-byte TCP send test vs %s",
				engp->tcpBytes, inet_ntoa(engp->hostIP));
#endif
			break;
		case CLM_TCP_RECV:
#ifdef SWEDISH
			sprintf(t->progress_str, "Kör TCP mottagningstest (%d bytes) mot %s",
				engp->tcpBytes,
				inet_ntoa(engp->hostIP)
			);
			if (engp->tcpBytes >= 1024)
				sprintf(t->progress_str, "Kör TCP mottagningstest (%d kB) mot %s",
					engp->tcpBytes / 1024,
					inet_ntoa(engp->hostIP)
				);
			if (engp->tcpBytes >= (1024*1024))
				sprintf(t->progress_str, "Kör TCP mottagningstest (%d MB) mot %s",
					engp->tcpBytes / (1024*1024),
					inet_ntoa(engp->hostIP)
				);
#endif
#ifdef ENGLISH
			sprintf(t->progress_str, "Running %d-byte TCP receive test vs %s",
				engp->tcpBytes, inet_ntoa(engp->hostIP));
#endif
			break;
	}


    /* initiate new test */
    if (StartClientContext(engp) != 0) {
      t->executing = false;
      t->error = engp->failCode;
	  thread_exit();
    }

    not_started = 1;

    t->progress = 0;

    /* run test until finished or an error occurs */
    while (engp->state != CLSM_FAILED && engp->state != CLSM_COMPLETE) {

      /* dying is important so check that here too */
      if (t->die == true) {
        StopContext(engp);
        t->completion = false;
        t->executing = false;
		thread_exit();
	  }

     if( engp->state == CLSM_FAILED )
	 {
		t->error = engp->failCode;
		strcpy( t->progress_str, "Felfelfel..." ); //engp->ctrlMessage );
	 }

      /* Check if we should hibernate */
      while (t->execute == false) {
        t->executing = false;
#ifdef SWEDISH
      sprintf(t->progress_str, "TPTEST test vilande");
#endif
#ifdef ENGLISH
      sprintf(t->progress_str, "TPTEST test paused");
#endif
        /* Check if we should die during hibernation */
        if (t->die == true) {
          StopContext(engp);
          t->completion = false;
          t->executing = false;
		  thread_exit();
		}
        usleep(100000);
      }
      t->executing = true;

      RunClientContext(engp);

      if (not_started) {
		  t->error = engp->failCode;
		  //strcpy( t->progress_str, engp->ctrlMessage );
        if (engp->state == CLSM_TESTLOOP) {
          not_started = 0;
          time(&stime);
        }
      }
      else {
		  switch (engp->tpMode) {
			  case M_UDP_SEND:
			  case M_UDP_RECV:
			  case M_UDP_FDX:
				  {
					time_t elapsed = time(NULL) - stime;
					t->progress = (unsigned char)((elapsed * 100) / engp->sessionTime);
					if (t->progress > 100)
						t->progress = 100;
				  }
				  break;
			  case M_TCP_SEND:
				  t->progress = (unsigned char)((engp->stats.BytesSent * 100) / engp->tcpBytes);
				  break;
			  case M_TCP_RECV:
				  t->progress = (unsigned char)((engp->stats.BytesRecvd * 100) / engp->tcpBytes);
				  break;
		  }
          if (t->progress > 100)
            t->progress = 100;
      }

    }

  }


  if( t->error == 0 )
  {
#ifdef ENGLISH
  strcpy(t->progress_str, "TPTEST test finished");
#endif
#ifdef SWEDISH
  strcpy(t->progress_str, "TPTEST test färdigt");
#endif
  }

  t->progress = 100;
  t->completion = true;
  t->executing = false;

  thread_exit();
  return 0;

}

