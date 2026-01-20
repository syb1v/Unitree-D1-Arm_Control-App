cmake_minimum_required(VERSION 3.14)
project(D1Control VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt5
find_package(Qt5 REQUIRED COMPONENTS Widgets Core Gui Network)

# Исходники
set(SOURCES
    src/main.cpp
    src/mainwindow.cpp
    src/arm_controller.cpp
    src/pose_manager.cpp
    src/calibration_manager.cpp
    src/joint_widget.cpp
    src/status_widget.cpp
    src/pose_list_widget.cpp
    src/motion_manager.cpp
    src/motion_player.cpp
    src/motion_recorder.cpp
    src/motion_widget.cpp
    src/connection_settings.cpp
    src/calibration_dialog.cpp
    src/cyclonedds_settings.cpp
)

set(HEADERS
    include/mainwindow.h
    include/arm_controller.h
    include/pose_manager.h
    include/calibration_manager.h
    include/joint_widget.h
    include/status_widget.h
    include/pose_list_widget.h
    include/motion_manager.h
    include/motion_player.h
    include/motion_recorder.h
    include/motion_widget.h
    include/connection_settings.h
    include/calibration_dialog.h
    include/cyclonedds_settings.h
)

# Include directories
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# Исполняемый файл
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# Линковка
target_link_libraries(${PROJECT_NAME}
    Qt5::Widgets
    Qt5::Core
    Qt5::Gui
    Qt5::Network
)

# Установка
install(TARGETS ${PROJECT_NAME} DESTINATION bin)
