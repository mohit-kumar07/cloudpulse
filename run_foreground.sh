
set -e
cd "$(dirname "$0")"
set -a
source cloudpulse_dashboard/.env
set +a
trap 'kill 0' EXIT

./cpp/build/cloud_monitor_agent 2>&1 | sed -u 's/^/[agent]      /' &
venv/bin/python python/integrator.py --config config/config.json 2>&1 | sed -u 's/^/[integrator] /' &
venv/bin/python cloudpulse_dashboard/app.py 2>&1 | sed -u 's/^/[dashboard]  /' &

wait
