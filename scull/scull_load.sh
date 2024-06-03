#!/bin/sh
module="scull"
device="scull"
mode="664"

# Group: since distributions do it differently, look for wheel or use staff
if grep '^staff:' /etc/group > /dev/null; then
    group="staff"
else
    group="wheel"
fi

# remove stale nodes
rm -f /dev/${device}? 

# invoke insmod with all arguments we got
# and use a pathname, as newer modutils don't look in . by default
# /sbin/insmod -f ./$module.ko $* || exit 1  // invalid module format error
/sbin/insmod ./$module.ko $* || exit 1

major=`cat /proc/devices | awk "\\$2==\"$module\" {print \\$1}"`

mknod /dev/${device}0 c $major 0
mknod /dev/${device}1 c $major 1
mknod /dev/${device}2 c $major 2
mknod /dev/${device}3 c $major 3
ln -sf /dev/${device}0  /dev/${device}  # file is not linked to device, but changed to be another file after "cp [file] /dev/scull", why?

# give appropriate group/permissions
# chgrp $group /dev/${device}[0-3]
chmod $mode  /dev/${device}[0-3]