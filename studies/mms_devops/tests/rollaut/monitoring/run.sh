./ceps -e "val number_of_markets = $1;val number_of_steps = $2;val SIMULATION_BASE_PERIOD=$3;" rollout.ceps rollout2sm.ceps $5.ceps driver.ceps --ws_api $4 --quiet
