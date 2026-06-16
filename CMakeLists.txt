cmake_minimum_required(VERSION 3.22)
project(bolt-sensors LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


add_library(Sensors STATIC)
target_sources(Sensors PRIVATE
    sensor-drivers/src/device_base.cpp
    sensor-drivers/src/tmp117.cpp
)

target_include_directories(Sensors PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-interfaces/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-drivers/include
)


# Hardware Adapters target for VS Code IntelliSense
add_library(HardwareAdapters STATIC)

target_sources(HardwareAdapters PRIVATE 
    hardware-adapters/src/cmsis_i2c_bus.cpp
)

target_include_directories(HardwareAdapters PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware-adapters/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-interfaces/include

    ${CMAKE_CURRENT_SOURCE_DIR}/hardware-adapters/vendor/cmsis
)


# I2C adapter providing mock data for example executable
add_library(MockAdapter STATIC)
target_sources(MockAdapter PRIVATE hardware-adapters/src/mock_i2c_bus.cpp)
target_include_directories(MockAdapter PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware-adapters/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-interfaces/include
)

# Library component naming example
add_library(HST::Sensor::Interfaces ALIAS Sensors)
add_library(HST::Sensor::Drivers::TMP117 ALIAS Sensors)
add_library(HST::Sensor::Adapters::CMSISI2C ALIAS Sensors)     # production
add_library(HST::Sensor::Adapters::MockI2C ALIAS MockAdapter)  # local mock data I2C adapter


# Example executables
add_executable(demo_tmp117 examples/demo_tmp117.cpp)
target_link_libraries(demo_tmp117 PRIVATE
    HST::Sensor::Drivers::TMP117
    HST::Sensor::Adapters::MockI2C
)
