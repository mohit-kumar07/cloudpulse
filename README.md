# CloudPulse — Cloud Resource Tracker with ServiceNow
Tech stack: C++ (Arch Linux agent) + MySQL + Python (ServiceNow integration) + ServiceNow

## What is included
- `cpp/monitor.cpp` : C++ resource monitoring agent that writes metrics to MySQL.
- `cpp/CMakeLists.txt` : CMake build file.
- `python/integrator.py` : Python script that reads metrics, checks thresholds, and creates incidents in ServiceNow.
- `python/requirements.txt` : Python dependencies.
- `sql/schema.sql` : MySQL schema for the metrics table.
- `systemd/cloudmonitor.service` : example systemd unit to run the C++ agent on Arch.
- `config/sample_config.json` : configuration for Python integrator and DB.
- `run.sh` : helper script to build C++ and run components (example).
- `.gitignore` : git ignore rules.

## Quick Setup (Arch Linux)
1. Install dependencies:
   - mysql/mariadb server and client, develop libraries
     `sudo pacman -Syu mariadb mariadb-libs cmake gcc make openssl`
   - MySQL Connector/C++ (package name may vary). Use AUR if necessary.
   - For Python: `python -m pip install -r python/requirements.txt`
2. Create DB:
   - Start MariaDB and secure it.
   - Run `mysql -u root -p < sql/schema.sql`
3. Build C++:
   ```
   mkdir build && cd build
   cmake ../cpp
   make
   sudo cp cloud_monitor_agent /usr/local/bin/
   ```
4. Configure `config/sample_config.json` with your DB and ServiceNow credentials and copy to `config/config.json`.
5. Run agent:
   - Manual: `./cloud_monitor_agent`
   - As service: `sudo cp systemd/cloudmonitor.service /etc/systemd/system/` and `sudo systemctl enable --now cloudmonitor`
6. Run Python integrator:
   `python python/integrator.py --config config/config.json`

## Notes
- This is a starter project for a final-year submission. Update ServiceNow instance credentials and ensure network access.
- See inline comments in source files for implementation notes.
