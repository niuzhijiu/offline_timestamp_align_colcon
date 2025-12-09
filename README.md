# offline_timestamp_align_colcon

## 简介
一个帮助初学者熟悉ROS2的foxy版本和时间标定的离线时间戳粗对齐项目

## 目录结构
offline_timestamp_align_colcon
    ├── CMakeLists.txt
    ├── include
    │   └── offline_timestamp_align_colcon
    ├── launch
    │   └── offline_timestamp_align_colcon.launch.py
    ├── msg
    │   └── FusedState.msg
    ├── package.xml
    └── src
        ├── cam_w_norm_calculate_colcon.cpp
        ├── fusion_node_colcon.cpp
        ├── imu_w_norm_calculate_colcon.cpp
        ├── publish_node_colcon.cpp
        ├── time_offset_calculate_norm_colcon.cpp
        ├── time_offset_calculate_portion_colcon.cpp
        └── timestamp_correct_colcon.cpp

    
## 文件功能简介    
CMakeLists.txt：声明编译规则
package.xml：声明依赖和包的基本信息
offline_timestamp_align_colcon.launch.py：一键启动ROS相关的发布和订阅节点
FusedState.msg：定义融合后发布的消息
cam_w_norm_calculate_colcon.cpp：计算三轴视觉角速度及模值
imu_w_norm_calculate_colcon.cpp：计算IMU角速度模值
time_offset_calculate_norm_colcon.cpp：凭借角速度模值估算时间偏移
time_offset_calculate_portion_colcon.cpp：凭借三轴角速度估算时间偏移
timestamp_correct_colcon.cpp：时间戳修正
publish_node_colcon.cpp：ROS发布节点
fusion_node_colcon.cpp：ROS信息融合节点

## 文件依赖关系示意图
![file dependency](file_dependency_colcon.png)

## 节点及话题通信示意图
![file dependency](node_and_topic_communication_colcon.png)
