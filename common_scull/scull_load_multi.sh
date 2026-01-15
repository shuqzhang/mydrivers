#!/bin/sh
source $(pwd)/common_functions.sh
module=$1
device=$2
mode="666"

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

is_loaded=$(is_module_loaded $1)

install_module $is_loaded $1
echo "bbbb"

major=`cat /proc/devices | awk "\\$2==\"$device\" {print \\$1}"`

minor=0
nr_devices=$3

while [ $minor -lt $nr_devices ]
do
    echo "minor=$minor"
    mknod /dev/${device}$minor c $major $minor
    minor=$(expr $minor + 1)
done

ln -sf /dev/${device}0  /dev/${device}  # file is not linked to device, but changed to be another file after "cp [file] /dev/scull", why?
echo "cccc"

# give appropriate group/permissions
# chgrp $group /dev/${device}[0-3]
chmod $mode  /dev/${device}[0-$minor]
