#!/bin/bash

../../ceps rollout-0/globals.ceps rollout-0/db_rollout_dump.ceps ../../db_descr/gen.ceps rollout-0/extract_rollout.ceps ../../lib/conf.ceps ../../lib/rollout_step.ceps ../../transformations/rollout2worker.ceps ../../transformations/rollout2sm.ceps rollout-0/criticality_definitions.ceps ../../transformations/rollout2watchdogs.ceps ../../transformations/driver4rollout_start_immediately.ceps 
