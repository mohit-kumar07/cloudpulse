# integrator.py
# Reads latest metrics from MariaDB.

import argparse
import json
import time
import mysql.connector
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

def now_ts():
    return datetime.utcnow().strftime("%Y-%m-%d %H:%M:%S UTC")

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--config', required=True, help="Path to config.json")
    args = parser.parse_args()

    cfg = load_config(args.config)
    dbcfg = cfg['mysql']
    poll_interval = cfg.get('poll_interval', 30)

    print(f"[{now_ts()}] Integrator started. Poll interval: {poll_interval}s")

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
        time.sleep(poll_interval)

if __name__ == '__main__':
    main()
