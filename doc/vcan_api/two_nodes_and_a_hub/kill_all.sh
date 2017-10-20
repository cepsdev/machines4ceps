kill $(ps -eo pid,fname | grep ceps | cut -f2 -d" ")
