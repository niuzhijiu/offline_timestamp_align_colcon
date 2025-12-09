#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cmath>
#include <iomanip>

//IMU数据结构
struct ImuData {
    double t;               //时间戳
    double w_norm;          //角速度模值
};

//视觉数据结构
struct VisData {
    double t;               //时间戳
    double w_norm;          //角速度模值
};

//读取imu的csv文件
std::vector<ImuData> readImuCsv(const std::string& filename) {
    std::vector<ImuData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open IMU file: " << filename << std::endl;
        return data;
    }

    std::string line;
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        if (!headerSkipped) {
            headerSkipped = true;  //跳过表头
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        ImuData d;

        //读取时间戳
        std::getline(ss, token, ',');
        d.t = std::stod(token);

        //跳过无关字段
        for (int i = 0; i < 6; ++i) {
            std::getline(ss, token, ',');
        }

        //读取IMU的角速度模值
        std::getline(ss, token, ',');
        d.w_norm = std::stod(token);

        data.push_back(d);
    }

    return data;
}

//读取视觉的CSV文件
std::vector<VisData> readVisCsv(const std::string& filename) {
    std::vector<VisData> data;
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open visual file: " << filename << std::endl;
        return data;
    }

    std::string line;
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        if (!headerSkipped) {
            headerSkipped = true;  //跳过表头
            continue;
        }

        std::stringstream ss(line);
        std::string token;
        VisData d;

        //读取时间戳
        std::getline(ss, token, ',');
        d.t = std::stod(token);

        //跳过无关分量
        for (int i = 0; i < 3; ++i) {
            std::getline(ss, token, ',');
        }

        //读取视觉的角速度模值
        std::getline(ss, token, ',');
        d.w_norm = std::stod(token);

        data.push_back(d);
    }

    return data;
}

//线性插值视觉数据到imu数据序列
std::vector<double> interpolate(const std::vector<VisData>& vis_data,
                                const std::vector<double>& imu_timestamps,
                                double bias) {
    std::vector<double> interpolated;
    interpolated.reserve(imu_timestamps.size());  //预分配内存提升效率

    for (double imu_t : imu_timestamps) {
        //计算偏移后的视觉时间戳
        double vis_t_shifted = imu_t - bias;

        //找到视觉数据中对应shifted_t的位置
        size_t idx = 0;
        while (idx + 1 < vis_data.size() && vis_data[idx + 1].t < vis_t_shifted) {
            ++idx;
        }

        //处理边界情况，超出视觉数据范围时用最后一个值
        if (idx + 1 >= vis_data.size()) {
            interpolated.push_back(vis_data.back().w_norm);
            continue;
        }

        //线性插值计算
        const VisData& v_prev = vis_data[idx];
        const VisData& v_next = vis_data[idx + 1];
        double t_prev = v_prev.t;
        double t_next = v_next.t;
        double norm_prev = v_prev.w_norm;
        double norm_next = v_next.w_norm;

        //计算插值权重
        double alpha = (vis_t_shifted - t_prev) / (t_next - t_prev);
        interpolated.push_back(norm_prev + alpha * (norm_next - norm_prev));
    }

    return interpolated;
}

//计算两个序列的归一化互相关系数
double computeCorrelation(const std::vector<double>& imu_norms,
                          const std::vector<double>& vis_norms_interp) {
    //计算两个序列的均值
    double mean_imu = 0.0, mean_vis = 0.0;
    for (double val : imu_norms) mean_imu += val;
    for (double val : vis_norms_interp) mean_vis += val;
    mean_imu /= imu_norms.size();
    mean_vis /= vis_norms_interp.size();

    //计算分子（协方差和）和分母（各自方差和的乘积开方）
    double numerator = 0.0;
    double denom_imu = 0.0, denom_vis = 0.0;
    for (size_t i = 0; i < imu_norms.size(); ++i) {
        double diff_imu = imu_norms[i] - mean_imu;
        double diff_vis = vis_norms_interp[i] - mean_vis;
        numerator += diff_imu * diff_vis;
        denom_imu += diff_imu * diff_imu;
        denom_vis += diff_vis * diff_vis;
    }

    //避免分母为0
    if (denom_imu <= 0 || denom_vis <= 0) {
        return 0.0;
    }

    return numerator / std::sqrt(denom_imu * denom_vis);
}

int main() {
    //数据文件路径
    std::string imu_csv_path = "/home/slam/20251205_test/imu0_data_w_norm.csv";
    std::string vis_csv_path = "/home/slam/20251205_test/visual_angular_velocity_mid_cpp.csv";

    //读取IMU和视觉数据
    std::vector<ImuData> imu_data = readImuCsv(imu_csv_path);
    std::vector<VisData> vis_data = readVisCsv(vis_csv_path);

    //检查数据是否读取成功
    if (imu_data.empty()) {
        std::cerr << "IMU data is empty or file read failed" << std::endl;
        return -1;
    }
    if (vis_data.empty()) {
        std::cerr << "visual data is empty or file read failed" << std::endl;
        return -1;
    }

    //提取IMU的时间戳和模值序列
    std::vector<double> imu_timestamps;
    std::vector<double> imu_norms;
    for (const ImuData& d : imu_data) {
        imu_timestamps.push_back(d.t);
        imu_norms.push_back(d.w_norm);
    }

    //搜索最佳时间偏移，范围-25ms至+25ms，步长1ms
    double best_bias = 0.0;         //最佳偏移量初始值为0
    double best_correlation = -1.0; //最佳相关系数初始值为-1

    //遍历所有可能的偏移量
    for (double bias = -0.025; bias <= 0.025; bias += 0.001) {
        //将视觉模值插值到IMU时间戳
        std::vector<double> vis_interp = interpolate(vis_data, imu_timestamps, bias);

        //计算相关性
        double corr = computeCorrelation(imu_norms, vis_interp);

        //更新最佳结果
        if (corr > best_correlation) {
            best_correlation = corr;
            best_bias = bias;
        }
    }

    //输出结果
    std::cout << "=== result ===" << std::endl;
    std::cout << "best time offset:" << std::fixed << std::setprecision(3)
              << best_bias * 1000 << " ms" << std::endl;
    std::cout << "maximum correlation coefficient:" << std::fixed << std::setprecision(6)
              << best_correlation << std::endl;

    return 0;
}
