from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        # 1. 融合节点（先启动）
        Node(
            package='offline_timestamp_align_colcon',  # 注意：ROS 2 包名需与 CMakeLists.txt 中的 project 一致
            executable='fusion_node_colcon',          # 可执行文件名（与 CMakeLists 中 add_executable 定义的一致）
            name='fusion_node_colcon',                       # 节点名称（自定义，保持与原逻辑一致）
            output='screen',                          # 输出到控制台
            parameters=[],                            # 若有参数可在此添加
        ),

        # 2. 发布节点（延迟 1 秒启动，与原逻辑一致）
        ExecuteProcess(
            cmd=[
                'bash', '-c', 
                'sleep 1; ros2 run offline_timestamp_align_colcon publish_node_colcon '  # 延迟 1 秒后启动发布节点
                '--ros-args '
                '-p cam_csv:=/home/slam/20251208_test/cam0_aligned.csv '              # 传递相机 CSV 参数
                '-p image_dir:=/home/slam/20251205_test/cam0/ '                        # 传递图像目录参数
                '-p imu_csv:=/home/slam/20251205_test/imu0_data.csv'                   # 传递 IMU CSV 参数
            ],
            output='screen'
        )
    ])
