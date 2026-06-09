#!/usr/bin/env bash
# run_pomiar.sh — wrapper przekierowujący do run_pomiar_2a.sh
# Zachowany dla kompatybilności z poprzednim workflow.
exec "$(dirname "$0")/run_pomiar_2a.sh" "$@"
