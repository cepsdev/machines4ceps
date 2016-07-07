/* out_frames.cpp 
   CREATED Fri Jun 24 18:10:00 2016

   GENERATED BY the sm4ceps C++ Generator VERSION 0.50 (c) Tomas Prerovsky <tomas.prerovsky@gmail.com>, ALL RIGHTS RESERVED. 
   Requires C++1y compatible compiler (use --std=c++1y for g++) 
   BASED ON cepS VERSION 1.1 (Jun  1 2016) BUILT WITH GCC 5.2.1 20151010 on GNU/LINUX 64BIT (C) BY THE AUTHORS OF ceps (ceps is hosted at github: https://github.com/cepsdev/ceps.git) 

   Input files (relative paths):
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/base_defs.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_disable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_enable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_brake_release.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_brake_apply.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_move.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_reset_fault_antrieb.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_to_indexposition.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_to_target_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_move_left_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/x_drive_move_right_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_disable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_enable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_brake_release.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_brake_apply.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_move.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_reset_fault_antrieb.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_to_arbeitsposition.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_to_indexposition.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_to_target_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_move_front_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/y_drive_move_rear_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_disable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_enable.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_brake_release.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_brake_apply.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_move.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_reset_fault_antrieb.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_to_index_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_to_lashing_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_to_target_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_move_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/z_drive_move_up.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/ctrl.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/dispatch.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_lashing_lock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_lashing_unlock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_lock_preconditioning.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_to_hinge_down.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_to_level_out.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_to_parking_position.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/antenna_to_raise.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/dust_discharge_to_do.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_close.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_lock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_open.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_precondition.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_to_close.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_to_open.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/flap_unlock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/platform_extend.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/platform_extend_fto_and_alu.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/platform_retract.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/platform_retract_ftc_and_all.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/platform_retract_preconditioning.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_extend.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_lock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_preconditioning.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_retract.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_to_extend.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_to_retract.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/stilts_unlock.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/system_extend.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-programs/system_retract.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/antenna_alignment.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/antenna_lashing.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/drives_state.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/dust_discharge.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/extbg_progress_indicator.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/extbg_error_evaluator.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/flap.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/flap_lock_one.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/flap_lock_two.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/funktionsueberwachung.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/io_module_controler.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/ivenet_state.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/pneumatic_cylinder_pressure.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/stilt_left.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/stilt_right.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/stilts_vk_left.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/stilts_vk_right.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/user_buttons.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/warning_mast_operations.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/x_drive_fault_state.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/y_drive_fault_state.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-state/z_drive_fault_state.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/control/sm-dingo/talin.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_fue.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_drives.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_io_modul.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_si_modul.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_extbg.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_ivenet.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/messages/messages_sae.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/bag/rx-bag.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/bag/rx-bag-sae.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/bag/tx-bag-ivenet.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/bag/tx-bag.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/bag/channels-bag-vcan.ceps
      /home/tprerovs/projects/sm4ceps/test/cppgen/statemachines/network/gencpp.ceps

   THIS IS A GENERATED FILE.

   *** DO NOT MODIFY. ***
*/

