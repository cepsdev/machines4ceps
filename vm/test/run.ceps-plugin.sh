
#!/bin/bash

echo

ceps $1	--pe --format ansi

echo 

LD_LIBRARY_PATH=$(pwd)/bin:$LD_LIBRARY_PATH ceps \
 $1 \
 --pluginlibINSERT_PLUGIN_NAME_HERE.so	--pe --format ansi				
					