/*
 *	ProPuT -- Userspace hooks
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include "module.h"

#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/poll.h>

static struct ufp_dev {
  struct ufp_context *ctx_first;
  struct mutex lock;
  dev_t id;
  struct cdev cdev;
} ufp_dev;

#define MAX_READ_BUF_SIZE   256

struct ufp_context {
  struct ufp_context *prev, *next;
  wait_queue_head_t read_queue;
  struct mutex lock;
  char *read_buf;
  u16 read_end;
};

DEFINE_MUTEX(ufp_mutex);

int ufp_response(struct ufp_context *ctx, struct proput_response_header *hdr)
{
  mutex_lock(&ctx->lock);
  if (ctx->read_end)
  {
    mutex_unlock(&ctx->lock);
    return -EBUSY;
  }

  if (hdr->len > MAX_READ_BUF_SIZE)
  {
    mutex_unlock(&ctx->lock);
    return -ENOBUFS;
  }

  memcpy(ctx->read_buf, hdr, hdr->len);
  ctx->read_end = hdr->len;

  wake_up(&ctx->read_queue);

  return 0;
}

static __poll_t ufp_poll(struct file *fp, struct poll_table_struct *wait)
{
  struct ufp_context *ctx = fp->private_data;
  __poll_t mask = POLLOUT | POLLWRNORM;

  if (mutex_lock_interruptible(&ctx->lock))
    return -ERESTARTSYS;

  poll_wait(fp, &ctx->read_queue, wait);

  if (ctx->read_end > 0)
    mask |= POLLIN | POLLRDNORM;

  mutex_unlock(&ctx->lock);
  return mask;
}

static ssize_t ufp_read(struct file *fp, char __user *buf, size_t sz, loff_t *off)
{
  struct ufp_context *ctx = fp->private_data;

  void *response;
  uint16_t len;

  if (!ctx)
    return -EINVAL;

  if (sz < sizeof(struct proput_response_header))
    return -EINVAL;

  if (mutex_lock_interruptible(&ctx->lock))
    return -ERESTARTSYS;

  if (ctx->read_end == 0)
  {
    if (fp->f_flags & O_NONBLOCK)
    {
      mutex_unlock(&ctx->lock);
      return -EAGAIN;
    }

    mutex_unlock(&ctx->lock);

    if (wait_event_interruptible(ctx->read_queue, (fp->private_data != ctx) || (ctx->read_end > 0)))
      return -ERESTARTSYS;

    if (mutex_lock_interruptible(&ctx->lock))
      return -ERESTARTSYS;
  }

  if (ctx->read_end > sz)
  {
    struct proput_response_header hdr = {
      .response = PROPUT_R_BUF,
      .len = ctx->read_end,
    };

    response = &hdr;
    len = sizeof(struct proput_response_header);
  }
  else
  {
    response = ctx->read_buf;
    len = ctx->read_end;
    ctx->read_end = 0;
  }

  if (copy_to_user(buf, response, len)) {
    mutex_unlock(&ctx->lock);
    return -EFAULT;
  }

  mutex_unlock(&ctx->lock);
  return len;
}

static ssize_t ufp_write(struct file *fp, const char __user *buf, size_t sz, loff_t *off)
{
  struct proput_cmd_header hdr;

  if (sz < sizeof(struct proput_cmd_header))
    return -EINVAL;

  if (copy_from_user(&hdr, buf, sizeof(struct proput_cmd_header)))
    return -EFAULT;

  if (sz != hdr.len)
    return -EINVAL;

  switch (hdr.cmd) {
#define OP(cmd, fun)  case cmd: return fun(fp->private_data, &hdr, buf, sz);
    PROPUT_OPS
#undef OP

    default:
      return -EINVAL;
  }
}

static int ufp_open(struct inode *inode, struct file *file)
{
  struct ufp_context *ctx;

  if (inode->i_cdev != &ufp_dev.cdev)
    return -EINVAL;

  if (!try_module_get(THIS_MODULE))
    return -EINVAL;

  ctx = kmalloc(sizeof(struct ufp_context), GFP_KERNEL);
  if (!ctx)
  {
    module_put(THIS_MODULE);
    return -ENOMEM;
  }

  *ctx = (struct ufp_context) {};

  mutex_init(&ctx->lock);
  init_waitqueue_head(&ctx->read_queue);

  ctx->read_buf = kmalloc(MAX_READ_BUF_SIZE, GFP_KERNEL);
  if (!ctx->read_buf)
  {
    kfree(ctx);
    module_put(THIS_MODULE);
    return -ENOMEM;
  }

  mutex_lock(&ufp_dev.lock);

  if ((ctx->next = ufp_dev.ctx_first))
    ctx->next->prev = ctx;

  mutex_unlock(&ufp_dev.lock);

  file->private_data = ctx;

  return 0;
}

static int ufp_release(struct inode *inode, struct file *file)
{
  struct ufp_context *ctx = file->private_data;

  if (inode->i_cdev != &ufp_dev.cdev)
    return -EINVAL;

  if (!ctx)
    return -EINVAL;

  if (mutex_lock_interruptible(&ctx->lock))
    return -ERESTARTSYS;

  if (ctx->read_buf)
  {
    kfree(ctx->read_buf);
    ctx->read_buf = NULL;
  }

  mutex_lock(&ufp_dev.lock);

  if (ctx->prev)
    ctx->prev->next = ctx->next;
  else
    ufp_dev.ctx_first = ctx->next;

  if (ctx->next)
    ctx->next->prev = ctx->prev;

  mutex_unlock(&ufp_dev.lock);

  file->private_data = NULL;
  kfree(ctx);

  module_put(THIS_MODULE);
  return 0;
}

static struct file_operations ufp_ops = {
  .owner = THIS_MODULE,
  .read = ufp_read,
  .write = ufp_write,
  .poll = ufp_poll,
  .open = ufp_open,
  .release = ufp_release,
};

int ufp_init(void)
{
  int e;

  mutex_init(&ufp_dev.lock);

  if ((e = alloc_chrdev_region(&ufp_dev.id, 0, 1, "proput")))
  {
    printk(KERN_ALERT "Failed to register proput device: %d\n", e);
    return e;
  }

  cdev_init(&ufp_dev.cdev, &ufp_ops);
  ufp_dev.cdev.owner = THIS_MODULE;
  
  if ((e = cdev_add(&ufp_dev.cdev, ufp_dev.id, 1)))
  {
    printk(KERN_ALERT "Failed to add proput device: %d\n", e);
    unregister_chrdev_region(ufp_dev.id, 1);
    return e;
  }

  return 0;
}

void ufp_exit(void)
{
  cdev_del(&ufp_dev.cdev);
  unregister_chrdev_region(ufp_dev.id, 1);
}
