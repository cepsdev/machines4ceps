kill $(ps -eo pid,fname | grep ceps | cut -f2 -d" ") 2> /dev/null
kill $(ps -eo pid,fname | grep ceps | cut -f1 -d" ") 2> /dev/null

