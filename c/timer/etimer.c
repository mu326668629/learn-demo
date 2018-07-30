/*
 * Author: yanglc
 * Date  : 2018-07-25
 */

#include <sys/time.h>
#include <signal.h>
#include <string.h>

#include "etimer.h"

static rbtree_t       etimer_rbtree;
static rbtree_node_t  etimer_sentinel;
static int            etimer_alarm;

static void etimer_signal_handler(int signo, siginfo_t *info, void *context);

struct etimer_signal_t {
    int signo;
    const char *signame;
    void     (*handler)(int, siginfo_t *, void *);
};

static struct etimer_signal_t etimer_signals[] = {
    { SIGALRM, "SIGALRM", etimer_signal_handler },

    {0, NULL, NULL}
};


static void etimer_signal_handler(int signo, siginfo_t *info, void *context)
{
    struct etimer_signal_t  *sig;

    for (sig = etimer_signals; sig->signo != 0; sig++) {
        if (sig->signo == signo) {
            break;
        }
    }

    switch (signo) {
        case SIGALRM:
        etimer_alarm = 1;
        break;
    }
}

static int etimer_signal_init()
{
    struct sigaction sa;
    struct etimer_signal_t  *sig;

    for (sig = etimer_signals; sig->signo != 0; sig++) {
        memset(&sa, sizeof(sa), 0);
        
        if (sig->handler) {
            sa.sa_sigaction = sig->handler;
            sa.sa_flags     = SA_SIGINFO;

        } else {
            sa.sa_handler = SIG_IGN;
        }
        sigemptyset(&sa.sa_mask);
        if (sigaction(sig->signo, &sa, NULL) == -1) {
            return -1;
        }
    }
    return 0;
}

rbtree_key_t etimer_get_msec()
{
    struct timeval   tv;

    gettimeofday(&tv, NULL);

#ifdef RBTREE_KEY_OVERFLOW
    return tv.tv_sec;
#else
    return tv.tv_sec * 1000 + tv.tv_usec / 1000;
#endif 
}

int etimer_init()
{
    rbtree_init(&etimer_rbtree, &etimer_sentinel,
                rbtree_insert_timer_value);
    return 0;
}

int etimer_start()
{
    sigset_t set;
    struct itimerval   itv;
    rbtree_key_t delay;

    if (etimer_signal_init() == -1) {
        return -1;
    }

    sigemptyset(&set);
    sigaddset(&set, SIGALRM);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        //TODO
    }

    sigemptyset(&set);

    for ( ;; ) {

        delay = etimer_find_timer();

        if (delay == ETIMER_INFINITE) {
            break;
        }
        
        itv.it_interval.tv_sec  = 0;
        itv.it_interval.tv_usec = 0;

        if (delay <= 0) {
            itv.it_value.tv_sec  = 0;
            itv.it_value.tv_usec = 500;
        } else {
            #ifdef RBTREE_KEY_OVERFLOW
            if (delay > 1) {
                delay = 1;
            }
            itv.it_value.tv_sec  = delay;
            itv.it_value.tv_usec = 0;
            #else
            if (delay > 500) {
                delay = 500;
            }
            itv.it_value.tv_sec  = delay / 1000;
            itv.it_value.tv_usec = (delay % 1000 ) * 1000;
            #endif
        }

        if (setitimer(ITIMER_REAL, &itv, NULL) == -1) {
            //TODO
        }

        sigsuspend(&set);

        if (etimer_alarm) {
            etimer_expire_timers();
            etimer_alarm = 0;
        }
    }

    return 0;
}

rbtree_key_t etimer_find_timer()
{
    rbtree_node_t   *node, *root, *sentinel;

    rbtree_key_t      etimer_current_msec;

    if (etimer_rbtree.root == &etimer_sentinel) {
        return ETIMER_INFINITE;
    }

    etimer_current_msec = etimer_get_msec();

    root     = etimer_rbtree.root;
    sentinel = etimer_rbtree.sentinel;

    node     = rbtree_min(root, sentinel);

    if (node->key > etimer_current_msec) {
        return (rbtree_key_int_t) (node->key - etimer_current_msec);
    }

    /* 不要返回负数，防止返回 -1 与 ETIMER_INFINITE 冲突*/
    return 0;
}

void etimer_expire_timers()
{
    etimer_node_t    *e;
    rbtree_node_t    *node, *root, *sentinel;

    rbtree_key_t      etimer_current_msec = etimer_get_msec();

    sentinel = etimer_rbtree.sentinel;

    for ( ;; ) {

        root = etimer_rbtree.root;

        if (root == sentinel) {
            return;
        }

        node = rbtree_min(root, sentinel);

        if (node->key > etimer_current_msec) {
            return;
        }

        rbtree_delete(&etimer_rbtree, node);

        e = (etimer_node_t*)node;

        e->timer_set = 0;

        e->handler(e);
    }
}

void  etimer_del_timer(etimer_node_t *e)
{
    rbtree_delete(&etimer_rbtree, &e->timer);
    e->timer_set = 0;
}

void  etimer_add_timer(etimer_node_t *e, rbtree_key_t timer)
{
    rbtree_key_t     etimer_current_msec = etimer_get_msec();

    if (e->timer_set) {
        etimer_del_timer(e);
    }

    #ifdef RBTREE_KEY_OVERFLOW
    e->timer.key = etimer_current_msec + timer / 1000;
    #else
    e->timer.key = etimer_current_msec + timer;
    #endif

    rbtree_insert(&etimer_rbtree, &e->timer);

    e->timer_set = 1;
}

#ifdef DEBUG

#include <stdio.h>
#include <unistd.h>

void handler(etimer_node_t *e)
{
    printf("%lu  e=%p\n", etimer_get_msec(), e);
    etimer_add_timer(e, 1000);
}

int main()
{
    etimer_node_t node;
    int i;

    node.handler = handler;
    //node.data    = 0x1;
    node.timer_set = 0;

    etimer_init();

    etimer_add_timer(&node, 4 * 1000);

    etimer_start();

    return 0;
}
#endif