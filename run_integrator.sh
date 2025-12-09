#!/bin/bash
# reliable wrapper: use absolute paths and unbuffered Python output
cd /home/$(whoami)/cloud_resource_tracker_project || exit 1
# activate venv if present
if [ -f "./venv/bin/activate" ]; then
    # shellcheck disable=SC1091
    source ./venv/bin/activate
fi
export PYTHONUNBUFFERED=1
# redirect stdout/stderr to absolute paths
exec >> /home/$(whoami)/cloud_resource_tracker_project/integrator.log 2>> /home/$(whoami)/cloud_resource_tracker_project/integrator.err.log
# run integrator with absolute config path
exec python python/integrator.py --config /home/$(whoami)/cloud_resource_tracker_project/config/config.json
