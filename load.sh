#/bin/bash

(cd scull && ./scull_load.sh)
(cd scullc && ./scull_load.sh)
./common_loader.sh completion completion
./common_loader.sh sleepy misc_modules
./common_loader.sh jit misc_modules
./common_loader.sh jiq misc_modules
