
#!/bin/bash

echo 

LD_LIBRARY_PATH=$(pwd)/bin:$LD_LIBRARY_PATH ceps \
 ../features/common/common.ceps \
 $1 \
 --pluginlibINSERT_PLUGIN_NAME_HERE.so	--doc-option no-macros --pe --format markdown_github 	
					