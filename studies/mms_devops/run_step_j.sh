

../../x86/ceps jenkins_mock.ceps step_j.ceps   --dot_gen --dot_gen_one_file_per_top_level_statemachine --ignore_simulations
for e in *.dot ; do
 dot $e -Tsvg -o web/${e%%.dot}.svg
done


../../x86/ceps jenkins_mock.ceps step_j.ceps --dot_gen --dot_gen_one_file_per_top_level_statemachine --ws_api 1063 
