set -e
sudo systemctl stop cloudmonitor.service cloudintegrator.service cloudpulse_dashboard.service
systemctl --no-pager status cloudmonitor.service cloudintegrator.service cloudpulse_dashboard.service || true
