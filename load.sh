#/bin/bash
(cd common_scull && chmod +x common_functions.sh)
(cd common_scull && chmod +x scull_load_multi.sh)
chmod +x misc_progs/*
(cd common_scull && ./scull_load_multi.sh scull scull 4)
(cd common_scull && ./scull_load_multi.sh scull scull_p 4)
(cd common_scull && ./scull_load_multi.sh scull scull_single 1)
(cd common_scull && ./scull_load_multi.sh scull scull_uid 1)
(cd common_scull && ./scull_load_multi.sh scull scull_wuid 1)
(cd common_scull && ./scull_load_multi.sh scull scull_copy 1)
(cd common_scull && ./scull_load_multi.sh scullc scullc 4)
./common_loader.sh completion completion
./common_loader.sh sleepy misc_modules
./common_loader.sh jit misc_modules
./common_loader.sh jiq misc_modules
