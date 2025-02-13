#/bin/bash

(cd scull && ./scull_unload.sh)
./common_unloader.sh completion
./common_unloader.sh sleepy
