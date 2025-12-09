// 声明头文件和依赖
#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/image.hpp>
#include <sensor_msgs/msg/imu.hpp>
#include <cv_bridge/cv_bridge.h>
#include <opencv2/opencv.hpp>

#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <algorithm>  // 用于std::remove

// 把相机和imu的csv文件映射成对应的结构体
struct CamData {
    double timestamp;
    std::string filename;
};

struct ImuData {
    double timestamp;
    double acc_x, acc_y, acc_z;
    double gyro_x, gyro_y, gyro_z;
};

// 读取相机CSV文件
std::vector<CamData> loadCamCsv(const std::string& path) {
    std::vector<CamData> data;
    std::ifstream file(path);
    
    if (!file.is_open()) {
        RCLCPP_ERROR(rclcpp::get_logger("publish_node_colcon"), "Failed to open camera CSV file! Path: %s", path.c_str());
        return data;
    }

    std::string line;
    bool headerSkipped = false;
    int valid_data_count = 0;
    
    while (std::getline(file, line)) {
        if (!headerSkipped) { 
            headerSkipped = true; 
            continue; 
        }

        // 清除行内隐藏的特殊字符
        line.erase(std::remove(line.begin(), line.end(), '\n'), line.end());
        line.erase(std::remove(line.begin(), line.end(), '\r'), line.end());

        std::stringstream ss(line);
        std::string ts, fname;
        std::getline(ss, ts, ',');
        std::getline(ss, fname, ',');

        // 清除文件名前后的空白字符
        size_t fname_start = fname.find_first_not_of(" \t");
        size_t fname_end = fname.find_last_not_of(" \t");
        if (fname_start == std::string::npos || fname_end == std::string::npos) {
            RCLCPP_WARN(rclcpp::get_logger("publish_node_colcon"), "Skip invalid line (empty filename): %s", line.c_str());
            continue;
        }
        fname = fname.substr(fname_start, fname_end - fname_start + 1);

        // 验证时间戳
        double timestamp = 0.0;
        try {
            timestamp = std::stod(ts);
        } catch (const std::invalid_argument& e) {
            RCLCPP_WARN(rclcpp::get_logger("publish_node_colcon"), "Skip line (invalid timestamp): %s, Error: %s", line.c_str(), e.what());
            continue;
        } catch (const std::out_of_range& e) {
            RCLCPP_WARN(rclcpp::get_logger("publish_node_colcon"), "Skip line (timestamp out of range): %s, Error: %s", line.c_str(), e.what());
            continue;
        }

        if (!ts.empty() && !fname.empty()) {
            CamData d;
            d.timestamp = timestamp;
            d.filename = fname;
            data.push_back(d);
            valid_data_count++;
        }
    }

    file.close();
    RCLCPP_INFO(rclcpp::get_logger("publish_node_colcon"), "Camera CSV loaded: Path=%s, Valid data count=%d, Total lines processed=%d", 
             path.c_str(), valid_data_count, (valid_data_count + 1));

    return data;
}

// 读取IMU CSV文件
std::vector<ImuData> loadImuCsv(const std::string& path) {
    std::vector<ImuData> data;
    std::ifstream file(path);
    std::string line;
    bool headerSkipped = false;
    
    while (std::getline(file, line)) {
        if (!headerSkipped) { 
            headerSkipped = true; 
            continue; 
        }

        std::stringstream ss(line);
        std::string ts, ax, ay, az, gx, gy, gz;
        std::getline(ss, ts, ',');
        std::getline(ss, ax, ',');
        std::getline(ss, ay, ',');
        std::getline(ss, az, ',');
        std::getline(ss, gx, ',');
        std::getline(ss, gy, ',');
        std::getline(ss, gz, ',');

        if (!ts.empty()) {
            try {
                ImuData d;
                d.timestamp = std::stod(ts);
                d.acc_x = std::stod(ax);
                d.acc_y = std::stod(ay);
                d.acc_z = std::stod(az);
                d.gyro_x = std::stod(gx);
                d.gyro_y = std::stod(gy);
                d.gyro_z = std::stod(gz);
                data.push_back(d);
            } catch (const std::exception& e) {
                RCLCPP_WARN(rclcpp::get_logger("publish_node_colcon"), "Skip invalid IMU line: %s, Error: %s", line.c_str(), e.what());
            }
        }
    }
    return data;
}

class PublishNode : public rclcpp::Node {
public:
    PublishNode() : Node("publish_node_colcon") {  
        // 声明并获取参数
        this->declare_parameter<std::string>("cam_csv", "/home/slam/20251208_test/cam0_aligned.csv");
        this->declare_parameter<std::string>("image_dir", "/home/slam/20251205_test/cam0/");
        this->declare_parameter<std::string>("imu_csv", "/home/slam/20251205_test/imu0_data.csv");

        this->get_parameter("cam_csv", cam_csv_path_);
        this->get_parameter("image_dir", image_dir_);
        this->get_parameter("imu_csv", imu_csv_path_);

        // 创建发布者
        pub_cam_ = this->create_publisher<sensor_msgs::msg::Image>("/cam_image", 100);
        pub_imu_ = this->create_publisher<sensor_msgs::msg::Imu>("/imu_data", 1000);

        // 加载CSV数据
        cam_data_ = loadCamCsv(cam_csv_path_);
        imu_data_ = loadImuCsv(imu_csv_path_);

        // 日志输出使用节点内置logger，自动关联publish_node_colcon
        RCLCPP_INFO(this->get_logger(), "camera CSV load: %s", cam_csv_path_.c_str());
        RCLCPP_INFO(this->get_logger(), "the number of cam: %zu", cam_data_.size());
        RCLCPP_INFO(this->get_logger(), "IMU CSV load: %s", imu_csv_path_.c_str());
        RCLCPP_INFO(this->get_logger(), "the number of imu: %zu", imu_data_.size());

        // 创建定时器
        timer_ = this->create_wall_timer(
            std::chrono::milliseconds(4),  // 约250Hz（1000/4≈250）
            std::bind(&PublishNode::publishData, this)
        );

        // 等待订阅者
        RCLCPP_INFO(this->get_logger(), "Waiting for subscribers to connect...");
    }

private:
    void publishData() {
        // 检查是否还有数据需要发布
        if ((i_ >= cam_data_.size() && j_ >= imu_data_.size()) || !rclcpp::ok()) {
            timer_->cancel();  // 停止定时器
            RCLCPP_INFO(this->get_logger(), "Data publish finished. Total cam: %zu, imu: %zu", 
                      cam_data_.size(), imu_data_.size());
            RCLCPP_INFO(this->get_logger(), "Waiting 2.0 s to let messages flush...");
            rclcpp::sleep_for(std::chrono::seconds(2));
            rclcpp::shutdown();
            return;
        }

        // 等待订阅者连接
        if (pub_cam_->get_subscription_count() == 0 || pub_imu_->get_subscription_count() == 0) {
            return;
        }

        // 发布相机数据
        if (i_ < cam_data_.size() && (j_ >= imu_data_.size() || cam_data_[i_].timestamp <= imu_data_[j_].timestamp)) {
            std::string img_path = image_dir_ + cam_data_[i_].filename;
            RCLCPP_INFO(this->get_logger(), "Trying to read image: %s (index: %zu)", img_path.c_str(), i_);

            cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);
            if (!img.empty()) {
                RCLCPP_INFO(this->get_logger(), "Image read successfully, publishing (index: %zu)", i_);
                
                sensor_msgs::msg::Image msg;
                msg.header.stamp = rclcpp::Time(static_cast<int64_t>(cam_data_[i_].timestamp * 1e9));
                msg.header.frame_id = cam_data_[i_].filename;
                msg.height = img.rows;
                msg.width = img.cols;
                msg.encoding = "mono8";
                msg.is_bigendian = false;
                msg.step = img.cols;
                msg.data.assign(img.data, img.data + img.total());

                pub_cam_->publish(msg);
            } else {
                RCLCPP_ERROR(this->get_logger(), "Failed to read image! Path: %s (index: %zu)", img_path.c_str(), i_);
            }
            i_++;
        }
        // 发布IMU数据
        else if (j_ < imu_data_.size()) {
            sensor_msgs::msg::Imu imu_msg;
            imu_msg.header.stamp = rclcpp::Time(static_cast<int64_t>(imu_data_[j_].timestamp * 1e9));
            imu_msg.header.frame_id = "imu";
            imu_msg.linear_acceleration.x = imu_data_[j_].acc_x;
            imu_msg.linear_acceleration.y = imu_data_[j_].acc_y;
            imu_msg.linear_acceleration.z = imu_data_[j_].acc_z;
            imu_msg.angular_velocity.x = imu_data_[j_].gyro_x;
            imu_msg.angular_velocity.y = imu_data_[j_].gyro_y;
            imu_msg.angular_velocity.z = imu_data_[j_].gyro_z;

            pub_imu_->publish(imu_msg);
            j_++;
        }
    }

    // 成员变量
    rclcpp::Publisher<sensor_msgs::msg::Image>::SharedPtr pub_cam_;
    rclcpp::Publisher<sensor_msgs::msg::Imu>::SharedPtr pub_imu_;
    rclcpp::TimerBase::SharedPtr timer_;
    
    std::string cam_csv_path_;
    std::string image_dir_;
    std::string imu_csv_path_;
    
    std::vector<CamData> cam_data_;
    std::vector<ImuData> imu_data_;
    
    size_t i_ = 0, j_ = 0;
};

int main(int argc, char** argv) {
    rclcpp::init(argc, argv);
    // 创建节点实例
    auto node = std::make_shared<PublishNode>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
