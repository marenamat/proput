/*
 *	ProPuT
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/fs.h>
#include <linux/init.h>
#include <asm/uaccess.h>

#include "proput.h"

struct proput_bitstream {
  unsigned short len;
  unsigned short pos;
  unsigned int repeat;
  unsigned char data[0];
};

#define READ_BUF_SZ	256
struct ufp_context {
  struct ufp_context *prev, *next;
  struct poll_table_struct *poll_read_pending;
  wait_queue_head_t read_queue;
  struct semaphore sem;
  u16 read_end;
  char read_buf[READ_BUF_SZ];
};

DEFINE_MUTEX(ufp_mutex);

static __poll_t ufp_poll(struct file *fp, struct poll_table_struct *pts)
{
  struct ufp_context *ctx = fp->private_data;

  if (down_interruptible(&ctx->sem))
    return -ERESTARTSYS;

  if (ctx->read_end > 0)
  {
    up(&ctx->sem);
    return POLLWR | POLLRD;
  }
  
  else if (ctx->poll_read_pending)
  {
    up(&ctx->sem);
    return -EBUSY;
  }

  ctx->poll_read_pending = pts;
  up(&ctx->sem);
  return POLLWR;
}

static ssize_t ufp_read(struct file *fp, char __user *buf, size_t sz, loff_t *off)
{
  struct ufp_context *ctx = fp->private_data;

  if (sz < sizeof(struct proput_response_header))
    return -EINVAL;

  if (down_interruptible(&ctx->sem))
    return -ERESTARTSYS;

  if (ctx->read_end == 0)
  {
    if (fp->f_flags & O_NONBLOCK)
      return -EAGAIN;

    up(&ctx->sem);

    if (wait_event_interruptible(ctx->read_queue, (ctx->read_end > 0)))
      return -ERESTARTSYS;

    if (down_interruptible(&ctx->sem))
      return -ERESTARTSYS;
  }

  char *response;
  uint16_t len;

  if (ctx->read_end > sz)
  {
    struct proput_response_header prh = {
      .response = PROPUT_R_BUF,
      .len = ctx->read_end,
    };

    response = &prh;
    len = sizeof(struct proput_response_header);
  }
  else
  {
    response = ctx->read_buf;
    len = ctx->read_end;
    ctx->read_end = 0;
  }

  if (copy_to_user(buf, response, len)) {
    up(&ctx->sem);
    return -EFAULT;
  }

  up(&ctx->sem);
  return len;
}

static ssize_t ufp_write(struct file *fp, const char __user *buf, size_t sz, loff_t *off)
{
}

static int ufp_open(struct inode *inode, struct file *file)
{
  if (ufp_connected)
    return -EBUSY;

  ufp_connected = 1;
  try_module_get(THIS_MODULE);
  return 0;
}

static int ufp_release(struct inode *inode, struct file *file)
{
  if (!ufp_connected)
    return -EIO;

  ufp_connected = 0;
  module_put(THIS_MODULE);
  return 0;
}

static struct file_operations ufp = {
  .read = ufp_read,
  .write = ufp_write,
  .open = ufp_open,
  .release = ufp_release,
};

static int ufp_major;

int __init proput_init(void)
{
  ufp_major = register_chrdev(0, "proput", &ufp);
  if (ufp_major < 0)
  {
    printk(KERN_ALERT "Failed to register proput device: %d\n", ufp_major);
    return ufp_major;
  }

  printk(KERN_INFO "ProPuT inserted\n");
  return 0;
}

void __exit proput_exit(void)
{
  unregister_chrdev(ufp_major, "proput");
  printk(KERN_INFO "ProPuT removed\n");
}

module_init(proput_init);
module_exit(proput_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maria Matejka <mq@jmq.cz>");
MODULE_DESCRIPTION("ProPuT");
//MODULE_SUPPORTED_DEVICE("proput");

