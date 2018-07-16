#!/bin/bash

if [ "$2" = "" ]; then
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1
else
 java -jar lib/jenkins-cli.jar -s "$ROLLAUT_JENKINS_URL" -http -auth "$ROLLAUT_JENKINS_AUTH"  -noKeyAuth build $1 -p $2=$3
fi
