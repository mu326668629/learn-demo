/*
 * Author: yanglc
 * Date  : 2018-07-25
 */
#ifndef __ETIMER_H__
#define __ETIMER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "rbtree.h"

#define ETIMER_INFINITE   (rbtree_key_t)-1

typedef struct etimer_node_s etimer_node_t;

struct etimer_node_s {
    rbtree_node_t timer;
    unsigned char timer_set; /* 是否已经添加到rbtree */
    void (*handler) (etimer_node_t *node);
    void *data;
};

/* 获取当前时间毫秒 */
rbtree_key_t etimer_get_msec();

/* 定时器初始化  */
int          etimer_init();

/* 挂起服务，等待定时器 */
int          etimer_start();

/* 获取定时器最小过期时间 */
rbtree_key_t etimer_find_timer();

/* 执行已经到期的定时器 */
void          etimer_expire_timers();

/* 删除定时器 */
void          etimer_del_timer(etimer_node_t *e);

/* 添加定时器 */
void          etimer_add_timer(etimer_node_t *e, rbtree_key_t timer);

#ifdef __cplusplus
}
#endif

#endif