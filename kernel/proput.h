/*
 *	ProPuT public API
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#ifndef PROPUT_H_INCLUDED_
#define PROPUT_H_INCLUDED_

#define PROPUT_MAX_BUF_SIZE	256

#define PROPUT_CMD_VAL(x, y)   ((y << 8) + x)

/* All TLD lengths include header size */

/* Generic command header, beginning of every message. */
struct proput_cmd_header {
  uint16_t cmd;
  uint16_t len;
  uint32_t data[0];
};

/* Generic response header, beginning of every message. */
struct proput_response_header {
  uint16_t response;
  uint16_t len;
  uint32_t data[0];
};

/* Request device list. Empty data. */
#define PROPUT_C_DEVLIST	PROPUT_CMD_VAL('D', 'L')

/* Send device list consisting of several devices headed by struct proput_device_header */
#define PROPUT_R_DEVLIST	PROPUT_CMD_VAL('D', 'L')

struct proput_device_header {
  uint16_t type;
  uint16_t len;
  uint32_t data[0];
};

/* A generic device type setting one bit for every available but unused pin */
#define PROPUT_DEVICE_TYPE_UNUSED	0x0001
struct proput_device_unused_pins {
  struct proput_device_header dh;
  uint32_t pins[2]; /* At most 64 pins */
};

#define PROPUT_R_BUF		PROPUT_CMD_VAL('B', 'u')

#endif
