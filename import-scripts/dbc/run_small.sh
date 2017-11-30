echo
echo "*** Don't forget to setup a vcan0 CAN interface! ***"

../../x86/ceps ../common.ceps dbc.ceps.lex frames.dbc config.ceps param_import_dbc.ceps driver.ceps --quiet --no_warn
