#!/bin/bash

#################################################################
# functions
#################################################################
# function error_message {
#   echo "call this script with {x86; arm} {dingo; bag}"
#   echo "e.g. './launch_all.sh x86 dingo' to execute dingo simulation environment on x86"
# }

#################################################################
# context validation
#################################################################
if [ $# -lt 3 ]; then
    echo "call this script with {x86; arm} {can; vcan} {dingo; bag} [NO_GUI|GEN]"
    exit
fi

TRGS_ROOT="/home/tprerovs/projects/sm4ceps/test/cppgen"

if [ $1 == "x86" ]; then
    if [ -z ${TRGS_ROOT+x} ]; then 
        echo "TRGS_ROOT is unset"
        exit;
    else 
        echo -e "Environment variable 'TRGS_ROOT' is set to '$TRGS_ROOT'"
    fi
    
    echo "Configuring script for $1"
    PATH_TO_BAG_EXE=$TRGS_ROOT/bin/x86
#    PATH_TO_BAG_EXE=/home/cb/subversion/TRGS/code/simulation/build-TRGS_BAG-Desktop_Qt_5_5_1_GCC_64bit-Debug
    EXECUTABLE=TRGS_BAG
    EXE=$TRGS_ROOT/utils/statemachines/x86
    C_FUNCTIONS_4_CEPS=$TRGS_ROOT/statemachines/libs/x86
elif [ $1 == "arm" ]; then
    echo "Configuring script for $1"    
    TRGS_ROOT=/media/app/progs
    PATH_TO_BAG_EXE=$TRGS_ROOT
    EXECUTABLE=TRGS
    EXE=$TRGS_ROOT
    C_FUNCTIONS_4_CEPS=$TRGS_ROOT/statemachines/libs/arm
fi

if [ $1 == "x86" ] || [ $1 == "arm" ]; then
    BASE_DEFS=$TRGS_ROOT/statemachines/control
    MESSAGES=$TRGS_ROOT/statemachines/network/messages
    NETWORK=$TRGS_ROOT/statemachines/network
    PATH_TO_MODELS=$BASE_DEFS
    PROGRAMS=$TRGS_ROOT/statemachines/control/sm-programs
    STATE=$TRGS_ROOT/statemachines/control/sm-state
    DINGO_PATH=$TRGS_ROOT/statemachines/control/sm-dingo
fi

## ------------------------------------------------------------------------
## 				BAG
## ------------------------------------------------------------------------

if [ $# -gt 3 ]; then
    if [ $4 == "NO_GUI" ]; then
        echo -e "Launching statemachines without GUI ..\n"
        BAG_DEFINITIONS="
          $EXE/sm 
          $BASE_DEFS/base_defs.ceps"
    fi
    if [ $4 == "GEN" ]; then
        echo -e "Generating C++ Code ..\n"
        BAG_DEFINITIONS="
           $TRGS_ROOT/x86/sm  --cppgen 
           $BASE_DEFS/base_defs.ceps"
    fi    
else
    BAG_DEFINITIONS="
      $PATH_TO_BAG_EXE/$EXECUTABLE 
      $BASE_DEFS/base_defs.ceps"
fi
## ------------------------------------------------------------------------

BAG_SMS=" 
  $PROGRAMS/x_drive_disable.ceps
  $PROGRAMS/x_drive_enable.ceps
  $PROGRAMS/x_drive_brake_release.ceps
  $PROGRAMS/x_drive_brake_apply.ceps
  $PROGRAMS/x_drive_move.ceps
  $PROGRAMS/x_drive_reset_fault_antrieb.ceps
  $PROGRAMS/x_drive_to_indexposition.ceps
  $PROGRAMS/x_drive_to_target_position.ceps
  $PROGRAMS/x_drive_move_left_down.ceps
  $PROGRAMS/x_drive_move_right_down.ceps
  $PROGRAMS/y_drive_disable.ceps
  $PROGRAMS/y_drive_enable.ceps
  $PROGRAMS/y_drive_brake_release.ceps
  $PROGRAMS/y_drive_brake_apply.ceps
  $PROGRAMS/y_drive_move.ceps
  $PROGRAMS/y_drive_reset_fault_antrieb.ceps
  $PROGRAMS/y_drive_to_arbeitsposition.ceps
  $PROGRAMS/y_drive_to_indexposition.ceps
  $PROGRAMS/y_drive_to_target_position.ceps
  $PROGRAMS/y_drive_move_front_down.ceps
  $PROGRAMS/y_drive_move_rear_down.ceps
  $PROGRAMS/z_drive_disable.ceps
  $PROGRAMS/z_drive_enable.ceps
  $PROGRAMS/z_drive_brake_release.ceps
  $PROGRAMS/z_drive_brake_apply.ceps
  $PROGRAMS/z_drive_move.ceps
  $PROGRAMS/z_drive_reset_fault_antrieb.ceps
  $PROGRAMS/z_drive_to_index_position.ceps
  $PROGRAMS/z_drive_to_lashing_position.ceps
  $PROGRAMS/z_drive_to_target_position.ceps
  $PROGRAMS/z_drive_move_down.ceps
  $PROGRAMS/z_drive_move_up.ceps
  $PROGRAMS/ctrl.ceps
  $PROGRAMS/dispatch.ceps
  $PROGRAMS/antenna_lashing_lock.ceps
  $PROGRAMS/antenna_lashing_unlock.ceps
  $PROGRAMS/antenna_lock_preconditioning.ceps
  $PROGRAMS/antenna_to_hinge_down.ceps
  $PROGRAMS/antenna_to_level_out.ceps
  $PROGRAMS/antenna_to_parking_position.ceps
  $PROGRAMS/antenna_to_raise.ceps
  $PROGRAMS/dust_discharge_to_do.ceps
  $PROGRAMS/flap_close.ceps
  $PROGRAMS/flap_lock.ceps
  $PROGRAMS/flap_open.ceps
  $PROGRAMS/flap_precondition.ceps
  $PROGRAMS/flap_to_close.ceps
  $PROGRAMS/flap_to_open.ceps
  $PROGRAMS/flap_unlock.ceps
  $PROGRAMS/platform_extend.ceps
  $PROGRAMS/platform_extend_fto_and_alu.ceps
  $PROGRAMS/platform_retract.ceps
  $PROGRAMS/platform_retract_ftc_and_all.ceps
  $PROGRAMS/platform_retract_preconditioning.ceps
  $PROGRAMS/stilts_extend.ceps
  $PROGRAMS/stilts_lock.ceps
  $PROGRAMS/stilts_preconditioning.ceps
  $PROGRAMS/stilts_retract.ceps
  $PROGRAMS/stilts_to_extend.ceps
  $PROGRAMS/stilts_to_retract.ceps
  $PROGRAMS/stilts_unlock.ceps
  $PROGRAMS/system_extend.ceps
  $PROGRAMS/system_retract.ceps
  $STATE/antenna_alignment.ceps
  $STATE/antenna_lashing.ceps
  $STATE/drives_state.ceps
  $STATE/dust_discharge.ceps
  $STATE/extbg_progress_indicator.ceps
  $STATE/extbg_error_evaluator.ceps
  $STATE/flap.ceps
  $STATE/flap_lock_one.ceps
  $STATE/flap_lock_two.ceps
  $STATE/funktionsueberwachung.ceps
  $STATE/io_module_controler.ceps
  $STATE/ivenet_state.ceps
  $STATE/pneumatic_cylinder_pressure.ceps
  $STATE/stilt_left.ceps
  $STATE/stilt_right.ceps
  $STATE/stilts_vk_left.ceps
  $STATE/stilts_vk_right.ceps
  $STATE/user_buttons.ceps
  $STATE/warning_mast_operations.ceps
  $STATE/x_drive_fault_state.ceps
  $STATE/y_drive_fault_state.ceps
  $STATE/z_drive_fault_state.ceps
  $DINGO_PATH/talin.ceps"

## ------------------------------------------------------------------------

MESSAGES=" 
  $NETWORK/messages/messages_fue.ceps
  $NETWORK/messages/messages_drives.ceps
  $NETWORK/messages/messages_io_modul.ceps
  $NETWORK/messages/messages_si_modul.ceps
  $NETWORK/messages/messages_extbg.ceps
  $NETWORK/messages/messages_ivenet.ceps
  $NETWORK/messages/messages_sae.ceps"
  
## ------------------------------------------------------------------------
NETWORK_LAYER_BAG_CAN="
  $NETWORK/bag/channels-bag-can.ceps"
  
## ------------------------------------------------------------------------
NETWORK_LAYER_BAG_VCAN="
  $NETWORK/bag/channels-bag-vcan.ceps"
  
## ------------------------------------------------------------------------
NETWORK_LAYER_BAG_RX_TX="
  $NETWORK/bag/rx-bag.ceps
  $NETWORK/bag/rx-bag-sae.ceps
  $NETWORK/bag/tx-bag-ivenet.ceps
  $NETWORK/bag/tx-bag.ceps"

## ------------------------------------------------------------------------
if  [ "$4" == "quiet" ]; then
  OPTIONS=" --quiet " #" --quiet " #" --debug"
elif [ "$4" == "debug" ]; then
  OPTIONS=" --debug " #" --quiet " #" --debug"
else
##  OPTIONS=" --quiet" #" --quiet " #" --debug"
  OPTIONS=" " #" --quiet " #" --debug"
fi

## ------------------------------------------------------------------------
IGNITOR_BAG="
  $NETWORK/complete_activator_bag.ceps"
  

  
  
  
if [ $# -gt 3 ]; then
    if [ $4 == "GEN" ]; then
IGNITOR_BAG=" $NETWORK/gencpp.ceps"
    fi    
fi  
## ------------------------------------------------------------------------
PLUGIN=" --plugin$C_FUNCTIONS_4_CEPS/libc_functions.so"
if [ $# -gt 3 ]; then
    if [ $4 == "GEN" ]; then
PLUGIN=" "
    fi    
fi
## ------------------------------------------------------------------------
if [ $2 == "can" ]; then
    BAG=$BAG_DEFINITIONS$BAG_SMS$MESSAGES$NETWORK_LAYER_BAG_RX_TX$NETWORK_LAYER_BAG_CAN$IGNITOR_BAG$OPTIONS$PLUGIN
elif [ $2 == "vcan" ]; then
    BAG=$BAG_DEFINITIONS$BAG_SMS$MESSAGES$NETWORK_LAYER_BAG_RX_TX$NETWORK_LAYER_BAG_VCAN$IGNITOR_BAG$OPTIONS$PLUGIN
fi


## ------------------------------------------------------------------------
## 				     DINGO
## ------------------------------------------------------------------------

DINGO_DEFINITIONS="
  $EXE/sm 
  $BASE_DEFS/base_defs.ceps
  $BASE_DEFS/dingo_definitions.ceps
  $NETWORK/events.ceps"
## ------------------------------------------------------------------------

DINGO_SMS="
  $DINGO_PATH/antenna_endschalter_lock_VZ1O.ceps
  $DINGO_PATH/antenna_endschalter_lock_VZ1V.ceps
  $DINGO_PATH/antenna_valve_VZ1O.ceps
  $DINGO_PATH/antenna_valve_VZ1V.ceps
  $DINGO_PATH/dust_valve_VV1.ceps
  $DINGO_PATH/flap_endschalter_HR1O.ceps
  $DINGO_PATH/flap_endschalter_HR1V.ceps
  $DINGO_PATH/flap_endschalter_HR2O.ceps
  $DINGO_PATH/flap_endschalter_HR2V.ceps
  $DINGO_PATH/flap_endschalter_HZG.ceps
  $DINGO_PATH/flap_endschalter_HZO.ceps
  $DINGO_PATH/flap_position.ceps
  $DINGO_PATH/flap_lock_HR1.ceps
  $DINGO_PATH/flap_lock_HR2.ceps
  $DINGO_PATH/flap_valve_HR.ceps
  $DINGO_PATH/flap_valve_HVZO.ceps
  $DINGO_PATH/flap_valve_HVZON.ceps
  $DINGO_PATH/flap_valve_HVZS.ceps
  $DINGO_PATH/flap_valve_HVZSN.ceps
  $DINGO_PATH/funktionsueberwachung.ceps
  $DINGO_PATH/io_module_sim.ceps
  $DINGO_PATH/mast_endschalter_EU_Z.ceps
  $DINGO_PATH/mast_endschalter_EO_Z.ceps
  $DINGO_PATH/mast_endschalter_IP_Z.ceps
  $DINGO_PATH/mast_endschalter_UB_Z.ceps
  $DINGO_PATH/pneumatics.ceps
  $DINGO_PATH/stilts_endschalter_SEKGL.ceps
  $DINGO_PATH/stilts_endschalter_SEKGR.ceps
  $DINGO_PATH/stilts_endschalter_SEKOL.ceps
  $DINGO_PATH/stilts_endschalter_SEKOR.ceps
  $DINGO_PATH/stilts_endschalter_SMOL.ceps
  $DINGO_PATH/stilts_endschalter_SMOR.ceps
  $DINGO_PATH/stilts_endschalter_SMUL.ceps
  $DINGO_PATH/stilts_endschalter_SMUR.ceps
  $DINGO_PATH/stilts_position.ceps
  $DINGO_PATH/stilts_valve_SVK.ceps
  $DINGO_PATH/stilts_valve_SVZA.ceps
  $DINGO_PATH/stilts_valve_SVZE.ceps
  $DINGO_PATH/talin.ceps
  $DINGO_PATH/vehicle_sim.ceps
  $DINGO_PATH/x_drive_sim.ceps
  $DINGO_PATH/y_drive_sim.ceps
  $DINGO_PATH/z_drive_sim.ceps
  $DINGO_PATH/x_drive_endschalter_IP_X.ceps
  $DINGO_PATH/y_drive_endschalter_AB_Y.ceps
  $DINGO_PATH/y_drive_endschalter_IP_Y.ceps"
  
  
## ------------------------------------------------------------------------
NETWORK_LAYER_DINGO_CAN="
  $NETWORK/dingo/channels-dingo-can.ceps"
  
## ------------------------------------------------------------------------
NETWORK_LAYER_DINGO_VCAN="
  $NETWORK/dingo/channels-dingo-vcan.ceps"
  
## ------------------------------------------------------------------------
NETWORK_LAYER_DINGO_RX_TX="
  $NETWORK/dingo/rx-dingo.ceps
  $NETWORK/dingo/tx-dingo.ceps
  $NETWORK/dingo/tx-dingo-sae.ceps"
  
## ------------------------------------------------------------------------
IGNITOR_DINGO="
  $NETWORK/complete_activator_dingo.ceps"

## ------------------------------------------------------------------------
if [ $2 == "can" ]; then
    DINGO=$DINGO_DEFINITIONS$DINGO_SMS$MESSAGES$NETWORK_LAYER_DINGO_RX_TX$NETWORK_LAYER_DINGO_CAN$IGNITOR_DINGO$OPTIONS$PLUGIN
elif [ $2 == "vcan" ]; then
    DINGO=$DINGO_DEFINITIONS$DINGO_SMS$MESSAGES$NETWORK_LAYER_DINGO_RX_TX$NETWORK_LAYER_DINGO_VCAN$IGNITOR_DINGO$OPTIONS$PLUGIN
fi

## ------------------------------------------------------------------------

#################################################################
# context validation
#################################################################
if  [ "$3" == "dingo" ]; then
  echo "Starting .."
  $DINGO
elif [ "$3" == "bag" ]; then
  echo 
  echo "Starting .."
  echo $BAG 
  echo
  $BAG
else
    echo "call this script with {x86; arm} {dingo; bag} [NO_GUI]"
    exit
fi
