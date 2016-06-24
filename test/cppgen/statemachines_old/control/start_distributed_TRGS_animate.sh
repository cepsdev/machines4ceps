#bin/bash

DEBUG_MODE=Debug
RELEASE_MODE=Release

PATH_TO_TRGS_EXE=$TRGS_ROOT/code/simulation/build-TRGS_BAG-Desktop_Qt_5_5_0_GCC_64bit-$DEBUG_MODE
PATH_TO_MODELS=$TRGS_ROOT/statemachines/control
EXECUTABLE=TRGS_BAG
EXE=$TRGS_ROOT/utils/statemachines/x86
BASE_DEFS=$PATH_TO_MODELS
PROGRAMS=$TRGS_ROOT/statemachines/control/sm-programs
STATE=$TRGS_ROOT/statemachines/control/sm-state
DINGO=$TRGS_ROOT/statemachines/control/sm-dingo
TESTS=$TRGS_ROOT/statemachines/control/unittests

## Launch parameters for TRGS_BAG
#TRGS="$EXE/sm
TRGS="$PATH_TO_TRGS_EXE/$EXECUTABLE 
  $BASE_DEFS/base_defs.ceps
  $BASE_DEFS/bag_activator.ceps
  --rip127.0.0.1 --rport2003 --server --port2001
  --quiet"

  
## Launch parameters for DINGO
DINGO_LOCAL="$EXE/sm 
  $BASE_DEFS/base_defs.ceps
  $BASE_DEFS/dingo_definitions.ceps
  $BASE_DEFS/dingo_animation.ceps
  $BASE_DEFS/dingo_activator.ceps
  --server --port2003 --rip127.0.0.1 --rport2001
  --quiet"

  
## Launch parameters for DINGO
DINGO_REMOTE="$EXE/sm 
  $BASE_DEFS/base_defs.ceps
  $BASE_DEFS/dingo_definitions.ceps
  $BASE_DEFS/dingo_animation.ceps
  $BASE_DEFS/dingo_activator.ceps
  --server --port2003 --rip192.168.1.2 --rport2001
  --quiet"
  

if [ "$1" == "dingo_local" ]; then
  $DINGO_LOCAL
elif [ "$1" == "dingo_remote" ]; then
  $DINGO_REMOTE
elif [ "$1" == "trgs" ]; then
  $TRGS
else
  echo "call this script with {dingo_local; dingo_remote; trgs}"
  exit;
fi


#$TRGS
#$DINGO
  
  
  
  
# ## Launch parameters for shell setup
# #cmd="bash -c 'ls';bash"
# tab="--tab-with-profile=Default"
# title_bag="--title=TRGS_BAG"
# title_dingo="--title=DINGO"
# tabulator=""
# tabulator+=($tab -e "$TRGS"  $title_bag)         
# tabulator+=($tab -e "$DINGO" $title_dingo)         
# ## Launch ..
# gnome-terminal "${tabulator[@]}"
# #exit 0





#  command -v xdotool >/dev/null 2>&1 || { echo >&2 "This script requires 'xdotool' but it's not installed.  Aborting."; exit 1; }
  ### http://planet.jboss.org/post/opening_a_new_tab_in_an_existing_gnome_terminal_window
# ls
#   
# WID=$(xprop -root | grep "_NET_ACTIVE_WINDOW(WINDOW)"| awk '{print $5}')
# xdotool windowfocus $WID
# xdotool key ctrl+shift+t
# 
# xdotool sleep $DELAY # it may take a while to start new shell :(
# xdotool type --delay 1 --clearmodifiers "$@"
# xdotool key Return
# 
# wmctrl -i -a $WID
# 
# ls
# 
