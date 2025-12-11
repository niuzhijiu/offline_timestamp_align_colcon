from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        # 1. 融合节点（先启动）
        Node(
            package='offline_timestamp_align_colcon',  # 包名
            executable='fusion_node_colcon',          # 可执行文件名
            name='fusion_node_colcon',                       # 节点名
            output='screen',                          # 输出到控制台
            parameters=[],                            # 参数
        ),

        # 2. 发布节点（延迟 1 秒启动）
        ExecuteProcess(
            cmd=[
                'bash', '-c', 
                'sleep 1; ros2 run offline_timestamp_align_colcon publish_node_colcon '  # 延迟 1 秒后启动发布节点
                '--ros-args '
                '-p cam_csv:=/home/slam/20251211/cam0_aligned.csv '              # 传递相机 CSV 参数
                '-p image_dir:=/home/slam/20251210/cam0/ '                        # 传递图像目录参数
                '-p imu_csv:=/home/slam/20251210/imu0_data.csv'                   # 传递 IMU CSV 参数
            ],
            output='screen'
        )
    ])
