# #!/bin/bash

EXE=$TRGS_ROOT/utils/statemachines/x86
BASE_DEFS=$TRGS_ROOT/statemachines/control
PROGRAMS=$TRGS_ROOT/statemachines/control/sm-programs
STATE=$TRGS_ROOT/statemachines/control/sm-state
DINGO=$TRGS_ROOT/statemachines/control/sm-dingo
TESTS=$TRGS_ROOT/statemachines/control/unittests

#########################################
#    samples
#########################################

# ##    flap_close 
# $EXE/sm2plantuml \
# $BASE_DEFS/../fixtures_usage_example.ceps

# ## flap_close
# $EXE/sm2plantuml \
# $BASE_DEFS../guard_usage_example.ceps

# # ## sim_e
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/sim_flap_unlock/scenario_e.ceps

# # ## sim_e
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/sim_flap_unlock/scenario_exception.ceps

# # ## threads
# $EXE/sm2plantuml \
# $BASE_DEFS../threads.ceps

# # ## threads
# $EXE/sm2plantuml \
# $BASE_DEFS../threads_02.ceps

#########################################
#    FUNKTIONSUEBERWACHUNG
#########################################

# ## funktionsueberwachung io module
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_io_module_unittests.ceps

# ## funktionsueberwachung si module
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_si_module_unittests.ceps

# ## funktionsueberwachung heckverteiler
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_heckverteiler_unittests.ceps

# ## funktionsueberwachung usv
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_usv_unittests.ceps

# ## funktionsueberwachung drives (X,Y,Z)
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_drives_unittests.ceps

# ## funktionsueberwachung extbg
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_extbg_unittests.ceps

# ## funktionsueberwachung psm
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_psm_unittests.ceps

# ## funktionsueberwachung ivenet
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/funktionsueberwachung.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/funktionsueberwachung/fue_ivenet_unittests.ceps

#########################################
#    EXTBG
#########################################

# ## externes bediengeraet
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/extbg_error_evaluator.ceps \
# $TESTS/extbg/extbg_unittests.ceps

#########################################
#    ctrl
#########################################

# # ## ctrl
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/flap.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $PROGRAMS/ctrl.ceps \
# $PROGRAMS/antenna_lashing_unlock.ceps \
# $PROGRAMS/antenna_to_raise.ceps \
# $PROGRAMS/antenna_to_hinge_down.ceps \
# $PROGRAMS/dispatch.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/mast_move_up.ceps \
# $PROGRAMS/mast_move_down.ceps \
# $PROGRAMS/stilts_to_extend.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_to_retract.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $TESTS/ctrl/ctrl_unittests.ceps

# ## dispatch unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/flap.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $STATE/stilt_left.ceps \
# $STATE/stilts_vk_left.ceps \
# $PROGRAMS/dispatch.ceps \
# $PROGRAMS/antenna_to_raise.ceps \
# $PROGRAMS/antenna_to_hinge_down.ceps \
# $PROGRAMS/antenna_to_level_out.ceps \
# $PROGRAMS/antenna_to_parking_position.ceps \
# $PROGRAMS/antenna_lashing_unlock.ceps \
# $PROGRAMS/antenna_lashing_lock.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $PROGRAMS/dust_discharge_to_do.ceps \
# $PROGRAMS/mast_move_up.ceps \
# $PROGRAMS/mast_move_down.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/platform_extend.ceps \
# $PROGRAMS/platform_retract_preconditioning.ceps \
# $PROGRAMS/platform_retract.ceps \
# $PROGRAMS/platform_extend_fto_and_alu.ceps \
# $PROGRAMS/platform_retract_ftc_and_all.ceps \
# $PROGRAMS/stilts_to_extend.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_to_retract.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $PROGRAMS/system_extend.ceps \
# $PROGRAMS/system_retract.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_to_arbeitsposition.ceps \
# $PROGRAMS/x_drive_to_indexposition.ceps \
# $PROGRAMS/y_drive_to_indexposition.ceps \
# $PROGRAMS/z_drive_to_index_position.ceps \
# $PROGRAMS/z_drive_to_lashing_position.ceps \
# $PROGRAMS/x_drive_to_target_position.ceps \
# $PROGRAMS/y_drive_to_target_position.ceps \
# $PROGRAMS/z_drive_to_target_position.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $DINGO/stilts_valve_SVZA.ceps \
# $DINGO/stilts_valve_SVZE.ceps \
# $DINGO/stilts_valve_SVK.ceps \
# $DINGO/stilts_endschalter_SEKGL.ceps \
# $DINGO/stilts_endschalter_SEKOL.ceps \
# $DINGO/stilts_endschalter_SMUL.ceps \
# $DINGO/stilts_endschalter_SMOL.ceps \
# $TESTS/dispatch/dispatch_unittests.ceps

#########################################
#    system and platform
#########################################

# # Platform extend
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/antenna_lashing.ceps \
# $STATE/drives_state.ceps \
# $STATE/funktionsueberwachung.ceps \
# $STATE/flap.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $STATE/stilt_left.ceps \
# $STATE/stilt_right.ceps \
# $STATE/stilts_vk_left.ceps \
# $STATE/stilts_vk_right.ceps \
# $PROGRAMS/system_extend.ceps \
# $PROGRAMS/platform_extend.ceps \
# $PROGRAMS/platform_extend_fto_and_alu.ceps \
# $PROGRAMS/platform_retract_ftc_and_all.ceps \
# $PROGRAMS/antenna_to_raise.ceps \
# $PROGRAMS/antenna_lashing_lock.ceps \
# $PROGRAMS/antenna_lashing_unlock.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_to_arbeitsposition.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/z_drive_to_index_position.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $PROGRAMS/stilts_to_extend.ceps \
# $PROGRAMS/stilts_to_retract.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $DINGO/antenna_valve_VZ1O.ceps \
# $DINGO/antenna_valve_VZ1V.ceps \
# $DINGO/antenna_endschalter_lock_VZ1O.ceps \
# $DINGO/antenna_endschalter_lock_VZ1V.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $DINGO/stilts_valve_SVZA.ceps \
# $DINGO/stilts_valve_SVZE.ceps \
# $DINGO/stilts_valve_SVK.ceps \
# $DINGO/stilts_endschalter_SEKGR.ceps \
# $DINGO/stilts_endschalter_SEKGL.ceps \
# $DINGO/stilts_endschalter_SEKOL.ceps \
# $DINGO/stilts_endschalter_SEKOR.ceps \
# $DINGO/stilts_endschalter_SMUL.ceps \
# $DINGO/stilts_endschalter_SMUR.ceps \
# $DINGO/stilts_endschalter_SMOL.ceps \
# $DINGO/stilts_endschalter_SMOR.ceps \
# $DINGO/talin.ceps \
# $DINGO/y_drive.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/system_and_platform/system_extend_unittests.ceps
# #$TESTS/system_and_platform/platform_extend_unittests.ceps

# ## System extend
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/system_extend.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_lock.ceps \
# $TESTS/system_and_platform/system_extend_unittests.ceps

#########################################
#    System Integration
#########################################
# ## System integration
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/bag_distributed_state.ceps \
# $STATE/z_drive_state.ceps \
# $PROGRAMS/ctrl.ceps \
# $PROGRAMS/dispatch.ceps \
# $PROGRAMS/mast_move_up.ceps \
# $PROGRAMS/mast_move_down.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/z_drive_to_target_position.ceps \
# $BASE_DEFS/bag_activator.ceps

#########################################
#    drives
#########################################

#########################################
#    drives-unittests
#########################################


#########################################
#    drives-integration
#########################################

# # X drive to index position unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/x_drive_to_indexposition.ceps \
# $DINGO/x_drive.ceps \
# $TESTS/drives/x_drive_to_indexposition_unittests.ceps

# ## X drive to target position unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/x_drive_to_target_position.ceps \
# $DINGO/x_drive.ceps \
# $TESTS/drives/x_drive_to_target_position_unittests.ceps

# # Y drive to index position unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_to_indexposition.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $DINGO/y_drive.ceps \
# $TESTS/drives/y_drive_to_indexposition_unittests.ceps

# ## Y drive to target position unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_to_target_position.ceps \
# $DINGO/y_drive.ceps \
# $TESTS/drives/y_drive_to_target_position_unittests.ceps

# # Y drive to arbeitsposition unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_to_arbeitsposition.ceps \
# $DINGO/y_drive.ceps \
# $TESTS/drives/y_drive_to_arbeitsposition_unittests.ceps

#########################################
#           drives-programs
#########################################

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/y_drive_enable_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/x_drive_enable_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/z_drive_enable_unittests.ceps

## x_drive reset fault antrieb unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/x_drive_reset_fault_antrieb_unittests.ceps

# ## y_drive reset fault antrieb unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/y_drive_reset_fault_antrieb_unittests.ceps

# ## z_drive reset fault antrieb unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $TESTS/drives/z_drive_reset_fault_antrieb_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $TESTS/drives/z_drive_disable_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $TESTS/drives/y_drive_disable_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $TESTS/drives/x_drive_disable_unittests.ceps

#########################################
#    drives-state
#########################################

# ##  arki: todo
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $TESTS/drives/z_drive_state_unittests.ceps

#########################################
#    drives-dingo
#########################################

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/x_drive_endschalter_IP_X.ceps \
# $TESTS/drives/x_drive_endschalter_IP_X_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/y_drive_endschalter_IP_Y.ceps \
# $TESTS/drives/y_drive_endschalter_IP_Y_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/y_drive_endschalter_AB_Y.ceps \
# $TESTS/drives/y_drive_endschalter_AB_Y_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/drives/z_drive_unittests.ceps

#########################################
#    antenna
#########################################

#########################################
#    antenna-dingo
#########################################

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/antenna_valve_VZ1V.ceps \
# $TESTS/antenna/antenna_valve_VZ1V_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/antenna_valve_VZ1O.ceps \
# $TESTS/antenna/antenna_valve_VZ1O_unittests.ceps

# ## antenna endschalter lock VZ1O unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/antenna_endschalter_lock_VZ1O.ceps \
# $TESTS/antenna/antenna_endschalter_lock_VZ1O_unittests.ceps

# ## antenna endschalter lock VZ1V unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/antenna_endschalter_lock_VZ1V.ceps \
# $TESTS/antenna/antenna_endschalter_lock_VZ1V_unittests.ceps

#########################################
#    antenna-state
#########################################

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/antenna_lashing.ceps \
# $TESTS/antenna/antenna_lashing_unittests.ceps

#########################################
#    antenna-programs
#########################################

# ## antenna to raise
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/x_drive_to_indexposition.ceps \
# $PROGRAMS/y_drive_to_arbeitsposition.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/antenna_to_raise.ceps \
# $TESTS/antenna/antenna_to_raise_unittests.ceps

# ## Antenna to hinge down
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/x_drive_to_indexposition.ceps \
# $PROGRAMS/y_drive_to_indexposition.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/antenna_to_hinge_down.ceps \
# $PROGRAMS/antenna_to_parking_position.ceps \
# $TESTS/antenna/antenna_to_hinge_down_unittests.ceps

# ## Antenna to parking position
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/antenna_to_parking_position.ceps \
# $TESTS/antenna/antenna_to_parking_position_unittests.ceps

# # antenna to level out
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/x_drive_move.ceps \
# $PROGRAMS/y_drive_move.ceps \
# $PROGRAMS/x_drive_enable.ceps \
# $PROGRAMS/y_drive_enable.ceps \
# $PROGRAMS/x_drive_disable.ceps \
# $PROGRAMS/y_drive_disable.ceps \
# $PROGRAMS/x_drive_to_target_position.ceps \
# $PROGRAMS/y_drive_to_target_position.ceps \
# $PROGRAMS/x_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/y_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/antenna_to_level_out.ceps \
# $TESTS/antenna/antenna_to_level_out_unittests.ceps


# ## antenna lashing unlock
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/antenna_lashing_unlock.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $TESTS/antenna/antenna_lashing_unlock_unittests.ceps

# ## antenna lashing lock
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/antenna_lashing_lock.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $TESTS/antenna/antenna_lashing_lock_unittests.ceps

# ## antenna lock preconditioning
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $TESTS/antenna/antenna_lock_preconditioning_unittests.ceps

#########################################
#    antenna integration
#########################################

# ## antenna lashing feature
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/antenna_lashing.ceps \
# $PROGRAMS/antenna_lashing_lock.ceps \
# $PROGRAMS/antenna_lashing_unlock.ceps \
# $PROGRAMS/antenna_lock_preconditioning.ceps \
# $DINGO/antenna_valve_VZ1V.ceps \
# $DINGO/antenna_valve_VZ1O.ceps \
# $DINGO/antenna_endschalter_lock_VZ1V.ceps \
# $DINGO/antenna_endschalter_lock_VZ1O.ceps \
# $TESTS/antenna/antenna_lashing_feature_unittests.ceps

#########################################
#    mast
#########################################

#########################################
#    mast-feature
#########################################

# ## mast move up
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/mast_move_up.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/mast/mast_move_up_unittests.ceps

# ## mast move down
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/mast_move_down.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/mast/mast_move_down_unittests.ceps

# ## mast movement feature unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/mast/mast_movement_feature_unittests.ceps

#########################################
#    mast-programs
#########################################

## mast move to indexposition unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/z_drive_to_index_position.ceps \
# $DINGO/z_drive.ceps \
# $DINGO/talin.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/mast/mast_move_to_indexposition_unittests.ceps

# ## mast move to target
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/mast_move_down.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_to_target_position.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $DINGO/talin.ceps \
# $DINGO/z_drive.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/mast/mast_move_to_target_unittests.ceps

# ## mast move to indexposition unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/z_drive_to_index_position.ceps \
# $DINGO/z_drive.ceps \
# $TESTS/mast/mast_move_to_indexposition_unittests.ceps

# ## mast move to lashing position unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/drives_state.ceps \
# $PROGRAMS/z_drive_move.ceps \
# $PROGRAMS/z_drive_enable.ceps \
# $PROGRAMS/z_drive_disable.ceps \
# $PROGRAMS/z_drive_reset_fault_antrieb.ceps \
# $PROGRAMS/z_drive_to_lashing_position.ceps \
# $DINGO/z_drive.ceps \
# $DINGO/talin.ceps \
# $DINGO/funktionsueberwachung.ceps \
# $TESTS/mast/mast_move_to_lashing_position_unittests.ceps

#########################################
#    mast-dingo
#########################################

# mast endschalter UB Z unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/mast_endschalter_UB_Z.ceps \
# $TESTS/mast/mast_endschalter_UB_Z_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/mast_endschalter_EU_Z.ceps \
# $TESTS/mast/mast_endschalter_EU_Z_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/mast_endschalter_EO_Z.ceps \
# $TESTS/mast/mast_endschalter_EO_Z_unittests.ceps

# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/mast_endschalter_IP_Z.ceps \
# $TESTS/mast/mast_endschalter_IP_Z_unittests.ceps

#########################################
#    dust discharge-feature
#########################################

# # Dust discharge feature unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/dust_discharge.ceps \
# $PROGRAMS/dust_discharge_to_do.ceps \
# $DINGO/dust_valve_VV1.ceps \
# $TESTS/dust_discharge/dust_discharge_feature_unittests.ceps

#########################################
#    dust discharge-programs
#########################################

# # Dust discharge to do unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/dust_discharge_to_do.ceps \
# $TESTS/dust_discharge/dust_discharge_to_do_unittests.ceps

# # Dust discharge unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/dust_discharge.ceps \
# $TESTS/dust_discharge/dust_discharge_unittests.ceps

# # Dust discharge valve VV1 unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/dust_valve_VV1.ceps \
# $TESTS/dust_discharge/dust_valve_VV1_unittests.ceps

#########################################
#    stilts
#########################################

#########################################
#    stilts-integration
#########################################

# ## Stilts feature unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/stilt_left.ceps \
# $STATE/stilt_right.ceps \
# $STATE/stilts_vk_left.ceps \
# $STATE/stilts_vk_right.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $PROGRAMS/stilts_to_extend.ceps \
# $PROGRAMS/stilts_to_retract.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $DINGO/pneumatics.ceps \
# $DINGO/stilts_valve_SVZA.ceps \
# $DINGO/stilts_valve_SVZE.ceps \
# $DINGO/stilts_valve_SVK.ceps \
# $DINGO/stilts_position.ceps \
# $DINGO/stilts_endschalter_SEKGL.ceps \
# $DINGO/stilts_endschalter_SEKGR.ceps \
# $DINGO/stilts_endschalter_SEKOL.ceps \
# $DINGO/stilts_endschalter_SEKOR.ceps \
# $DINGO/stilts_endschalter_SMUL.ceps \
# $DINGO/stilts_endschalter_SMUR.ceps \
# $DINGO/stilts_endschalter_SMOL.ceps \
# $DINGO/stilts_endschalter_SMOR.ceps \
# $TESTS/stilts/stilts_feature_unittests.ceps

#########################################
#    stilts-state
#########################################

# # Stilts stilt left unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/stilt_left.ceps \
# $TESTS/stilts/stilts_stilt_left_unittests.ceps

#########################################
#    stilts-programs
#########################################

# # Stilts to retract_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $PROGRAMS/stilts_to_retract.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $TESTS/stilts/stilts_to_retract_unittests.ceps

# Stilts to extend_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_to_extend.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $TESTS/stilts/stilts_to_extend_unittests.ceps

# # Stilts lock unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_lock.ceps \
# $TESTS/stilts/stilts_lock_unittests.ceps

# # Stilts unlock unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_unlock.ceps \
# $TESTS/stilts/stilts_unlock_unittests.ceps

# # Stilts preconditioning unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_preconditioning.ceps \
# $TESTS/stilts/stilts_preconditioning_unittests.ceps

# # Stilts retract_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_retract.ceps \
# $TESTS/stilts/stilts_retract_unittests.ceps

# # Stilts extend_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/stilts_extend.ceps \
# $TESTS/stilts/stilts_extend_unittests.ceps

#########################################
#    stilts-dingo
#########################################

# # Stilt_position_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_position.ceps \
# $TESTS/stilts/stilts_position_unittests.ceps

# # Stilt_valve_SVZA_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_valve_SVZA.ceps \
# $TESTS/stilts/stilts_valve_SVZA_unittests.ceps

# # Stilt_valve_SVZE_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_valve_SVZE.ceps \
# $TESTS/stilts/stilts_valve_SVZE_unittests.ceps

# # Stilts endschalter SEKOL unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SEKOL.ceps \
# $TESTS/stilts/stilts_endschalter_SEKOL_unittests.ceps

# # Stilts endschalter SEKOR unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SEKOR.ceps \
# $TESTS/stilts/stilts_endschalter_SEKOR_unittests.ceps

# # Stilts endschalter SEKGL unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SEKGL.ceps \
# $TESTS/stilts/stilts_endschalter_SEKGL_unittests.ceps

# # Stilts endschalter SEKGR unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SEKGR.ceps \
# $TESTS/stilts/stilts_endschalter_SEKGR_unittests.ceps

# # Stilts_endschalter_SMOL_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SMOL.ceps \
# $TESTS/stilts/stilts_endschalter_SMOL_unittests.ceps

# # Stilts_endschalter_SMOR_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SMOR.ceps \
# $TESTS/stilts/stilts_endschalter_SMOR_unittests.ceps

# # Stilts_endschalter_SMUL_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SMUL.ceps \
# $TESTS/stilts/stilts_endschalter_SMUL_unittests.ceps

# # Stilts_endschalter_SMUR_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_endschalter_SMUR.ceps \
# $TESTS/stilts/stilts_endschalter_SMUR_unittests.ceps

# # Stilts_valve_SVK_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/stilts_valve_SVK.ceps \
# $TESTS/stilts/stilts_valve_SVK_unittests.ceps

#########################################
#    flap feature test
#########################################

# # test for simulation
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/flap.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $PROGRAMS/dispatch.ceps


# ## flap feature unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/flap.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $DINGO/flap_position.ceps \
# $DINGO/flap_lock_HR1.ceps \
# $DINGO/flap_lock_HR2.ceps \
# $DINGO/flap_valve_HR.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $TESTS/flap/flap_feature_unittests.ceps

# ## flap program and valves unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $DINGO/flap_valve_HR.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $TESTS/flap/flap_program_and_valves_unittests.ceps

# ## flap_valves_and_es_unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $DINGO/flap_position.ceps \
# $DINGO/flap_lock_HR1.ceps \
# $DINGO/flap_lock_HR2.ceps \
# $DINGO/flap_valve_HR.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $TESTS/flap/flap_valves_and_es_unittests.ceps

# # flap locks unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $STATE/flap_lock_one.ceps \
# $STATE/flap_lock_two.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $DINGO/flap_lock_HR1.ceps \
# $DINGO/flap_lock_HR2.ceps \
# $DINGO/flap_valve_HR.ceps \
# $TESTS/flap/flap_locks_unittests.ceps

#########################################
#    flap-programs
#########################################

# ## flap_precondition
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $TESTS/flap/flap_precondition_unittests.ceps

# ## flap lock unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_lock.ceps \
# $TESTS/flap/flap_lock_unittests.ceps

# ## flap unlock unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $TESTS/flap/flap_unlock_unittests.ceps

# # flap open unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_open.ceps \
# $TESTS/flap/flap_open_unittests.ceps

# ## flap_close
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_close.ceps \
# $TESTS/flap/flap_close_unittests.ceps

# # flap to open unittests
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_open.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_to_open.ceps \
# $TESTS/flap/flap_to_open_unittests.ceps

# ## flap_to_close
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $PROGRAMS/flap_precondition.ceps \
# $PROGRAMS/flap_lock.ceps \
# $PROGRAMS/flap_unlock.ceps \
# $PROGRAMS/flap_close.ceps \
# $PROGRAMS/flap_to_close.ceps \
# $TESTS/flap/flap_to_close_unittests.ceps

#########################################
#    flap-sm-state
#########################################

# ## flap
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/flap.ceps \
# $TESTS/flap/flap_unittests.ceps

# ## flap_lock_one
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/flap_lock_one.ceps \
# $TESTS/flap/flap_lock_one_unittests.ceps

# ## flap_lock_two
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $STATE/flap_lock_two.ceps \
# $TESTS/flap/flap_lock_two_unittests.ceps


#########################################
#    flap-sm-dingo
#########################################

# ## flap_position
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_position.ceps \
# $TESTS/flap/flap_position_unittests.ceps

# ## Dingo_flap_lock_HR1
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_lock_HR1.ceps \
# $TESTS/flap/flap_lock_HR1_unittests.ceps

# ## Dingo_flap_lock_HR2
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_lock_HR2.ceps \
# $TESTS/flap/flap_lock_HR2_unittests.ceps

# ## flap_endschalter_HZG
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HZG.ceps \
# $TESTS/flap/flap_endschalter_HZG_unittests.ceps

# ## flap_endschalter_HZO
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HZO.ceps \
# $TESTS/flap/flap_endschalter_HZO_unittests.ceps

# ## Dingo_flap_endschalter_HR1O
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HR1O.ceps \
# $TESTS/flap/flap_endschalter_HR1O_unittests.ceps

# ## Dingo_flap_endschalter_HR2O
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HR2O.ceps \
# $TESTS/flap/flap_endschalter_HR2O_unittests.ceps

# ## Dingo_flap_endschalter_HR1V
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HR1V.ceps \
# $TESTS/flap/flap_endschalter_HR1V_unittests.ceps

# ## Dingo_flap_endschalter_HR2V
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_endschalter_HR2V.ceps \
# $TESTS/flap/flap_endschalter_HR2V_unittests.ceps

# ## Dingo_flap_valve_HR
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_valve_HR.ceps \
# $TESTS/flap/flap_valve_HR_unittests.ceps

# ## Flap_valve_HVZO
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_valve_HVZO.ceps \
# $TESTS/flap/flap_valve_HVZO_unittests.ceps

# ## Flap_valve_HVZON
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_valve_HVZON.ceps \
# $TESTS/flap/flap_valve_HVZON_unittests.ceps

# ## Flap_valve_HVZS
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_valve_HVZS.ceps \
# $TESTS/flap/flap_valve_HVZS_unittests.ceps

# ## Flap_valve_HVZSN
# $EXE/sm2plantuml \
# $BASE_DEFS/base_defs.ceps \
# $BASE_DEFS/dingo_definitions.ceps \
# $DINGO/flap_valve_HVZSN.ceps \
# $TESTS/flap/flap_valve_HVZSN_unittests.ceps
