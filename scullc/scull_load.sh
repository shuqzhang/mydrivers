#!/bin/sh
chmod +x common_functions.sh
chmod +x scull_load_multi.sh
chmod +x $(pwd)/../misc_progs/*
./scull_load_multi.sh scullc scullc 4
