mkdir exec/$1 -p
cd exec/$1
../../../../x86/ceps ../../lex/rollout.ceps.lex ../../scenarios/$1.rll ../../transformations/rollout2sm.ceps --ignore_simulations --dot_gen --dot_gen_one_file_per_top_level_statemachine
for e in *.dot ; do
 dot $e -Tsvg -o ../../web/${e%%.dot}.svg
done
../../../../x86/ceps ../../lex/rollout.ceps.lex ../../scenarios/$1.rll ../../transformations/rollout2sm.ceps ../../drivers/rollout2simulation_basic_1.ceps $2 --dot_gen --dot_gen_one_file_per_top_level_statemachine --ws_api 1063


cd ../..