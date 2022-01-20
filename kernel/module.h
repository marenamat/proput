/*
 *	ProPuT internal API
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef PROPUT_MODULE_H_INCLUDED_
#define PROPUT_MODULE_H_INCLUDED_

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/types.h>
#include <linux/errno.h>
#include <linux/mutex.h>
#include <linux/slab.h>
#include <asm/uaccess.h>

#include "proput.h"

struct ufp_context;

int ufp_init(void);
void ufp_exit(void);

int ufp_response(struct ufp_context *ctx, struct proput_response_header *hdr);

#define PROPUT_OPS \
  OP(PROPUT_C_DEVLIST, cmd_devlist)

#define OP_DEF(fun) int fun(struct ufp_context *ctx, struct proput_cmd_header *hdr, const char __user *buf, size_t sz)

#define OP(cmd, fun) OP_DEF(fun);
PROPUT_OPS
#undef OP

#define DBG(...)  printk(KERN_INFO "ProPuT: " __VA_ARGS__)
#define ERR(...)  printk(KERN_ERR "ProPuT: (E)" __VA_ARGS__)

#endif
