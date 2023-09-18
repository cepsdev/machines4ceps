
#!/bin/bash

echo 

LD_LIBRARY_PATH=$(pwd)/bin:$LD_LIBRARY_PATH  ceps ../features/common/common.ceps ../features/common/gherkin.ceps.lex $1 \
 ../features/common/summary.ceps --pe --format ansi  --pluginlibINSERT_PLUGIN_NAME_HERE.so