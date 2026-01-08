// 声明头文件和依赖
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>
#include <algorithm>
#include <opencv2/opencv.hpp>

// CSV文件未必命名标准，可能会有多余的空格，需要先清洗。工具函数trim：清理字符串两端的空格、制表符、回车、引号。
static inline std::string trim(const std::string &s) {
    const std::string ws = " \t\n\r\"";
    size_t start = s.find_first_not_of(ws);
    if (start == std::string::npos) return "";
    size_t end = s.find_last_not_of(ws);
    return s.substr(start, end - start + 1);
}

// 图像数据集有的会给内部图像命名不标准。工具函数sanitize_filename：文件名清洗工具，提取有效的文件名
static inline std::string sanitize_filename(const std::string &raw) {
    std::string s = trim(raw);

    const std::vector<std::string> exts = {".png", ".jpg", ".jpeg", ".bmp", ".tiff"};
    size_t pos = std::string::npos;
    size_t best_ext_len = 0;
    std::string lower = s;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);

    for (const auto &ext : exts) {
        size_t p = lower.find(ext);
        if (p != std::string::npos) {
            if (pos == std::string::npos || p < pos) {
                pos = p;
                best_ext_len = ext.size();
            }
        }
    }
    if (pos != std::string::npos) {
       
        s = s.substr(0, pos + best_ext_len);
    }
    
    return trim(s);
}

// 相机CSV的数据结构，包括了时间戳和文件名
struct CamData {
    std::string timestamp_str; 
    double timestamp;         
    std::string filename_raw;  
    std::string filename;      
};

// csv读取函数readCamCsvRobust：去读取相机csv文件里面的信息
std::vector<CamData> readCamCsvRobust(const std::string &csv_path) {
    // 初始化用于存储最终解析结果的向量
    std::vector<CamData> out;
    // 打开文件
    std::ifstream ifs(csv_path);
    if (!ifs.is_open()) {
        std::cerr << "could not open file: " << csv_path << std::endl;
        return out;
    }
    // 用于存储读取的每一行内容
    std::string line;
    // 是否跳过表头状态
    bool headerSkipped = false;
    // 行计数器，记录当前行号
    size_t lineno = 0;
    // 循环读取文件的每一行
    while (std::getline(ifs, line)) {
        lineno++;
        // 跳过不处理表头
        if (!headerSkipped) { headerSkipped = true; continue; } 
        // 跳过空行
        if (line.empty()) continue;

        // 查找第一个逗号的位置，以此对时间戳进行分割
        size_t comma = line.find(',');
        // 格式错误的话，输出错误信息后跳过这一行
        if (comma == std::string::npos) {
            std::cerr << "a problem with the line" << lineno << " ，skip: " << line << std::endl;
            continue;
        }
        // 从0到逗号，时间戳
        std::string ts_token = line.substr(0, comma);
        // 从逗号到最后，文件名字
        std::string fname_token = line.substr(comma + 1);
        // 创建一个CamData结构体实例，存储当前行的解析结果
        CamData d;
        // 调用之前的trim函数修剪时间戳字符串的无效字符
        d.timestamp_str = trim(ts_token);
        // 字符串转数字，解析时间戳
        try {
            d.timestamp = std::stod(d.timestamp_str);
        } catch (...) {
            std::cerr << "the line " << lineno << " ： timestamp analysis failed: '" << d.timestamp_str << "'" << std::endl;
            continue;
        }
        // 存储原始文件名
        d.filename_raw = fname_token;
        // 清洗文件名
        d.filename = sanitize_filename(fname_token);
        // 将处理好的相机数据添加到结果向量保存
        out.push_back(d);
    }
    return out;
}

int main() {
    // 相机内参矩阵，需要根据相机实际参数设置。cx，cy通过查阅官方手册得知，fx，fy采用6mm时的euroc官方数据
    cv::Mat K = (cv::Mat_<double>(3,3) << 
        1000.0, 0.0, 376.0,
        0.0, 1000.0, 240.0,
        0.0, 0.0, 1.0);

    // 输入相机图像文件夹地址、时间戳地址、保存计算所的数据的地址
    std::string folder = "/home/cat/noetic_education_data/cam0/";
    std::string csv_path = "/home/cat/noetic_education_data/cam0_timestamp.csv";
    std::string save_path = "/home/cat/foxy_education_data/visual_angular_velocity_mid_cpp.csv";

    // 读取CSV数据并存储
    auto data = readCamCsvRobust(csv_path);
    // 检查有效性
    if (data.size() < 2) {
        std::cerr << "insufficient CSV data or parsing failure" << std::endl;
        return -1;
    }

    // 创建输出文件流
    std::ofstream fout(save_path);
    // 写入表头
    fout << "timestamp,wx,wy,wz,w_norm\n";

    // 遍历相邻的两帧图像数据
    int validCount = 0;
    for (size_t i = 0; i + 1 < data.size(); ++i) {
        double t1 = data[i].timestamp;
        double t2 = data[i+1].timestamp;
        double dt = t2 - t1;
        // 检查时序正确与否
        if (dt <= 0) {
            std::cout << "Timestamp exception, skip: " << data[i].timestamp_str << " -> " << data[i+1].timestamp_str << std::endl;
            continue;
        }

        // 容错处理文件名，保证能对应上
        std::string fname = data[i].filename.empty() ? trim(data[i].filename_raw) : data[i].filename;

        // 存储第一帧图像的完整路径
        std::string img1_path;
        // 是绝对路径的话就赋值，否则进行拼接得到路径
        if (!fname.empty() && fname.front() == '/') {
            img1_path = fname;
        } else {
            img1_path = folder + fname;
        }

        // 输出日志记录程序调试和跟踪程序运行状态
        std::cout << "\nProcessing pair " << i << ": raw_token='" << data[i].filename_raw
          << "' -> sanitized='" << fname << "'\n    Attempting to read: " << img1_path << std::endl;

        // 读取第一帧图像
        cv::Mat img1 = cv::imread(img1_path, cv::IMREAD_GRAYSCALE);
        // 检查第一帧图像是否读取成功
        if (img1.empty()) {
            std::cout << "Image1 reading failed: " << img1_path << std::endl;
            continue;
        }

        // 读取第二帧图像的有效文件名
        std::string fname2 = data[i+1].filename.empty() ? trim(data[i+1].filename_raw) : data[i+1].filename;
        // 得到第二帧图像的路径
        std::string img2_path = (fname2.size() && fname2.front() == '/') ? fname2 : folder + fname2;
        // 读取第二帧图像
        cv::Mat img2 = cv::imread(img2_path, cv::IMREAD_GRAYSCALE);
        // 检查第一帧图像是否读取成功
        if (img2.empty()) {
            std::cout << "Image2 reading failed: " << img2_path << std::endl;
            continue;
        }

    // ORB算法提取相邻两帧图像的特征点和特征描述符
        // 创建ORB特征提取器
        cv::Ptr<cv::ORB> orb = cv::ORB::create(2000);
        // 定义存储特征的数据结构：特征点kp，特征描述des
        std::vector<cv::KeyPoint> kp1, kp2;
        cv::Mat des1, des2;
        // 提取特征点和计算描述符
        orb->detectAndCompute(img1, cv::Mat(), kp1, des1);
        orb->detectAndCompute(img2, cv::Mat(), kp2, des2);

        // 检查特征提取结果
        if (des1.empty() || des2.empty()) {
            std::cout << "Insufficient feature points" << std::endl;
            continue;
        }

    // 用BFMatcher进行特征匹配
        // 创建BFMatcher匹配器
        cv::BFMatcher bf(cv::NORM_HAMMING, true);
        // 定义存储匹配结果的容器
        std::vector<cv::DMatch> matches;
        // 执行特征匹配
        bf.match(des1, des2, matches);
        // 检查匹配结果
        if (matches.size() < 5) {
            std::cout << "Insufficient matching points: " << matches.size() << std::endl;
            continue;
        }

    // 提取匹配点的像素坐标
        // 存储两帧中匹配点的像素坐标
        std::vector<cv::Point2f> pts1, pts2;
        // 遍历所有匹配对
        for (auto &m : matches) {
            pts1.push_back(kp1[m.queryIdx].pt);
            pts2.push_back(kp2[m.trainIdx].pt);
        }

    // 估计本质矩阵E
        // 存储RANSAC算法的内点掩码，标记哪些匹配点是有效内点
        cv::Mat mask;
        // 计算得到E
        cv::Mat E = cv::findEssentialMat(pts1, pts2, K, cv::RANSAC, 0.999, 1.0, mask);
        // 检查结果
        if (E.empty()) {
            std::cout << "E matrix calculation failed" << std::endl;
            continue;
        }
    // 检查内点数量是否足够
        // 统计内点数量
        int inliers = cv::countNonZero(mask);
        // 检查有效性
        if (inliers < 5) {
            std::cout << "Insufficient RANSAC inliers: " << inliers << std::endl;
            continue;
        }

    // 得到旋转矩阵R。从本质矩阵E中恢复相对旋转R和平移t，使用内点掩码mask过滤外点
        cv::Mat R, t;
        cv::recoverPose(E, pts1, pts2, K, R, t, mask);

    // 将旋转矩阵R转换为旋转向量rvec
        cv::Mat rvec;
        // 罗德里格斯公式将旋转矩阵R转换为旋转向量rvec
        cv::Rodrigues(R, rvec);
        cv::Vec3d rotvec;
        // 将旋转向量转换为cv::Vec3d类型并检测输出成功与否
        if (rvec.rows == 3 && rvec.cols == 1) {
            rotvec = cv::Vec3d(rvec.at<double>(0,0), rvec.at<double>(1,0), rvec.at<double>(2,0));
        } else {
            std::cout << "Rodrigues:Abnormal output format" << std::endl;
            continue;
        }

    // 计算角速度并输出结果
        cv::Vec3d omega = rotvec / dt;
        // 计算角速度的模值
        double omega_norm = cv::norm(omega);
        // 记两帧时间戳的中间时刻作为角速度的时间戳，属于常见做法
        double t_mid = 0.5 * (t1 + t2);

        // 写入输出文件：timestamp,wx,wy,wz,w_norm
        fout << std::fixed << std::setprecision(6) << t_mid << ","
             << std::setprecision(15) << omega[0] << "," << omega[1] << "," << omega[2] << ","
             << omega_norm << "\n";

        // 输出成功信息
        std::cout << "success: t=" << std::fixed << std::setprecision(6) << t_mid
                  << ", dt=" << dt
                  << ", w=(" << std::setprecision(15) << omega[0] << ", " << omega[1] << ", " << omega[2] << ")"
                  << ", |w|=" << omega_norm << std::endl;

        ++validCount;
    }

    fout.close();
    std::cout << "\nResults saved to " << save_path << "\ntotal write " << validCount << " angular velocity data" << std::endl;
    return 0;
}
