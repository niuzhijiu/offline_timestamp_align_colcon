# 整体的时间戳
builtin_interfaces/Time timestamp               # 融合后话题的时间戳
# 视觉信息
string image_filename                           # 相机图片文件名
sensor_msgs/Image image                         # 图像数据
# IMU 信息
geometry_msgs/Vector3[] angular_velocity        # 存储多个 IMU 的角速度数据
geometry_msgs/Vector3[] linear_acceleration     # 存储多个 IMU 的加速度数据
float64[] imu_timestamp                         # 存储多个 IMU 的时间戳数据
