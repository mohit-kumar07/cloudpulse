# integrator.py
# Reads latest metrics from MySQL and creates incidents in ServiceNow for threshold breaches.

import argparse
import json
import time
import mysql.connector
import requests
from datetime import datetime

def load_config(path):
    with open(path) as f:
        return json.load(f)

def get_latest_metrics(dbcfg, limit=1):
    conn = mysql.connector.connect(
        host=dbcfg['host'], user=dbcfg['user'], password=dbcfg['password'], database=dbcfg['database']
    )
    cursor = conn.cursor(dictionary=True)
    cursor.execute("SELECT * FROM metrics ORDER BY timestamp DESC LIMIT %s", (limit,))
    rows = cursor.fetchall()
    cursor.close()
    conn.close()
    return rows

def create_incident(sn_cfg, short, desc, urgency='2', impact='2'):
    url = sn_cfg['instance'].rstrip('/') + "/api/now/table/incident"
    auth = (sn_cfg['user'], sn_cfg['password'])
    payload = {
        "short_description": short,
        "description": desc,
        "urgency": urgency,
        "impact": impact
    }
    headers = {'Content-Type': 'application/json', 'Accept': 'application/json'}
    try:
        r = requests.post(url, auth=auth, json=payload, headers=headers, timeout=15)
    except Exception as e:
        print(f"[-] Error calling ServiceNow API: {e}")
        return False

    if r.status_code in (200,201):
        number = r.json().get('result', {}).get('number')
        print(f"[+] Incident created: {number}")
        return True
    else:
        print('[-] Failed to create incident:', r.status_code, r.text)
        return False

def now_ts():
    return datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S UTC")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', required=True, help="Path to config.json")
    args = parser.parse_args()

    cfg = load_config(args.config)
    dbcfg = cfg['mysql']
    sncfg = cfg['servicenow']
    poll_interval = cfg.get('poll_interval', 30)

    default_thresholds = {
        'cpu': 80.0,
        'memory': 80.0,
        'disk': 90.0,
        'net_recv_kbps': 5000.0,
        'net_trans_kbps': 3000.0
    }
    thresholds = default_thresholds
    thresholds.update(cfg.get('thresholds', {}))

    print(f"[{now_ts()}] Integrator started. Poll interval: {poll_interval}s")
    print(f"Thresholds: {thresholds}")

    while True:
        try:
            rows = get_latest_metrics(dbcfg, limit=1)
        except Exception as e:
            print(f"[{now_ts()}] DB connection error: {e}")
            time.sleep(poll_interval)
            continue

        if not rows:
            print(f"[{now_ts()}] No metrics found in DB.")
            time.sleep(poll_interval)
            continue

        row = rows[0]
        ts = row.get('timestamp')
        cpu = float(row.get('cpu_usage', 0.0) or 0.0)
        mem = float(row.get('memory_usage', 0.0) or 0.0)
        disk = float(row.get('disk_usage', 0.0) or 0.0)
        rx = float(row.get('net_recv_kbps', 0.0) or 0.0)
        tx = float(row.get('net_trans_kbps', 0.0) or 0.0)

        print(f"[{now_ts()}] {ts} -> CPU:{cpu:.2f}% MEM:{mem:.2f}% DISK:{disk:.2f}% RX:{rx:.2f}KB/s TX:{tx:.2f}KB/s")

        if cpu > thresholds.get('cpu', default_thresholds['cpu']):
            create_incident(sncfg, f'High CPU usage: {cpu:.2f}%', f'CPU at {cpu:.2f}% recorded at {ts}')
        if mem > thresholds.get('memory', default_thresholds['memory']):
            create_incident(sncfg, f'High Memory usage: {mem:.2f}%', f'Memory at {mem:.2f}% recorded at {ts}')
        if disk > thresholds.get('disk', default_thresholds['disk']):
            create_incident(sncfg, f'High Disk usage: {disk:.2f}%', f'Disk usage at {disk:.2f}% recorded at {ts}')
        if rx > thresholds.get('net_recv_kbps', default_thresholds['net_recv_kbps']):
            create_incident(sncfg, f'High Network Inbound: {rx:.2f} KB/s', f'Network inbound at {rx:.2f} KB/s recorded at {ts}')
        if tx > thresholds.get('net_trans_kbps', default_thresholds['net_trans_kbps']):
            create_incident(sncfg, f'High Network Outbound: {tx:.2f} KB/s', f'Network outbound at {tx:.2f} KB/s recorded at {ts}')

        time.sleep(poll_interval)

if __name__ == '__main__':
    main()
