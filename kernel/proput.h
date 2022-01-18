/*
 *	ProPuT
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef PROPUT_H_INCLUDED_
#define PROPUT_H_INCLUDED_

#define PROPUT_TYPE_SEND_BITS	  1

struct proput_send_bits {
  unsigned short type;
  unsigned short len;
  unsigned int repeat;
  unsigned char data[0];
};

#endif
