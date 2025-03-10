#!/bin/sh
module=$1
device=$1

# invoke rmmod with all arguments we got
/sbin/rmmod $module || exit 1

# remove nodes
rm -f /dev/${device}0 /dev/${device}

exit 0
