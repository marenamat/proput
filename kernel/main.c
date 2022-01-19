/*
 *	ProPuT
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include "module.h"

#include <linux/init.h>

struct proput_bitstream {
  unsigned short len;
  unsigned short pos;
  unsigned int repeat;
  unsigned char data[0];
};

int __init proput_init(void)
{
  int e;

  if ((e = ufp_init()))
    return e;

  printk(KERN_INFO "ProPuT inserted\n");
  return 0;
}

void __exit proput_exit(void)
{
  ufp_exit();
  printk(KERN_INFO "ProPuT removed\n");
}

module_init(proput_init);
module_exit(proput_exit);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Maria Matejka <mq@jmq.cz>");
MODULE_DESCRIPTION("ProPuT");
//MODULE_SUPPORTED_DEVICE("proput");

