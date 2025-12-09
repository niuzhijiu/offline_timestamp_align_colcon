//声明头文件和依赖
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <iomanip>

//定义相机数据结构体
struct CamData {
    double timestamp;
    std::string filename;
};

//读取相机CSV
std::vector<CamData> readCamCsv(const std::string& filename) {
    std::vector<CamData> data;
    //打开文件并且检查成功与否
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot open the file: " << filename << std::endl;
        return data;
    }
    std::string line;
    //处理表头
    bool headerSkipped = false;
    while (std::getline(file, line)) {
        if (!headerSkipped) {
            headerSkipped = true; 
            continue;
        }
        //逗号分割字段
        std::stringstream ss(line);
        std::string token;
        CamData d;
        //读取第一个字段：时间戳
        std::getline(ss, token, ',');
        d.timestamp = std::stod(token);
        //读取第二个字段：文件名
        std::getline(ss, d.filename, ',');
        data.push_back(d);
    }
    return data;
}

//修正，然后保存修正后的 CSV
void saveAlignedCsv(const std::string& filename, const std::vector<CamData>& data, double bias) {
    //创建输出文件流，关联到目标保存路径并检查成功与否
    std::ofstream file(filename);
    if (!file.is_open()) {
        std::cerr << "cannot write the file: " << filename << std::endl;
        return;
    }
    //写表头
    file << "timestamp_sec,image_filename\n";
    //遍历所有相机数据，处理并写入文件
    for (auto& d : data) {
        file << std::fixed << std::setprecision(6) << (d.timestamp + bias) << "," << d.filename << "\n";
    }
    file.close();
}

int main() {
    //偏移量b
    double b = -0.007;  

    //输入输出文件路径
    std::string cam_csv = "/home/slam/20251205_test/cam0_timestamp.csv";
    std::string cam_aligned_csv = "/home/slam/20251208_test/cam0_aligned.csv";

    //读取CSV
    auto data = readCamCsv(cam_csv);
    if (data.empty()) {
        std::cerr << "CSV is empty so exit" << std::endl;
        return -1;
    }

    //保存结果
    saveAlignedCsv(cam_aligned_csv, data, b);

    std::cout << "camera timestamp has been shifted as a whole " 
              << std::fixed << std::setprecision(2) << b * 1000.0 
              << " ms，result has been saved to " << cam_aligned_csv << std::endl;

    return 0;
}
