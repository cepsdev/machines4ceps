
if [ "$2" == "NO_ENV" ]; then
 echo ""
else
 source set_env.sh
fi
./ceps "$1/ceps.ceps" --plugin./libjenkins4ceps.so --server
