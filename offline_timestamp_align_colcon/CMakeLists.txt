cmake_minimum_required(VERSION 3.5)
project(offline_timestamp_align_colcon)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)

rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/FusedState.msg"
  DEPENDENCIES sensor_msgs geometry_msgs builtin_interfaces
)

add_executable(cam_w_norm_calculate_colcon
  src/cam_w_norm_calculate_colcon.cpp
)
ament_target_dependencies(cam_w_norm_calculate_colcon
  rclcpp 
  OpenCV
)
target_link_libraries(cam_w_norm_calculate_colcon
  ${OpenCV_LIBS}
)

add_executable(imu_w_norm_calculate_colcon
  src/imu_w_norm_calculate_colcon.cpp
)

add_executable(time_offset_calculate_portion_colcon
  src/time_offset_calculate_portion_colcon.cpp
)

add_executable(time_offset_calculate_norm_colcon
  src/time_offset_calculate_norm_colcon.cpp
)

add_executable(timestamp_correct_colcon
  src/timestamp_correct_colcon.cpp
)

add_executable(publish_node_colcon
  src/publish_node_colcon.cpp
)
ament_target_dependencies(publish_node_colcon
  rclcpp
  sensor_msgs
  cv_bridge
  OpenCV
)
target_link_libraries(publish_node_colcon
  ${OpenCV_LIBS}
)

add_executable(fusion_node_colcon
  src/fusion_node_colcon.cpp
)
ament_target_dependencies(fusion_node_colcon
  rclcpp
  sensor_msgs
  geometry_msgs
)

rosidl_target_interfaces(fusion_node_colcon
  ${PROJECT_NAME} "rosidl_typesupport_cpp"
)

install(TARGETS
  cam_w_norm_calculate_colcon
  imu_w_norm_calculate_colcon
  time_offset_calculate_portion_colcon
  time_offset_calculate_norm_colcon
  timestamp_correct_colcon
  publish_node_colcon
  fusion_node_colcon
  DESTINATION lib/${PROJECT_NAME}
)

if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  ament_lint_auto_find_test_dependencies()
endif()

install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

ament_package()
