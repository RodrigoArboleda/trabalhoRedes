#include <time.h>
#include <pthread.h>
#include <stdio.h>
/* for ETIMEDOUT */
#include <errno.h>
#include <string.h>

pthread_mutex_t calculating = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t done = PTHREAD_COND_INITIALIZER;

void *expensive_call(void *data)
{
        int oldtype;

        /* allow the thread to be killed at any time */
        pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, &oldtype);

        /* ... calculations and expensive io here, for example:
         * infinitely loop
         */
        for (;;) {}

        /* wake up the caller if we've completed in time */
        pthread_cond_signal(&done);
        return NULL;
}

/**/
int exec_n_segundos(int n, void *(*funcao) (void *)){
    struct timespec abs_time;
    pthread_t tid;

    pthread_mutex_lock(&calculating);

    /* pthread cond_timedwait expects an absolute time to wait until */
    clock_gettime(CLOCK_REALTIME, &abs_time);
    abs_time.tv_sec += n;

    pthread_create(&tid, NULL, funcao, NULL);

    /* pthread_cond_timedwait can return spuriously: this should
    * be in a loop for production code
    */
    int err = pthread_cond_timedwait(&done, &calculating, &abs_time);
    printf("xe\n");
    pthread_mutex_unlock(&calculating);

    return err;
}


int main()
{
        exec_n_segundos(3,expensive_call);
        printf("xd\n");
        return 0;
}