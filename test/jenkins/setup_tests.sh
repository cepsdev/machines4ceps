ceps_home="../../x86"
if [ ! -z $1 ]; then
 ceps_home="$1";
fi
jenkins_plugin="$ceps_home/../../bin/libjenkins4ceps.so"
ln $ceps_home/ceps ./ceps --symbolic -f
ln $jenkins_plugin ./libjenkins4ceps.so --symbolic -f

