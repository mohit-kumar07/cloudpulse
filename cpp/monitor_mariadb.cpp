// monitor_mariadb.cpp
// C++ resource monitor that inserts CPU and memory usage into MariaDB using the C API.
//
// Build with: see CMakeLists.txt and instructions in README.
// Requires mariadb (Connector/C) dev headers installed.

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#include <vector>
#include <stdexcept>
#include <cstring>
#include <sstream>      // <<-- needed for istringstream

#include <mysql/mysql.h> // MariaDB / MySQL C API

// read first line of /proc/stat, compute CPU usage percentage (simplified)
static unsigned long long last_total = 0;
static unsigned long long last_idle = 0;

float getCPUUsage() {
    std::ifstream file("/proc/stat");
    if (!file.is_open()) return 0.0f;

    std::string line;
    std::getline(file, line);
    // Parse: cpu  user nice system idle iowait irq softirq steal ...
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
    if (totald != 0) {
        cpu_percent = 100.0f * (float)(totald - idled) / (float)totald;
    }

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
        if (key == "MemTotal:") {
            file >> memTotal;
        } else if (key == "MemAvailable:") {
            file >> memAvailable;
            break;
        } else {
            std::string rest;
            std::getline(file, rest);
        }
    }
    if (memTotal == 0) return 0.0f;
    return 100.0f * (float)(memTotal - memAvailable) / (float)memTotal;
}

int main(int argc, char** argv) {
    // DB config - you can later read these from a file or CLI args
    const char* db_host = "127.0.0.1";
    const char* db_user = "monitor";
    const char* db_pass = "1405";
    const char* db_name = "cloud_monitor";
    unsigned int db_port = 3306;
    int interval_seconds = 5;

    // Initialize mysql library (optional)
    if (mysql_library_init(0, nullptr, nullptr)) {
        std::cerr << "Could not initialize MariaDB client library" << std::endl;
        return 1;
    }

    MYSQL *conn = mysql_init(nullptr);
    if (!conn) {
        std::cerr << "mysql_init failed" << std::endl;
        mysql_library_end();
        return 2;
    }

    // enable automatic reconnection
    my_bool reconnect = 1;
    mysql_options(conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (!mysql_real_connect(conn, db_host, db_user, db_pass, db_name, db_port, nullptr, 0)) {
        std::cerr << "Connection failed: " << mysql_error(conn) << std::endl;
        mysql_close(conn);
        mysql_library_end();
        return 3;
    }

    std::cout << "Connected to MariaDB at " << db_host << " as " << db_user << std::endl;

    // warm up CPU delta
    std::this_thread::sleep_for(std::chrono::milliseconds(500));
    getCPUUsage();

    while (true) {
        try {
            float cpu = getCPUUsage();
            float mem = getMemoryUsage();

            std::cout << std::fixed << std::setprecision(2)
                      << "CPU: " << cpu << "%  MEM: " << mem << "%" << std::endl;

            // Use snprintf to build query safely for numeric values
            char query[256];
            int qlen = snprintf(query, sizeof(query),
                "INSERT INTO metrics (cpu_usage, memory_usage) VALUES (%.2f, %.2f)",
                cpu, mem);
            if (qlen <= 0 || qlen >= (int)sizeof(query)) {
                std::cerr << "Query build error" << std::endl;
            } else {
                if (mysql_query(conn, query)) {
                    std::cerr << "Insert failed: " << mysql_error(conn) << std::endl;
                    // try to reconnect
                    if (mysql_ping(conn) != 0) {
                        std::cerr << "Ping failed, trying to reconnect..." << std::endl;
                        mysql_close(conn);
                        conn = mysql_init(nullptr);
                        if (!mysql_real_connect(conn, db_host, db_user, db_pass, db_name, db_port, nullptr, 0)) {
                            std::cerr << "Reconnect failed: " << mysql_error(conn) << std::endl;
                            // wait before next attempt
                            std::this_thread::sleep_for(std::chrono::seconds(5));
                        } else {
                            std::cerr << "Reconnected successfully" << std::endl;
                        }
                    }
                }
            }
        } catch (const std::exception &ex) {
            std::cerr << "Runtime error: " << ex.what() << std::endl;
        }

        std::this_thread::sleep_for(std::chrono::seconds(interval_seconds));
    }

    mysql_close(conn);
    mysql_library_end();
    return 0;
}
