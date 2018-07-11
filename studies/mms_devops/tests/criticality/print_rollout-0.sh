#!/bin/bash

../../ceps rollout-0/globals.ceps rollout-0/db_rollout_dump.ceps ../../db_descr/gen.ceps rollout-0/extract_rollout.ceps  rollout-0/criticality_definitions.ceps ../../transformations/rollout2watchdogs.ceps --print_evaluated_input_tree
