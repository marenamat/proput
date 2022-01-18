#!/bin/bash

set -e
MAJOR=$(sed -rn 's/([0-9]+) proput$/\1/p' /proc/devices)
mknod -m a=rw /dev/proput c $MAJOR 0
