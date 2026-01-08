// 声明头文件和依赖
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <geometry_msgs/msg/vector3.hpp>
#include "offline_timestamp_align_colcon/msg/fused_state.hpp"
#include <vector>
#include <string>
#include <csignal>
#include <memory>

// 定义FusionNode类
class FusionNode : public rclcpp::Node {
public:
    // 构造函数：初始化ROS2节点fusion_node_colcon
    FusionNode() : Node("fusion_node_colcon") {
        // 创建发布器：发布 FusedState 消息到 /fused_topic，队列大小 10
        pub_ = this->create_publisher<offline_timestamp_align_colcon::msg::FusedState>(
            "/fused_topic", 10);

        // 订阅相机话题 /cam_image，队列大小 100
        sub_cam_ = this->create_subscription<sensor_msgs::msg::Image>(
            "/cam_image", 100,
            std::bind(&FusionNode::camCallback, this, std::placeholders::_1));
        
        // 订阅IMU话题 /imu_data，队列大小 1000
        sub_imu_ = this->create_subscription<sensor_msgs::msg::Imu>(
            "/imu_data", 1000,
            std::bind(&FusionNode::imuCallback, this, std::placeholders::_1));

        // 初始化成员变量
        first_frame_ = true;
        frame_count_ = 0;
        imu_total_count_ = 0;

        RCLCPP_INFO(this->get_logger(), "Fusion node (fusion_node_colcon) initialized!");
    }

    // IMU回调函数：缓存IMU数据，等待相机帧配对
    void imuCallback(const sensor_msgs::msg::Imu::SharedPtr msg) {
        imu_buffer_.push_back(*msg);
    }

    // 相机回调函数：融合对应时间区间的IMU数据并发布
    void camCallback(const sensor_msgs::msg::Image::SharedPtr cam_msg) {
        double t_curr = this->getTimeInSeconds(cam_msg->header.stamp);

        // 第一帧特殊处理：仅发布图像，无IMU数据
        if (first_frame_) {
            auto fused_msg = offline_timestamp_align_colcon::msg::FusedState();
            // 时间戳赋值
            fused_msg.timestamp = cam_msg->header.stamp;
            // 图像文件名
            fused_msg.image_filename = cam_msg->header.frame_id;
            // 图像数据
            fused_msg.image = *cam_msg;

            // 发布融合消息
            pub_->publish(fused_msg);
            RCLCPP_INFO(this->get_logger(), "Published first fused data: %s (no IMU)",
                     fused_msg.image_filename.c_str());

            // 更新状态
            first_frame_ = false;
            last_cam_stamp_ = cam_msg->header.stamp;
            frame_count_++;
            return;
        }

        // 上一帧相机时间戳
        double t_prev = this->getTimeInSeconds(last_cam_stamp_);

        // 筛选 [t_prev, t_curr) 区间内的IMU数据
        std::vector<sensor_msgs::msg::Imu> imu_selected;
        for (auto& imu : imu_buffer_) {
            double t_imu = this->getTimeInSeconds(imu.header.stamp);
            if (t_prev <= t_imu && t_imu < t_curr) {
                imu_selected.push_back(imu);
            }
        }

        // 填充融合消息
        auto fused_msg = offline_timestamp_align_colcon::msg::FusedState();
        fused_msg.timestamp = cam_msg->header.stamp;
        fused_msg.image_filename = cam_msg->header.frame_id;
        fused_msg.image = *cam_msg;

        // 填充IMU数据包括角速度、加速度、时间戳
        for (auto& imu : imu_selected) {
            fused_msg.angular_velocity.push_back(imu.angular_velocity);
            fused_msg.linear_acceleration.push_back(imu.linear_acceleration);
            fused_msg.imu_timestamp.push_back(this->getTimeInSeconds(imu.header.stamp));
        }

        // 发布融合消息
        pub_->publish(fused_msg);
        RCLCPP_INFO(this->get_logger(), "Published fused data: %s with %lu IMU samples",
                 fused_msg.image_filename.c_str(), imu_selected.size());

        // 更新状态
        last_cam_stamp_ = cam_msg->header.stamp;
        frame_count_++;
        imu_total_count_ += imu_selected.size();
    }

    // 节点关闭时的统计输出
    void shutdown() {
        RCLCPP_INFO(this->get_logger(), "Fusion node finished. Total camera frames: %d, total IMU samples fused: %d",
                 frame_count_, imu_total_count_);
    }

    // 工具函数：将 ROS2 Time 转为秒级浮点数
    double getTimeInSeconds(const rclcpp::Time& time) {
        return static_cast<double>(time.nanoseconds()) / 1e9;
    }

private:
    // ROS2 发布器/订阅器
    rclcpp::Publisher<offline_timestamp_align_colcon::msg::FusedState>::SharedPtr pub_;
    rclcpp::Subscription<sensor_msgs::msg::Image>::SharedPtr sub_cam_;
    rclcpp::Subscription<sensor_msgs::msg::Imu>::SharedPtr sub_imu_;

    // IMU数据缓存
    std::vector<sensor_msgs::msg::Imu> imu_buffer_;
    // 上一帧相机时间戳
    rclcpp::Time last_cam_stamp_;

    // 状态标记与计数器
    bool first_frame_;
    int frame_count_;
    int imu_total_count_;
};

// 全局节点指针，用于信号处理
std::shared_ptr<FusionNode> g_node = nullptr;

// Ctrl+C 信号处理函数
void sigintHandler(int sig) {
    (void)sig;                   // 消除未使用参数警告
    if (g_node) {
        g_node->shutdown();
    }
    rclcpp::shutdown();
}

int main(int argc, char** argv) {
    // 初始化 ROS2 节点
    rclcpp::init(argc, argv);
    
    // 创建 FusionNode 实例
    g_node = std::make_shared<FusionNode>();

    // 注册 SIGINT 信号处理
    signal(SIGINT, sigintHandler);

    // 进入 ROS2 回调循环
    rclcpp::spin(g_node);

    // 确保节点关闭时执行清理
    if (g_node) {
        g_node->shutdown();
    }
    rclcpp::shutdown();
    return 0;
}
