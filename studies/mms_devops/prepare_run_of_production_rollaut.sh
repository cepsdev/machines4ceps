#!/bin/bash
cp $1 db_rollout_dump.ceps
rm fetch_rollout_plan
ln lib/fetch_rollout_plan_null.sh fetch_rollout_plan --symbolic


