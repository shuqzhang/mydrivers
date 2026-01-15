#!/bin/sh
source $(pwd)/common_functions.sh

module=$1
device=$2

# remove nodes
nr_devices=$3
max_minor=$(expr $nr_devices - 1)
rm -f /dev/${device}[0-$max_minor] /dev/${device}

# invoke rmmod with all arguments we got
is_loaded=$(is_module_loaded $1)

uninstall_module $is_loaded $1

exit 0
