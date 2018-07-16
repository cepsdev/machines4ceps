#!/bin/bash
lib/extract_rollouts -h "$ROLLAUT_DB_HOST" -u "$ROLLAUT_DB_USER" -p "$ROLLAUT_DB_PASSWD" > db_rollout_dump.ceps
