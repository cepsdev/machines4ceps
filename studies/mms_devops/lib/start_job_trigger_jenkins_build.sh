#!/bin/bash

if [ "$2" = "" ]; then
 java -jar lib/jenkins-cli.jar -s http://localhost:8080 -http -auth "tomas:lAKtat37,"  -noKeyAuth build $1
else
 java -jar lib/jenkins-cli.jar -s http://localhost:8080 -http -auth "tomas:lAKtat37,"  -noKeyAuth build $1 -p $2=$3
fi
