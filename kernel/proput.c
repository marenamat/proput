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

static _Bool ufp_connected = 0;

struct proput_bitstream {
  unsigned short len;
  unsigned short pos;
  unsigned int repeat;
  unsigned char data[0];
};

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
//  .read = ufp_read,
//  .write = ufp_write,
  .open = ufp_open,
  .release = ufp_release,
};

static int ufp_major;

int __init proput_init(void)
{
  printk(KERN_INFO "ProPuT inserted\n");
  ufp_major = register_chrdev(0, "proput", &ufp);
  if (ufp_major < 0)
  {
    printk(KERN_ALERT "Failed to register proput device: %d\n", ufp_major);
    return ufp_major;
  }

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
MODULE_SUPPORTED_DEVICE("proput");

