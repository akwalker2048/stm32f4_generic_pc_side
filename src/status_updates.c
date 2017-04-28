#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include "status_updates.h"

void status_update(int signum)
{
 static int count = 0;
 /* printf ("timer expired %d times\n", ++count); */
}

void start_status_updates(void)
{
 struct sigaction sa;
 struct itimerval timer;

 /* Install timer_handler as the signal handler for SIGVTALRM. */
 memset (&sa, 0, sizeof (sa));
 sa.sa_handler = &status_update;
 sigaction (SIGALRM, &sa, NULL);

 /* Configure the timer to expire after 1 sec... */
 timer.it_value.tv_sec = 1;
 timer.it_value.tv_usec = 0;
 /* ... and every 1 sec after that. */
 timer.it_interval.tv_sec = 1;
 timer.it_interval.tv_usec = 0;
 /* Start a real timer. It expires based on the wall clock. */
 setitimer (ITIMER_REAL, &timer, NULL);

}
