cmake_minimum_required(VERSION 3.5)
project(marm_code)

SET(CMAKE_CXX_STANDARD 17)

# Include directories for CycloneDDS and Unitree SDK2
include_directories(
    /usr/local/include
    /usr/local/include/ddscxx
)

# Link directories
link_directories(/usr/local/lib)

# Link libraries
link_libraries(unitree_sdk2 ddsc ddscxx rt pthread)

add_executable(joint_angle_control src/joint_angle_control.cpp src/msg/ArmString_.cpp)
add_executable(multiple_joint_angle_control src/multiple_joint_angle_control.cpp src/msg/ArmString_.cpp)
add_executable(joint_enable_control src/joint_enable_control.cpp src/msg/ArmString_.cpp)
add_executable(arm_zero_control src/arm_zero_control.cpp src/msg/ArmString_.cpp)
add_executable(get_arm_joint_angle src/get_arm_joint_angle.cpp src/msg/ArmString_.cpp src/msg/PubServoInfo_.cpp)
add_executable(udp_relay src/udp_relay.cpp src/msg/ArmString_.cpp src/msg/PubServoInfo_.cpp)
add_executable(slow_move_test src/slow_move_test.cpp src/msg/ArmString_.cpp)