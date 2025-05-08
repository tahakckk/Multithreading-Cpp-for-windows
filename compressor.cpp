#include "compressor.h"
#include <iostream>
#include <fstream>
#include <algorithm>
#include <thread>
#include <chrono>

Compressor::Compressor(const std::string& inputFile, const std::string& outputFile, size_t chunkSize)
    : inputFile_(inputFile), outputFile_(outputFile), chunkSize_(chunkSize), fileSize_(0), chunkCount_(0), progressBar_(nullptr) {}

std::vector<char> Compressor::runLengthEncode(const std::vector<char>& data) {
    std::vector<char> encoded;
    size_t n = data.size();
    for (size_t i = 0; i < n;) {
        char current = data[i];
        size_t run = 1;
        while (i + run < n && data[i + run] == current && run < 255) {
            ++run;
        }
        encoded.push_back(current);
        encoded.push_back(static_cast<char>(run));
        i += run;
    }
    return encoded;
}

void Compressor::compressChunk(size_t chunkIndex, const std::vector<char>& chunkData, std::vector<char>& compressedData) {
    compressedData = runLengthEncode(chunkData);
    // İlerleme çubuğunu güncelle
    if (progressBar_) {
        std::lock_guard<std::mutex> lock(writeMutex_);
        progressBar_->update(chunkIndex + 1);
    }
}

void Compressor::compress(bool multithread) {
    std::cout << "[INFO] Opening input file: " << inputFile_ << std::endl;
    std::ifstream inFile(inputFile_, std::ios::binary | std::ios::ate);
    if (!inFile) {
        std::cout << "[ERROR] Cannot open input file!\n";
        return;
    }
    fileSize_ = inFile.tellg();
    std::cout << "[INFO] Input file size: " << fileSize_ << " bytes" << std::endl;
    inFile.seekg(0, std::ios::beg);
    chunkCount_ = (fileSize_ + chunkSize_ - 1) / chunkSize_;
    std::cout << "[INFO] Chunk size: " << chunkSize_ << ", Chunk count: " << chunkCount_ << std::endl;
    if (chunkCount_ == 0) {
        std::cout << "[ERROR] Chunk count is zero! Nothing to compress.\n";
        return;
    }
    if (!multithread) {
        std::vector<char> inputData(fileSize_);
        inFile.read(inputData.data(), fileSize_);
        std::vector<char> compressed = runLengthEncode(inputData);
        std::ofstream outFile(outputFile_, std::ios::binary);
        if (!outFile) {
            std::cout << "[ERROR] Cannot open output file!\n";
            return;
        }
        outFile.write(compressed.data(), compressed.size());
        std::cout << "[INFO] Compression finished!\n";
        return;
    }
    // Multithread compress
    std::vector<std::vector<char>> compressedChunks(chunkCount_);
    std::vector<std::thread> threads(chunkCount_);
    std::vector<bool> chunkDone(chunkCount_, false);
    progressBar_ = new ProgressBar(chunkCount_);
    for (size_t i = 0; i < chunkCount_; ++i) {
        size_t thisChunkSize = std::min(chunkSize_, fileSize_ - i * chunkSize_);
        std::vector<char> chunkData(thisChunkSize);
        inFile.read(chunkData.data(), thisChunkSize);
        threads[i] = std::thread([this, i, chunkData, &compressedChunks, &chunkDone]() mutable {
            std::vector<char> compressed;
            this->compressChunk(i, chunkData, compressed);
            {
                std::lock_guard<std::mutex> lock(this->writeMutex_);
                compressedChunks[i] = std::move(compressed);
                chunkDone[i] = true;
            }
        });
    }
    // Ana thread ilerleme çubuğunu günceller
    size_t lastProgress = 0;
    while (lastProgress < chunkCount_) {
        size_t done = 0;
        {
            std::lock_guard<std::mutex> lock(writeMutex_);
            for (size_t i = 0; i < chunkCount_; ++i) {
                if (chunkDone[i]) ++done;
            }
        }
        if (done > lastProgress) {
            progressBar_->update(done);
            lastProgress = done;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    for (auto& t : threads) t.join();
    progressBar_->done();
    delete progressBar_;
    std::ofstream outFile(outputFile_, std::ios::binary);
    if (!outFile) {
        std::cout << "[ERROR] Cannot open output file!\n";
        return;
    }
    for (const auto& chunk : compressedChunks) {
        outFile.write(chunk.data(), chunk.size());
    }
    std::cout << "[INFO] Compression finished!\n";
}

std::vector<char> Compressor::runLengthDecode(const std::vector<char>& data) {
    std::vector<char> decoded;
    size_t n = data.size();
    for (size_t i = 0; i + 1 < n; i += 2) {
        char value = data[i];
        unsigned char count = static_cast<unsigned char>(data[i + 1]);
        decoded.insert(decoded.end(), count, value);
    }
    return decoded;
}

void Compressor::decompressChunk(size_t chunkIndex, const std::vector<char>& chunkData, std::vector<char>& decompressedData) {
    decompressedData = runLengthDecode(chunkData);
}

void Compressor::decompress(bool multithread) {
    std::cout << "[INFO] Opening compressed file: " << inputFile_ << std::endl;
    std::ifstream inFile(inputFile_, std::ios::binary | std::ios::ate);
    if (!inFile) {
        std::cout << "[ERROR] Cannot open input file!\n";
        return;
    }
    fileSize_ = inFile.tellg();
    std::cout << "[INFO] Compressed file size: " << fileSize_ << " bytes" << std::endl;
    inFile.seekg(0, std::ios::beg);
    // Chunk sayısı tahmini: Sıkıştırılmış dosyada chunk sınırları bilinmiyor, bu nedenle decompress işlemi için ek meta veri gerekir.
    // Şimdilik tüm dosyayı tek chunk olarak açıyoruz (singlethread) veya eşit parçalara bölüp decompress ediyoruz (multithread, veri bozulabilir!)
    if (!multithread) {
        std::vector<char> compressedData(fileSize_);
        inFile.read(compressedData.data(), fileSize_);
        std::vector<char> decompressed = runLengthDecode(compressedData);
        std::ofstream outFile(outputFile_, std::ios::binary);
        if (!outFile) {
            std::cout << "[ERROR] Cannot open output file!\n";
            return;
        }
        outFile.write(decompressed.data(), decompressed.size());
        std::cout << "[INFO] Decompression finished!\n";
        return;
    }
    // Multithread decompress için: Dosyayı chunkSize kadar parçalara bölüp her birini ayrı thread ile açıyoruz
    chunkCount_ = (fileSize_ + chunkSize_ - 1) / chunkSize_;
    std::vector<std::vector<char>> decompressedChunks(chunkCount_);
    std::vector<std::thread> threads(chunkCount_);
    std::vector<bool> chunkDone(chunkCount_, false);
    progressBar_ = new ProgressBar(chunkCount_);
    for (size_t i = 0; i < chunkCount_; ++i) {
        size_t thisChunkSize = std::min(chunkSize_, fileSize_ - i * chunkSize_);
        std::vector<char> chunkData(thisChunkSize);
        inFile.read(chunkData.data(), thisChunkSize);
        threads[i] = std::thread([this, i, chunkData, &decompressedChunks, &chunkDone]() mutable {
            std::vector<char> decompressed;
            this->decompressChunk(i, chunkData, decompressed);
            {
                std::lock_guard<std::mutex> lock(this->writeMutex_);
                decompressedChunks[i] = std::move(decompressed);
                chunkDone[i] = true;
            }
        });
    }
    // Ana thread ilerleme çubuğunu günceller
    size_t lastProgress = 0;
    while (lastProgress < chunkCount_) {
        size_t done = 0;
        {
            std::lock_guard<std::mutex> lock(writeMutex_);
            for (size_t i = 0; i < chunkCount_; ++i) {
                if (chunkDone[i]) ++done;
            }
        }
        if (done > lastProgress) {
            progressBar_->update(done);
            lastProgress = done;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
    for (auto& t : threads) t.join();
    progressBar_->done();
    delete progressBar_;
    std::ofstream outFile(outputFile_, std::ios::binary);
    if (!outFile) {
        std::cout << "[ERROR] Cannot open output file!\n";
        return;
    }
    for (const auto& chunk : decompressedChunks) {
        outFile.write(chunk.data(), chunk.size());
    }
    std::cout << "[INFO] Decompression finished!\n";
}

double Compressor::benchmark(void (Compressor::*func)(bool), Compressor& obj, bool multithread) {
    auto start = std::chrono::high_resolution_clock::now();
    (obj.*func)(multithread);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    return diff.count();
}

#include <cstdio>
bool Compressor::compareFiles(const std::string& file1, const std::string& file2) {
    std::ifstream f1(file1, std::ios::binary);
    std::ifstream f2(file2, std::ios::binary);
    
    if (!f1) {
        std::cout << "[ERROR] Cannot open first file for comparison: " << file1 << std::endl;
        return false;
    }
    if (!f2) {
        std::cout << "[ERROR] Cannot open second file for comparison: " << file2 << std::endl;
        return false;
    }
    
    // Dosya boyutlarını kontrol et
    f1.seekg(0, std::ios::end);
    f2.seekg(0, std::ios::end);
    size_t size1 = f1.tellg();
    size_t size2 = f2.tellg();
    
    if (size1 != size2) {
        std::cout << "[ERROR] Files have different sizes: " << size1 << " vs " << size2 << std::endl;
        return false;
    }
    
    // Dosyaları başa sar
    f1.seekg(0, std::ios::beg);
    f2.seekg(0, std::ios::beg);
    
    // İçerikleri karşılaştır
    std::istreambuf_iterator<char> begin1(f1), begin2(f2), end;
    return std::equal(begin1, end, begin2);
} 