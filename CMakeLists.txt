cmake_minimum_required(VERSION 3.9.0)
set(CMAKE_CXX_STANDARD 14)
project(realsense-sandbox VERSION 1.0.0 DESCRIPTION "realsense-sandbox")

# log level
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DSPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE")

add_subdirectory(sandbox)
add_subdirectory(third-party)
