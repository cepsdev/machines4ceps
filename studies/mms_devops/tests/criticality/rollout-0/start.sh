
     cd runs
     cp ../db_rollout_dump.ceps ./rollout-0
     cd rollout-0
     ../../ceps db_rollout_dump.ceps \
             globals.ceps\
             ../../db_descr/gen.ceps \
             extract_rollout.ceps \
             ../../lib/conf.ceps \
             ../../lib/rollout_step.ceps \
             ../../transformations/rollout2worker.ceps \
             ../../transformations/rollout2sm.ceps \
             ../../transformations/driver4rollout_start_immediately.ceps \
             --dot_gen_one_file_per_top_level_statemachine \
             --dot_gen \
             --ignore_simulations
     mkdir ../../web/rollout-0__svgs -p
     for e in *.dot ; do
        dot $e -Tsvg -o ../../web/rollout-0__svgs/${e%%.dot}.svg
     done

     for e in *.dot ; do
        rm $e
     done
               
     
    