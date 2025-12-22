set -e
cd "$(dirname "$0")"
sudo cp systemd/cloudmonitor.service systemd/cloudintegrator.service /etc/systemd/system/
sudo cp cloudpulse_dashboard/systemd/cloudpulse_dashboard.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now cloudmonitor.service cloudintegrator.service cloudpulse_dashboard.service
systemctl --no-pager status cloudmonitor.service cloudintegrator.service cloudpulse_dashboard.service
