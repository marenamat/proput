/*
 *	ProPuT -- Device manipulation
 *
 *	(c) 2022 Maria Matejka <mq@jmq.cz>
 *
 *	Can be freely distributed and used under the terms of the GNU GPL.
 */

#include "module.h"

OP_DEF(cmd_devlist)
{
  struct proput_response_header rh;

  if (sz != sizeof(*hdr))
    return -EINVAL;

  /* No devices yet to be listed */
  rh.response = PROPUT_R_DEVLIST;
  rh.len = sizeof(rh);

  return ufp_response(ctx, &rh);
}
