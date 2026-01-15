#!/bin/bash

function is_module_loaded()
{
    module_lists=$(lsmod | grep $1 | awk '{print $1}')
    is_loaded=0
    for item in $module_lists
    do
        if [ "$item" = "$1" ]; then
            is_loaded=$(expr $is_loaded + 1)
        fi
    done
    echo $is_loaded
    return 0
}

function install_module()
{
    expect_num=1
    is_loaded=$1
    module=$2
    module_dir=$2
    if [ $is_loaded -gt $expect_num ]; then
        echo "There are more than one modules names with $module. need fix scripts"
    elif [ $is_loaded -eq $expect_num ]; then
        echo "$module installed. skip this step"
    else
        /sbin/insmod ../$module_dir/$module.ko || return 1
    fi
    return 0
}

function uninstall_module()
{
    expect_num=1
    is_loaded=$1
    module=$2
    if [ $is_loaded -gt $expect_num ]; then
        echo "There are more than one modules names with $module. need fix scripts"
    elif [ $is_loaded -eq $expect_num ]; then
        /sbin/rmmod $module || return 1
    else
        echo "$module installed. skip this step"
    fi
    return 0
}
