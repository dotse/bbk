#include "batch.h"

#ifdef UNIX
#include <unistd.h>
#endif

#include <string.h>

struct batch_arg_struct * new_batch_arg_struct() {
	struct batch_arg_struct *ba;
	ba = (struct batch_arg_struct *)
		malloc(sizeof(struct batch_arg_struct));
	ba->number_thread_args = 0;
	ba->thread_args = NULL;
	return ba;
}

void batch_add_thread_arg(struct batch_arg_struct *ba, struct thread_arg_struct *ta) {
	ba->number_thread_args += 1;
	ba->thread_args = (struct thread_arg_struct **)
		realloc(ba->thread_args, sizeof(struct thread_arg_struct *) * 
		ba->number_thread_args);
	(ba->thread_args)[ba->number_thread_args - 1] = ta;
}

void delete_batch_arg_struct(struct batch_arg_struct *ba) {
	if (ba == NULL) return;
	if (ba->thread_args != NULL)
		free(ba->thread_args);
	free(ba);
}


void * batch_executor(void *arg) {
	unsigned int i;
	struct batch_arg_struct *ba;
	struct thread_arg_struct *t;
#ifdef UNIX
	pthread_t mythread;
#endif
#ifdef WIN32
	DWORD mythread;
	HANDLE hThread;
#endif

	t = (struct thread_arg_struct *)arg;
    ba = (struct batch_arg_struct *)t->thread_args;

	t->progress = 0;
	t->completion = false;
	t->executing = true;
	t->started = true;

	for (i = 0; i < ba->number_thread_args; i++) {

		struct thread_arg_struct *ta = ba->thread_args[i];

		t->progress = (unsigned char) 
			((((int)(i)) * 100) / (ba->number_thread_args));

#ifdef UNIX
		/* create executor thread, specify tcp_ping_executor as thread function */
		if (pthread_create(&mythread, NULL, ta->start_routine, (void *)ta) != 0) {
			continue;
		}
#endif
#ifdef WIN32
		if ((hThread = CreateThread( NULL, 0, ta->start_routine, (PVOID)ta, 0, &mythread)) == NULL) {
			continue;
		}
#endif

		/* First wait for thread to start execution */
		while (ta->started == false) {
			/* Check if we should hibernate */
			while (t->execute == false) {
				t->executing = ta->executing = false;
				/* Check if we should die during hibernation */
				if (t->die == true) {
					ta->die = true;
					while (ta->executing == true);
					t->completion = false;
					t->executing = false;
					thread_exit();
				}
				usleep(100000);
			}
			t->executing = true;
			usleep(50000);
		}
		t->executing = true;

		/* Then wait for it to finish executing */  
		while (ta->executing == true) {

			/* Check if we should die */
			if (t->die == true) {
				ta->die = true;
				while (ta->executing == true);
				t->completion = false;
				t->executing = false;
				thread_exit();
			}
		
			/* update progress counter */
			t->progress = (unsigned char)
				( ((((float)i) + (((float)(ta->progress)) / 100.0)) /
				  ((float)(ba->number_thread_args))) * 100.0 );

			/* update progress string */
			strcpy(t->progress_str, ta->progress_str);
				
			/* Check if we should hibernate */
			while (t->execute == false) {
				t->executing = ta->executing = false;
				/* Check if we should die during hibernation */
				if (t->die == true) {
					ta->die = true;
					while (ta->executing == true);
					t->completion = false;
					t->executing = false;
					thread_exit();
				}
				usleep(100000);
			}
			t->executing = true;
			usleep(50000);
		}

	}

#ifdef ENGLISH
	strcpy(t->progress_str, "All tests finished");
#endif
#ifdef SWEDISH
	strcpy(t->progress_str, "Alla test färdiga");
#endif

  t->progress = 100;
  t->completion = true;
  t->executing = false;
  thread_exit();
  return 0;

}
