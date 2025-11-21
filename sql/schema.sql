CREATE DATABASE IF NOT EXISTS cloud_monitor;
USE cloud_monitor;

CREATE TABLE IF NOT EXISTS metrics (
    id INT AUTO_INCREMENT PRIMARY KEY,
    timestamp TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    cpu_usage FLOAT,
    memory_usage FLOAT,
    disk_usage FLOAT,
    net_recv_kbps FLOAT,
    net_trans_kbps FLOAT
);
