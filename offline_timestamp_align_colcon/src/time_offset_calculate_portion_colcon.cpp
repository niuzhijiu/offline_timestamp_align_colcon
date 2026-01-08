// 声明头文件和依赖
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <limits>
#include <iomanip>

// 定义要读取的所需IMU数据
struct ImuData {
    double t;
    double gx, gy, gz;
};

// 定义要读取的所需视觉数据
struct VisData {
    double t;
    double wx, wy, wz;
};

// 读取IMU的CSV文件
std::vector<ImuData> readImuCsv(const std::string& filename) {
    std::vector<ImuData> data;
    // 打开文件并检查成功与否
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open IMU file: " << filename << std::endl;
        return data;
    }
    // 存储每行内容
    std::string line;
    // 表头标志
    bool headerSkipped = false;
    // 跳过表头
    while (std::getline(file, line)) {
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        // 以逗号进行分割
        std::stringstream ss(line);
        std::string token;
        ImuData d;
        // 读timestamp_sec
        std::getline(ss, token, ','); d.t = std::stod(token);
        // acc_x, acc_y, acc_z不需要
        std::getline(ss, token, ',');
        std::getline(ss, token, ',');
        std::getline(ss, token, ',');
        // gyro_x, gyro_y, gyro_z
        std::getline(ss, token, ','); d.gx = std::stod(token);
        std::getline(ss, token, ','); d.gy = std::stod(token);
        std::getline(ss, token, ','); d.gz = std::stod(token);
        // 将解析后的IMU数据存入vector
        data.push_back(d);
    }
    return data;
}

// 读取视觉的CSV
std::vector<VisData> readVisCsv(const std::string& filename) {
    std::vector<VisData> data;
    // 打开文件并检查成功与否
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open VISUAL file: " << filename << std::endl;
        return data;
    }
    std::string line;
    // 表头标志
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        if (!headerSkipped) {
            headerSkipped = true;
            continue;
        }
        // 逗号分割
        std::stringstream ss(line);
        std::string token;
        VisData d;
        // 读取时间戳和角速度分量，依次为t, wx, wy, wz
        std::getline(ss, token, ','); d.t = std::stod(token);
        std::getline(ss, token, ','); d.wx = std::stod(token);
        std::getline(ss, token, ','); d.wy = std::stod(token);
        std::getline(ss, token, ','); d.wz = std::stod(token);
        data.push_back(d);
    }
    return data;
}

// 线性插值视觉数据到IMU序列
std::vector<double> interpolate(const std::vector<VisData>& vis,
                                const std::vector<double>& imu_t,
                                const std::vector<double>& vis_s,
                                double bias) {
    // 插值后的视觉数据
    std::vector<double> vis_interp;
    // 预分配内存去优化效率
    vis_interp.reserve(imu_t.size());

    // 遍历每个IMU时间戳
    for (double t : imu_t) {
        double shifted_t = t - bias;                                    // 平移视觉时间戳进行补偿bias
        size_t j = 0;                                                   // 找到shifted_t在视觉数据中的位置
        while (j + 1 < vis.size() && vis[j+1].t < shifted_t) j++;
        if (j + 1 >= vis.size()) {
            vis_interp.push_back(vis.back().wx);                        // 如果越界则用最后一个填充
            continue;
        }
        // 线性插值，根据时间差计算比例
        double t1 = vis[j].t, t2 = vis[j+1].t;
        double v1 = vis_s[j], v2 = vis_s[j+1];
        double alpha = (shifted_t - t1) / (t2 - t1);
        // 插值结果
        vis_interp.push_back(v1 + alpha * (v2 - v1));
    }
    return vis_interp;
}

// 归一化互相关的计算
double computeCorrelation(const std::vector<double>& imu_s,
                          const std::vector<double>& vis_s) {
    // 计算IMU和视觉数据的平均值
    double mean_imu = 0, mean_vis = 0;
    for (auto v : imu_s) mean_imu += v;
    for (auto v : vis_s) mean_vis += v;
    mean_imu /= imu_s.size();
    mean_vis /= vis_s.size();

    // 分子：协方差之和      分母：各自方差之和的乘积开方
    double num = 0, denom1 = 0, denom2 = 0;
    for (size_t i = 0; i < imu_s.size(); i++) {
        double a = imu_s[i] - mean_imu;
        double b = vis_s[i] - mean_vis;
        num += a * b;
        denom1 += a * a;
        denom2 += b * b;
    }
    // 防止除数是0
    if (denom1 <= 0 || denom2 <= 0) return 0;
    // 计算
    return num / std::sqrt(denom1 * denom2);
}

int main() {
    // 定义路径
    std::string imu_csv = "/home/cat/noetic_education_data/imu0_data.csv";
    std::string vis_csv = "/home/cat/foxy_education_data/visual_angular_velocity_mid_cpp.csv";

    // 读取IMU和视觉数据并检查是否为空
    auto imu = readImuCsv(imu_csv);
    auto vis = readVisCsv(vis_csv);
    if (imu.empty() || vis.empty()) {
        std::cerr << "input data is empty" << std::endl;
        return -1;
    }

    // 提取imu时间戳角速度
    std::vector<double> imu_t, imu_x, imu_y, imu_z;
    for (auto& d : imu) {
        imu_t.push_back(d.t);
        imu_x.push_back(d.gx);
        imu_y.push_back(d.gy);
        imu_z.push_back(d.gz);
    }

    // 提取视觉的分量
    std::vector<double> vis_x, vis_y, vis_z;
    for (auto& d : vis) {
        vis_x.push_back(d.wx);
        vis_y.push_back(d.wy);
        vis_z.push_back(d.wz);
    }

    // 当前的全局最佳bias和相关系数
    double best_bias = 0, best_rho = -1;
    // 最佳结果对应的轴
    std::string best_axis;
    // 分别对x、y、z轴计算最佳偏移
    for (std::string axis : {"x", "y", "z"}) {
        std::vector<double> imu_s, vis_s;
        if (axis == "x") { imu_s = imu_x; vis_s = vis_x; }
        if (axis == "y") { imu_s = imu_y; vis_s = vis_y; }
        if (axis == "z") { imu_s = imu_z; vis_s = vis_z; }

        // 搜索当前轴的最佳偏移，范围是[-25,+25]ms,步长是0.5ms
        double best_rho_axis = -1, best_bias_axis = 0;
        for (double bias = -0.025; bias <= 0.025; bias += 0.0005) {
            // 视觉插值到imu
            auto vis_interp = interpolate(vis, imu_t, vis_s, bias);
            // 相关性计算
            double rho = computeCorrelation(imu_s, vis_interp);
            // 更新
            if (rho > best_rho_axis) {
                best_rho_axis = rho;
                best_bias_axis = bias;
            }
        }
        // 输出结果
        std::cout << "axis " << axis << ": best b̂ = "
                  << best_bias_axis * 1000.0 << " ms " << std::endl;

        if (best_rho_axis > best_rho) {
            best_rho = best_rho_axis;
            best_bias = best_bias_axis;
            best_axis = axis;
        }
    }

    std::cout << "\n>>> best result is from axis " << best_axis
              << ": bias = " << std::fixed << std::setprecision(15)
              << best_bias * 1000.0 << " ms " << std::endl;

    return 0;
}
