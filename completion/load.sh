#!/bin/sh
module="completion"
device="completion"
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

ln -sf /dev/${device}0  /dev/${device}  # file is not linked to device, but changed to be another file after "cp [file] /dev/completion", why?

# give appropriate group/permissions
chmod $mode  /dev/${device}0