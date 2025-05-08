#pragma once
#include <string>
#include <vector>
#include <thread>
#include <mutex>
#include "progress_bar.h"
#include <chrono>

class Compressor {
public:
    Compressor(const std::string& inputFile, const std::string& outputFile, size_t chunkSize);
    void compress(bool multithread = true);
    void decompress(bool multithread = true);
    // Benchmark ve karşılaştırma fonksiyonları
    static double benchmark(void (Compressor::*func)(bool), Compressor& obj, bool multithread);
    static bool compareFiles(const std::string& file1, const std::string& file2);
private:
    std::string inputFile_;
    std::string outputFile_;
    size_t chunkSize_;
    size_t fileSize_;
    size_t chunkCount_;
    std::mutex writeMutex_;
    ProgressBar* progressBar_;
    void compressChunk(size_t chunkIndex, const std::vector<char>& chunkData, std::vector<char>& compressedData);
    void decompressChunk(size_t chunkIndex, const std::vector<char>& chunkData, std::vector<char>& decompressedData);
    std::vector<char> runLengthEncode(const std::vector<char>& data);
    std::vector<char> runLengthDecode(const std::vector<char>& data);
}; 