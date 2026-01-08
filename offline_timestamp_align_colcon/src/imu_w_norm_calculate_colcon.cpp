// 声明头文件和依赖
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cmath>
#include <vector>
#include <iomanip>

// 定义数据结构来保存一行原始imu数据
struct ImuData {
    std::string timestamp_sec;
    std::string acc_x, acc_y, acc_z;
    std::string gyro_x, gyro_y, gyro_z;
};

// 读取CSV文件
// 定义读取函数readImuCsv
std::vector<ImuData> readImuCsv(const std::string& filename) {
    // 存储所有IMU数据的向量
    std::vector<ImuData> data;
    // 打开文件并检查成功与否
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open the file: " << filename << std::endl;
        return data;
    }

    // 存储每行内容并且跳过表头
    std::string line;
    // 是否跳过表头标志
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        if (!headerSkipped) {  
            headerSkipped = true;
            continue;
        }

        // 将当前行转换为字符串流，用于分割
        std::stringstream ss(line);
        ImuData imu;

        // 按逗号分割字符串，依次读取各字段到ImuData成员
        std::getline(ss, imu.timestamp_sec, ',');
        std::getline(ss, imu.acc_x, ',');
        std::getline(ss, imu.acc_y, ',');
        std::getline(ss, imu.acc_z, ',');
        std::getline(ss, imu.gyro_x, ',');
        std::getline(ss, imu.gyro_y, ',');
        std::getline(ss, imu.gyro_z, ',');

        // 将当前行数据添加到向量
        data.push_back(imu);
    }
    file.close();
    // 返回所读取到的原始imu的csv里的数据
    return data;
}

// 打开保存输出CSV文件
void saveNormCsv(const std::string& filename, const std::vector<ImuData>& data) {
    std::ofstream file(filename);
    // 检查成功打开与否
    if (!file.is_open()) {
        std::cerr << "cannot write the file: " << filename << std::endl;
        return;
    }

    // 输出表头
    file << "timestamp_sec,acc_x,acc_y,acc_z,gyro_x,gyro_y,gyro_z,imu_w_norm\n";

    for (const auto& imu : data) {
        // 计算模长
        double gx = std::stod(imu.gyro_x);
        double gy = std::stod(imu.gyro_y);
        double gz = std::stod(imu.gyro_z);
        double norm = std::sqrt(gx * gx + gy * gy + gz * gz);

        // 直接写原始时间戳和原始数据，不改格式
        file << imu.timestamp_sec << ","
             << imu.acc_x << "," << imu.acc_y << "," << imu.acc_z << ","
             << imu.gyro_x << "," << imu.gyro_y << "," << imu.gyro_z << ",";

        // 模长保留15位小数
        file << std::fixed << std::setprecision(15) << norm << "\n";
    }

    file.close();
    std::cout << "IMU data and angular velocity module length have been saved to " << filename << std::endl;
}

int main() {
    // 定义路径
    std::string imu_csv = "/home/cat/noetic_education_data/imu0_data.csv";
    std::string save_path = "/home/cat/foxy_education_data/imu0_data_w_norm.csv";

    // 读取IMU数据
    std::vector<ImuData> imu_data = readImuCsv(imu_csv);
    if (imu_data.empty()) {
        std::cerr << "IMU data is empty so exit" << std::endl;
        return -1;
    }

    // 保存模长结果
    saveNormCsv(save_path, imu_data);

    return 0;
}
