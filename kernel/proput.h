/*
 *	ProPuT public API
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef PROPUT_H_INCLUDED_
#define PROPUT_H_INCLUDED_

#define PROPUT_CMD_VAL(x, y)   ((x << 8) + y)

struct proput_cmd_header {
  uint16_t cmd;
  uint16_t len;
  unsigned char data[0];
};

struct proput_response_header {
  uint16_t response;
  uint16_t len;
  unsigned char data[0];
};

#define PROPUT_C_DEVLIST	PROPUT_CMD_VAL('D', 'L')

#define PROPUT_R_DEVLIST	PROPUT_CMD_VAL('D', 'L')
#define PROPUT_R_BUF		PROPUT_CMD_VAL('B', 'u')

#endif
