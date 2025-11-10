cmake_minimum_required(VERSION 3.5)
project(cloud_monitor_agent)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra")

find_package(PkgConfig REQUIRED)
pkg_check_modules(MARIADB mariadb) # uses pkg-config if available

if(MARIADB_FOUND)
    include_directories(${MARIADB_INCLUDE_DIRS})
    link_directories(${MARIADB_LIBRARY_DIRS})
    add_executable(cloud_monitor_agent monitor_mariadb.cpp)
    target_link_libraries(cloud_monitor_agent ${MARIADB_LIBRARIES} pthread)
else()
    # Fallback: try common include/lib locations
    include_directories(/usr/include/mysql /usr/include)
    link_directories(/usr/lib /usr/local/lib)
    add_executable(cloud_monitor_agent monitor_mariadb.cpp)
    target_link_libraries(cloud_monitor_agent mariadb pthread)
endif()

