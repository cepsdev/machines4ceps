#!/bin/bash

#../../../x86/ceps ../lex/rollout.ceps.lex $1/$1.rll ../transformations/rollout2sm.ceps $2 --dot_gen --dot_gen_one_file_per_top_level_statemachine --ws_api 1063

for e in *.dot ; do
 dot $e -Tsvg -o ../web/${e%%.dot}.svg
done

../../../x86/ceps ../lib/conf.ceps \
                  ../lib/rollout_step.ceps \
                  ../lex/rollout.ceps.lex \
                  $1/$1.rll \
                  ../transformations/rollout2worker.ceps \
                  ../transformations/rollout2sm.ceps \
                  ../transformations/driver4rollout_start_immediately.ceps \
                  $2 \
                  --dot_gen_one_file_per_top_level_statemachine \
                  --dot_gen \
                  --ws_api 1063