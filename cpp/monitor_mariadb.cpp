// monitor_mariadb.cpp
// Extended: CPU, Memory, Disk usage (%), Network recv/trans (KB/s)
// Uses MariaDB C API to insert metrics.

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <sstream>
#include <algorithm>
#include <sys/statvfs.h>
#include <map>

#include <mysql/mysql.h> // MariaDB / MySQL C API

static unsigned long long last_total = 0;
static unsigned long long last_idle = 0;

struct NetTotals { unsigned long long rx = 0; unsigned long long tx = 0; };
static std::map<std::string, NetTotals> prev_net;

float getCPUUsage() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0f;

    std::string line;
    std::getline(file, line);
    std::istringstream ss(line);
    std::string cpu_label;
    unsigned long long user=0, nice=0, system=0, idle=0, iowait=0, irq=0, softirq=0, steal=0;
    ss >> cpu_label >> user >> nice >> system >> idle >> iowait >> irq >> softirq >> steal;

    unsigned long long idle_time = idle + iowait;
    unsigned long long non_idle = user + nice + system + irq + softirq + steal;
    unsigned long long total = idle_time + non_idle;

    unsigned long long totald = total - last_total;
    unsigned long long idled = idle_time - last_idle;

    float cpu_percent = 0.0f;
    if (totald != 0) cpu_percent = 100.0f * (float)(totald - idled) / (float)totald;

    last_total = total;
    last_idle = idle_time;
    return cpu_percent;
}

float getMemoryUsage() {
    std::ifstream file("/proc/meminfo");
    if (!file.is_open()) return 0.0f;
    std::string key;
    unsigned long long memTotal = 0, memAvailable = 0;
    while (file >> key) {
        if (key == "MemTotal:") file >> memTotal;
        else if (key == "MemAvailable:") { file >> memAvailable; break; }
        else { std::string rest; std::getline(file, rest); }
    }
    if (memTotal == 0) return 0.0f;
    return 100.0f * (float)(memTotal - memAvailable) / (float)memTotal;
}

float getDiskUsagePercent(const char* path = "/") {
    struct statvfs stat;
    if (statvfs(path, &stat) != 0) return 0.0f;
    unsigned long long total = stat.f_blocks * (unsigned long long)stat.f_frsize;
    unsigned long long avail = stat.f_bavail * (unsigned long long)stat.f_frsize;
    if (total == 0) return 0.0f;
    unsigned long long used = total - avail;
    return 100.0f * (float)used / (float)total;
}

std::pair<unsigned long long, unsigned long long> readNetTotals() {
    std::ifstream file("/proc/net/dev");
    unsigned long long tot_rx = 0, tot_tx = 0;
    if (!file.is_open()) return {0,0};
    std::string line;
    std::getline(file, line);
    std::getline(file, line);
    while (std::getline(file, line)) {
        std::istringstream ss(line);
        std::string ifname;
        if (!(ss >> ifname)) continue;
        if (ifname.back() == ':') ifname.pop_back();
        unsigned long long rx_bytes = 0, tx_bytes = 0;
        ss >> rx_bytes;
        std::vector<unsigned long long> vals;
        vals.push_back(rx_bytes);
        unsigned long long v;
        for (int i = 0; i < 15 && ss >> v; ++i) vals.push_back(v);
        if (vals.size() >= 9) {
            tx_bytes = vals[8];
        } else {
            std::replace(line.begin(), line.end(), ':', ' ');
            std::istringstream ss2(line);
            std::string name; ss2 >> name;
            ss2 >> rx_bytes;
            for (int i=0;i<8;i++) ss2 >> v;
            ss2 >> tx_bytes;
        }
        if (ifname == "lo") continue;
        tot_rx += rx_bytes;
        tot_tx += tx_bytes;
    }
    return {tot_rx, tot_tx};
}

std::pair<float, float> getNetworkKbps(int interval_seconds) {
    auto current = readNetTotals();
    static unsigned long long last_rx = 0;
    static unsigned long long last_tx = 0;

    if (last_rx == 0 && last_tx == 0) {
        last_rx = current.first;
        last_tx = current.second;
        return {0.0f, 0.0f};
    }

    unsigned long long rx_diff = (current.first >= last_rx) ? (current.first - last_rx) : 0;
    unsigned long long tx_diff = (current.second >= last_tx) ? (current.second - last_tx) : 0;

    last_rx = current.first;
    last_tx = current.second;

    float rx_kbps = (float)rx_diff / 1024.0f / (float)interval_seconds;
    float tx_kbps = (float)tx_diff / 1024.0f / (float)interval_seconds;
    return {rx_kbps, tx_kbps};
}

int main(int argc, char** argv) {
    const char* db_host = "127.0.0.1";
    const char* db_user = "monitor";
    const char* db_pass = "1405";
    const char* db_name = "cloud_monitor";
    unsigned int db_port = 3306;
    int interval_seconds = 5;

    if (mysql_library_init(0, nullptr, nullptr)) {
        std::cerr << "Could not initialize MariaDB client library" << std::endl;
        return 1;
    }

    MYSQL *conn = mysql_init(nullptr);
    if (!conn) { std::cerr << "mysql_init failed" << std::endl; mysql_library_end(); return 2; }

    my_bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(conn, db_host, db_user, db_pass, db_name, db_port, nullptr, 0)) {
        std::cerr << "Connection failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn); mysql_library_end(); return 3; }

    std::cout << "Connected to MariaDB at " << db_host << " as " << db_user << std::endl;

    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    getCPUUsage();
    getNetworkKbps(interval_seconds);

    while (true) {
        float cpu = getCPUUsage();
        float mem = getMemoryUsage();
        float disk = getDiskUsagePercent("/");
        auto net = getNetworkKbps(interval_seconds);
        float rx_kbps = net.first;
        float tx_kbps = net.second;

        std::cout << std::fixed << std::setprecision(2)
                  << "CPU: " << cpu << "%  MEM: " << mem << "%  DISK: " << disk << "%  RX: " << rx_kbps << " KB/s  TX: " << tx_kbps << " KB/s"
                  << std::endl;

        char query[512];
        int qlen = snprintf(query, sizeof(query),
            "INSERT INTO metrics (cpu_usage, memory_usage, disk_usage, net_recv_kbps, net_trans_kbps) VALUES (%.2f, %.2f, %.2f, %.2f, %.2f)",
            cpu, mem, disk, rx_kbps, tx_kbps);
        if (qlen <= 0 || qlen >= (int)sizeof(query)) {
            std::cerr << "Query build error" << std::endl;
        } else {
            if (mysql_query(conn, query)) {
                std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
            }
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }

    mysql_close(conn);
    mysql_library_end();
    return 0;
}
