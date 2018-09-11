#!/bin/bash
source set_env_prod_db.sh
lib/fetch_rollout_plan.sh
cp db_rollout_dump.ceps runs_store/$1
