# 🌩️ CloudPulse  
### Intelligent Cloud Infrastructure Monitoring & Automated Incident Management  
**C++ • Python • MariaDB • Arch Linux • ServiceNow**

![Made with C++](https://img.shields.io/badge/C++-17-blue)
![Made with Python](https://img.shields.io/badge/Python-3.13-yellow)
![Database](https://img.shields.io/badge/MariaDB-10.11+-brightgreen)
![Platform](https://img.shields.io/badge/Arch-Linux-blue)
![Platform](https://img.shields.io/badge/ServiceNow-ITSM-darkgreen)

---

## 📌 Overview

**CloudPulse** is a real-time cloud infrastructure monitoring system built using:

- **C++ agent** for system metrics (CPU, RAM, Disk, Network)
- **MariaDB backend** for data storage
- **Python integrator** for automated incident creation
- **ServiceNow ITSM** for ticketing workflows
- **systemd services** for continuous background execution

---

## 📐 Architecture

```
           +-------------------------+
           |     C++ Monitor Agent   |
           |  (CPU/RAM/DISK/NET)     |
           +-----------+-------------+
                       |
                       | Inserts metrics (5s)
                       v
              +------------------+
              |    MariaDB       |
              |   metrics table  |
              +--------+---------+
                       |
                       | Reads latest metrics (30s)
                       v
           +----------------------------+
           |   Python Integrator        |
           | Threshold + Cooldown Logic |
           +--------+-------------------+
                    |
                    | REST API call
                    v
         +------------------------------+
         |       ServiceNow ITSM        |
         |  Incident Auto-Creation      |
         +------------------------------+
```

---

## 📦 Tech Stack

| Component | Technology |
|----------|------------|
| Agent | C++17 |
| Backend | MariaDB |
| Integrator | Python 3.13 |
| OS | Arch Linux |
| Automation | systemd |
| Ticketing | ServiceNow |

---

## 🛠️ Installation (Arch Linux)

### 1. Install Dependencies
```bash
sudo pacman -S mariadb mariadb-clients base-devel cmake python python-pip
```

### 2. Start MariaDB
```bash
sudo systemctl enable --now mariadb
```

### 3. Create Database
```sql
CREATE DATABASE cloud_monitor;
CREATE USER 'monitor'@'localhost' IDENTIFIED BY 'yourpassword';
GRANT ALL PRIVILEGES ON cloud_monitor.* TO 'monitor'@'localhost';
```

Import schema:
```bash
mysql -u monitor -p cloud_monitor < sql/schema.sql
```

---

## ⚙️ Build C++ Agent

```bash
cd cpp
mkdir build && cd build
cmake ..
make -j$(nproc)
sudo cp cloud_monitor_agent /usr/local/bin/
```

Run:
```bash
/usr/local/bin/cloud_monitor_agent
```

---

## 📡 Python Integrator Setup

```bash
cd python
python -m venv ../venv
source ../venv/bin/activate
pip install -r requirements.txt
```

Run:
```bash
python integrator.py --config ../config/config.json
```

---

## 🔧 systemd Services

### Agent
```bash
sudo cp systemd/cloud_monitor_agent.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now cloud_monitor_agent.service
```

### Integrator
```bash
sudo cp systemd/cloud_integrator.service /etc/systemd/system/
sudo systemctl daemon-reload
sudo systemctl enable --now cloud_integrator.service
```

---

## 🧪 Testing

### View latest metrics
```bash
mysql -u monitor -p -e "SELECT * FROM cloud_monitor.metrics ORDER BY timestamp DESC LIMIT 10;"
```

### Trigger stress:
```bash
stress-ng --cpu 4 --timeout 20
stress-ng --vm 1 --vm-bytes 1G --timeout 20
```

### Verify ServiceNow incidents
Visit:
```
https://yourinstance.service-now.com/nav_to.do?uri=incident_list.do
```

---

## 📁 Project Structure

```
cloudpulse/
│── cpp/
│── python/
│── config/
│── sql/
│── systemd/
│── README.md
```
