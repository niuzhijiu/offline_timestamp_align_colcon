cmake_minimum_required(VERSION 3.5)
project(offline_timestamp_align_colcon)

if(NOT CMAKE_C_STANDARD)
  set(CMAKE_C_STANDARD 99)
  set(CMAKE_C_STANDARD_REQUIRED ON)
endif()
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 14)  
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(sensor_msgs REQUIRED)
find_package(geometry_msgs REQUIRED)
find_package(cv_bridge REQUIRED)
find_package(OpenCV REQUIRED)
find_package(rosidl_default_generators REQUIRED)
find_package(builtin_interfaces REQUIRED)

# 生成自定义消息接口
rosidl_generate_interfaces(${PROJECT_NAME}
  "msg/FusedState.msg"
  DEPENDENCIES sensor_msgs geometry_msgs builtin_interfaces
)

# 生成可执行文件
add_executable(cam_w_norm_calculate_colcon src/cam_w_norm_calculate_colcon.cpp)
add_executable(imu_w_norm_calculate_colcon src/imu_w_norm_calculate_colcon.cpp)
add_executable(time_offset_calculate_portion_colcon src/time_offset_calculate_portion_colcon.cpp)
add_executable(time_offset_calculate_norm_colcon src/time_offset_calculate_norm_colcon.cpp)
add_executable(timestamp_correct_colcon src/timestamp_correct_colcon.cpp)
add_executable(publish_node_colcon src/publish_node_colcon.cpp)
add_executable(fusion_node_colcon src/fusion_node_colcon.cpp)

# 给编译目标自动关联 ROS2 包的所有依赖项
ament_target_dependencies(cam_w_norm_calculate_colcon
  rclcpp 
  OpenCV
)
ament_target_dependencies(imu_w_norm_calculate_colcon
  rclcpp
  sensor_msgs
)
ament_target_dependencies(time_offset_calculate_portion_colcon
  rclcpp
  sensor_msgs
)
ament_target_dependencies(time_offset_calculate_norm_colcon
  rclcpp
  sensor_msgs
)
ament_target_dependencies(timestamp_correct_colcon
  rclcpp
  sensor_msgs
)
ament_target_dependencies(publish_node_colcon
  rclcpp
  sensor_msgs
  cv_bridge
  OpenCV
)
ament_target_dependencies(fusion_node_colcon
  rclcpp
  sensor_msgs
  geometry_msgs
  rosidl_default_runtime
)

# 给指定的编译目标显式链接所需的二进制库文件
target_link_libraries(cam_w_norm_calculate_colcon
  ${OpenCV_LIBS}
)
target_link_libraries(publish_node_colcon
  ${OpenCV_LIBS}
)

# 将自定义 ROS2 接口的 C++ 类型支持代码显式链接到指定的编译目标
rosidl_target_interfaces(fusion_node_colcon
  ${PROJECT_NAME} "rosidl_typesupport_cpp"
)

# 安装可执行文件
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

# 安装launch文件
install(DIRECTORY launch/
  DESTINATION share/${PROJECT_NAME}/launch
)

# 测试模块
if(BUILD_TESTING)
  find_package(ament_lint_auto REQUIRED)
  set(ament_cmake_copyright_FOUND TRUE)
  set(ament_cmake_cpplint_FOUND TRUE)
  set(ament_cmake_flake8_FOUND TRUE)
  set(ament_cmake_pep257_FOUND TRUE)
  ament_lint_auto_find_test_dependencies()
endif()

# 导出依赖
ament_export_dependencies(
  rosidl_default_generators
  rosidl_default_runtime
  rclcpp
  sensor_msgs
  geometry_msgs
  cv_bridge
  OpenCV
  builtin_interfaces
)

ament_package()
