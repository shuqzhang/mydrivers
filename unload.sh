#/bin/bash

(cd common_scull && chmod +x scull_unload_multi.sh)
(cd common_scull && ./scull_unload_multi.sh scull scull 4)
(cd common_scull && ./scull_unload_multi.sh scull scull_p 4)
(cd common_scull && ./scull_unload_multi.sh scull scull_single 1)
(cd common_scull && ./scull_unload_multi.sh scull scull_uid 1)
(cd common_scull && ./scull_unload_multi.sh scull scull_wuid 1)
(cd common_scull && ./scull_unload_multi.sh scull scull_copy 1)
(cd common_scull && ./scull_unload_multi.sh scullc scullc 4)
./common_unloader.sh completion
./common_unloader.sh sleepy
./common_unloader.sh jit
./common_unloader.sh jiq
