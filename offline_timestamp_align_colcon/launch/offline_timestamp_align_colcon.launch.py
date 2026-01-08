from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess
from launch.substitutions import PathJoinSubstitution
from launch_ros.substitutions import FindPackageShare

def generate_launch_description():
    return LaunchDescription([
        # 1.融合节点，先启动
        Node(
            package='offline_timestamp_align_colcon',         # 包名
            executable='fusion_node_colcon',                  # 可执行文件名
            name='fusion_node_colcon',                        # 节点名
            output='screen',                                  # 输出到控制台
            parameters=[],                                    # 参数
        ),

        # 2.发布节点，延迟1秒启动
        ExecuteProcess(
            cmd=[
                'bash', '-c', 
                'sleep 1; ros2 run offline_timestamp_align_colcon publish_node_colcon '                 # 延迟1秒后启动发布节点
                '--ros-args '
                '-p cam_csv:=/home/cat/foxy_education_data/cam0_aligned.csv '                           # 传递相机的 CSV 参数
                '-p image_dir:=/home/cat/noetic_education_data/cam0/ '                                  # 传递图像目录参数
                '-p imu_csv:=/home/cat/noetic_education_data/imu0_data.csv'                             # 传递 IMU 的 CSV 参数
            ],
            output='screen'
        )
    ])
