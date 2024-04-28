cmake_minimum_required(VERSION 3.0.2)
project(henes_car_control)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

## Find catkin and any catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
  std_msgs
  geometry_msgs
  sensor_msgs
  nav_msgs
  tf
)

## Declare a catkin package
catkin_package(
  CATKIN_DEPENDS roscpp rospy std_msgs geometry_msgs sensor_msgs nav_msgs tf
)

## Build henes_car_control_node
include_directories(
  include
  ${catkin_INCLUDE_DIRS}
)

## Declare a C++ executable for henes_car_control_node
add_executable(henes_car_control_node src/henes_car_control.cpp)
target_link_libraries(henes_car_control_node ${catkin_LIBRARIES})
add_dependencies(henes_car_control_node ${${PROJECT_NAME}_EXPORTED_TARGETS} ${catkin_EXPORTED_TARGETS})

## Mark executables and/or libraries for installation
install(TARGETS henes_car_control_node
  RUNTIME DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

## Mark other files for installation (e.g., launch and bag files, etc.)
install(DIRECTORY launch/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/launch
)

install(DIRECTORY config/
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}/config
)

