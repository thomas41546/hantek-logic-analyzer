/*
     _ _       _ _        _                 _       
  __| (_) __ _(_) |_ __ _| |  ___  ___   __| | __ _ 
 / _` | |/ _` | | __/ _` | | / __|/ _ \ / _` |/ _` |
| (_| | | (_| | | || (_| | | \__ \ (_) | (_| | (_| |
 \__,_|_|\__, |_|\__\__,_|_| |___/\___/ \__,_|\__,_|
         |___/                written by Ondra Havel

*/

#define _POSIX_C_SOURCE	201001L
#include <time.h>
#include <string.h>
#include "thread.h"
#include "io.h"
#include "local.h"

static volatile int fl_terminate = 0;
static volatile int fl_running = 0;


volatile unsigned int dso_period_usec;
volatile int dso_trigger_mode = TRIGGER_AUTO;

pthread_mutex_t buffer_mutex = PTHREAD_MUTEX_INITIALIZER;
static pthread_mutex_t thread_mutex = PTHREAD_MUTEX_INITIALIZER;


static
void __nsleep(const struct timespec *req)  
{  
    struct timespec rem;

    if(nanosleep(req,&rem)==-1) {
		while(nanosleep(&rem,&rem)==-1);
	}
}  

static
void myusleep(unsigned long usec)  
{
    struct timespec req;
    time_t sec = usec / 1000000;
    usec -= sec * 1000000;
    req.tv_sec = sec;  
    req.tv_nsec = usec * 1000;
    __nsleep(&req);
}

cb_fn my_cb = 0;

void dso_thread_set_cb(cb_fn cb)
{	// TODO: mutex
	if(!my_cb)
		my_cb = cb;
}

static
void *dso_thread(void *ptr)
{
	DMSG("DSO thread started\n");
    while(!fl_terminate) {
		if(!fl_running) {
			pthread_mutex_lock(&thread_mutex);	// wait on signal
			pthread_mutex_unlock(&thread_mutex);
			if(fl_terminate)
				return 0;
		}

		//DMSG("period = %d\n", dso_period_usec);

		if(my_cb) {
			my_cb();
			my_cb = 0;
		}

		dso_capture_start();
		myusleep(dso_period_usec);
		dso_trigger_enabled();

		int fl_complete = 0;
        int trPoint = 0;
		int nr_empty = 0;

		while(!fl_complete) {
			int cs = dso_get_capture_state(&trPoint);
			if (cs < 0) {
				DMSG("dso_get_capture_state io error\n");
				continue;
			}

			switch(cs) {
				case 0:	// empty
					if(nr_empty == 3) {
						dso_capture_start();
						nr_empty = 0;
					}
					nr_empty++;
					dso_trigger_enabled();
					dso_force_trigger();
					myusleep(dso_period_usec);
					break;

				case 1: // in progress
					myusleep(dso_period_usec >> 1);
					myusleep(dso_period_usec >> 1);
					break;

				case 2: // full
					pthread_mutex_lock(&buffer_mutex);
					if (dso_get_channel_data(dso_buffer, dso_buffer_size) < 0) {
						DMSG("Error in command GetChannelData\n");
					}
					dso_buffer_dirty = 1;
					dso_trigger_point = trPoint;
					pthread_mutex_unlock(&buffer_mutex);

					/*FORMERLEY UPDATE_GUI */
					fl_complete = 1;
					break;

				default:
					DMSG("unknown capture state %i\n", cs);
					break;
			}
		}
    }
	return 0;
}

#include <pthread.h>

static pthread_t my_thread;

void dso_thread_terminate()
{
	if(!fl_running)
		dso_thread_resume();
    fl_terminate = 1;

	pthread_join(my_thread, 0);
	DMSG("DSO thread terminated\n");
}

void dso_thread_init()
{
	if(dso_buffer)
		memset(dso_buffer, 0, sizeof(dso_buffer));

	pthread_mutex_lock(&thread_mutex);

	pthread_create(&my_thread, 0, &dso_thread, 0);
}

void dso_thread_resume()
{
	fl_running = 1;
	pthread_mutex_unlock(&thread_mutex);
}

void dso_thread_pause()
{
	pthread_mutex_lock(&thread_mutex);
	fl_running = 0;
}
