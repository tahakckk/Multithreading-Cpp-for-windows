#include <iostream>
#include <fstream>
#include <filesystem>
#include <random>

namespace fs = std::filesystem;

void generate_file(const std::string& path, size_t size) {
    std::ofstream file(path, std::ios::binary);
    if (!file) {
        std::cout << "[ERROR] Cannot create file: " << path << std::endl;
        return;
    }
    std::mt19937 rng(42); // Sabit seed, tekrar üretilebilirlik için
    std::uniform_int_distribution<unsigned char> dist(32, 126); // ASCII karakterler

    for (size_t i = 0; i < size; ++i) {
        char c = static_cast<char>(dist(rng));
        file.write(&c, 1);
    }
    std::cout << "[INFO] File created: " << path << " (" << size << " bytes)" << std::endl;
}

int main() {
    // Klasörleri oluştur
    fs::create_directories("input");
    fs::create_directories("output");

    // Dosya yolları ve boyutları
    std::string bigFile = "input/bigfile.txt";
    std::string smallFile = "input/smallfile.txt";
    size_t bigSize = 100 * 1024 * 1024;    // 100 MB
    size_t smallSize = 1024;              // 1 KB

    generate_file(bigFile, bigSize);
    generate_file(smallFile, smallSize);

    std::cout << "[INFO] All files generated successfully." << std::endl;
    return 0;
} 