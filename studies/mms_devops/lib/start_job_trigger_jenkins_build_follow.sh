#!/bin/bash

if [ "$2" = "" ]; then
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1 -f
elif [ "$2" != "" ] && [ "$4" = "" ]; then
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1 -f -p $2="$3"
elif [ "$2" != "" ] && [ "$4" != "" ] && [ "$6" = "" ]; then
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1 -f -p $2="$3" -p $4="$5"
else  
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1 -f -p $2=$3 -p $4="$5" -p $6="$7"
fi
