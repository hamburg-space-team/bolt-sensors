cmake_minimum_required(VERSION 3.22)
project(bolt-sensors LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Core hardware independent sensor logic
add_library(Sensors STATIC)
target_sources(Sensors PRIVATE
    sensor-drivers/src/device_base.cpp
    sensor-drivers/src/tmp117.cpp
)

target_include_directories(Sensors PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-interfaces/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-drivers/include
)


# Mock adapters for native host execution
add_library(MockAdapters STATIC)
target_sources(MockAdapters PRIVATE hardware-adapters/src/mock_i2c_bus.cpp)
target_include_directories(MockAdapters PUBLIC 
    ${CMAKE_CURRENT_SOURCE_DIR}/hardware-adapters/include
    ${CMAKE_CURRENT_SOURCE_DIR}/sensor-interfaces/include
)


# Example executables for different boards
set(BOARDS
    "mission-bolt-btc"
)

foreach(BOARD IN LISTS BOARDS)
    string(REPLACE "-" "_" BOARD_TARGET_NAME ${BOARD})

    add_executable(example_${BOARD_TARGET_NAME} examples/${BOARD}/example_${BOARD_TARGET_NAME}.cpp)
    
    target_link_libraries(example_${BOARD_TARGET_NAME} PRIVATE
        Sensors
        MockAdapters
    )

    target_include_directories(example_${BOARD_TARGET_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/common/
    )
endforeach()
