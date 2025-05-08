#include <iostream>
#include <cstdlib>
#include <string>
#include <filesystem>
#include <fstream>
#include <windows.h>
#include <chrono>
#include <iomanip>
#include "compressor.h"
#include "progress_bar.h"

// Minimum ve maksimum chunk size değerleri (bytes)
constexpr size_t MIN_CHUNK_SIZE = 1024;        // 1KB
constexpr size_t MAX_CHUNK_SIZE = 1024*1024*1024; // 1GB

void printUsage(const char* progName) {
    std::cout << "Usage: " << progName << " <compress|decompress> <single|multi> <input_file> <output_file> <chunk_size_in_bytes>\n";
    std::cout << "Example: " << progName << " compress multi input/bigfile.txt output/bigfile.compressed 1048576\n";
    std::cout << "\nChunk size limits:\n";
    std::cout << "  Minimum: " << MIN_CHUNK_SIZE << " bytes (1KB)\n";
    std::cout << "  Maximum: " << MAX_CHUNK_SIZE << " bytes (1GB)\n";
}

std::string ensure_output_dir(const std::string& fileName) {
    try {
        std::filesystem::create_directories("output");
        return "output/" + fileName;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to create output directory: " << e.what() << std::endl;
        throw;
    }
}

std::string ensure_decompress_dir(const std::string& fileName) {
    try {
        std::filesystem::create_directories("output/decompress");
        return "output/decompress/" + fileName;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] Failed to create decompress directory: " << e.what() << std::endl;
        throw;
    }
}

bool checkFilePermissions(const std::string& filePath, bool forWriting = false) {
    try {
        if (!std::filesystem::exists(filePath)) {
            return false;
        }

        // Windows'ta dosya izinlerini kontrol et
        DWORD access = forWriting ? GENERIC_WRITE : GENERIC_READ;
        HANDLE hFile = CreateFileA(
            filePath.c_str(),
            access,
            FILE_SHARE_READ,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

        if (hFile == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_ACCESS_DENIED) {
                std::cerr << "[ERROR] Access denied to file: " << filePath << std::endl;
            } else {
                std::cerr << "[ERROR] Failed to open file: " << filePath << " (Error: " << error << ")" << std::endl;
            }
            return false;
        }

        CloseHandle(hFile);
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] File permission check failed: " << e.what() << std::endl;
        return false;
    }
}

bool testFileAccess(const std::string& filePath, bool forWriting = false) {
    try {
        if (forWriting) {
            std::ofstream testFile(filePath, std::ios::app);
            if (!testFile.is_open()) {
                std::cerr << "[ERROR] Cannot write to file: " << filePath << std::endl;
                return false;
            }
            testFile.close();
        } else {
            std::ifstream testFile(filePath);
            if (!testFile.is_open()) {
                std::cerr << "[ERROR] Cannot read file: " << filePath << std::endl;
                return false;
            }
            testFile.close();
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "[ERROR] File access test failed: " << e.what() << std::endl;
        return false;
    }
}

std::string getFileName(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

void printTestResults(const std::string& fileName, 
                     double multiCompressTime, double multiDecompressTime, bool multiMatch,
                     double singleCompressTime, double singleDecompressTime, bool singleMatch) {
    std::cout << "\nTest Results (for " << fileName << "):\n";
    std::cout << "┌─────────────────┬───────────────┬───────────────┬───────────────┐\n";
    std::cout << "│ Mode           │ Compression   │ Decompression │ Comparison    │\n";
    std::cout << "├─────────────────┼───────────────┼───────────────┼───────────────┤\n";
    std::cout << "│ Multi-thread   │ " << std::fixed << std::setprecision(6) << std::setw(11) << multiCompressTime << "s │ "
              << std::setw(11) << multiDecompressTime << "s │ " << (multiMatch ? "✅ MATCH" : "❌ NO MATCH") << " │\n";
    std::cout << "│ Single-thread  │ " << std::setw(11) << singleCompressTime << "s │ "
              << std::setw(11) << singleDecompressTime << "s │ " << (singleMatch ? "✅ MATCH" : "❌ NO MATCH") << " │\n";
    std::cout << "└─────────────────┴───────────────┴───────────────┴───────────────┘\n";
    
    // Performance comparison
    double compressRatio = multiCompressTime / singleCompressTime;
    double decompressRatio = multiDecompressTime / singleDecompressTime;
    
    std::cout << "\nPerformance Comparison:\n";
    if (compressRatio > 1.0) {
        std::cout << "├─ Single-thread compression is " << std::fixed << std::setprecision(2) << compressRatio << "x faster than multi-thread\n";
    } else {
        std::cout << "├─ Multi-thread compression is " << std::fixed << std::setprecision(2) << (1.0/compressRatio) << "x faster than single-thread\n";
    }
    
    if (decompressRatio > 1.0) {
        std::cout << "└─ Single-thread decompression is " << std::fixed << std::setprecision(2) << decompressRatio << "x faster than multi-thread\n";
    } else {
        std::cout << "└─ Multi-thread decompression is " << std::fixed << std::setprecision(2) << (1.0/decompressRatio) << "x faster than single-thread\n";
    }
}

int main(int argc, char* argv[]) {
    // Windows console UTF-8 support
    SetConsoleOutputCP(CP_UTF8);
    
    if (argc == 2 && std::string(argv[1]) == "test") {
        // Test mode
        std::cout << "Starting test mode...\n\n";
        
        // Create output directories
        try {
            std::filesystem::create_directories("output");
            std::filesystem::create_directories("output/decompress");
        } catch (const std::exception& e) {
            std::cout << "[ERROR] Failed to create output directories: " << e.what() << std::endl;
            return 1;
        }
        
        // Test files
        std::vector<std::string> testFiles = {
            "input/smallfile.txt",
            "input/bigfile.txt"
        };
        
        for (const auto& testFile : testFiles) {
            std::cout << "\n=== Starting tests for " << testFile << " ===\n";
            
            // Multi-thread test
            std::string multiCompressed = "output/" + getFileName(testFile) + ".compressed";
            std::string multiDecompressed = "output/decompress/" + getFileName(testFile) + ".decompressed.txt";
            
            Compressor multiCompressor(testFile, multiCompressed, 1048576);
            
            auto multiStart = std::chrono::high_resolution_clock::now();
            multiCompressor.compress(true);
            auto multiCompressEnd = std::chrono::high_resolution_clock::now();
            
            Compressor multiDecompressor(multiCompressed, multiDecompressed, 1048576);
            multiDecompressor.decompress(true);
            auto multiDecompressEnd = std::chrono::high_resolution_clock::now();
            
            // Single-thread test
            std::string singleCompressed = "output/" + getFileName(testFile) + ".single.compressed";
            std::string singleDecompressed = "output/decompress/" + getFileName(testFile) + ".single.decompressed.txt";
            
            Compressor singleCompressor(testFile, singleCompressed, 1048576);
            
            auto singleStart = std::chrono::high_resolution_clock::now();
            singleCompressor.compress(false);
            auto singleCompressEnd = std::chrono::high_resolution_clock::now();
            
            Compressor singleDecompressor(singleCompressed, singleDecompressed, 1048576);
            singleDecompressor.decompress(false);
            auto singleDecompressEnd = std::chrono::high_resolution_clock::now();
            
            // Time calculations
            double multiCompressTime = std::chrono::duration<double>(multiCompressEnd - multiStart).count();
            double multiDecompressTime = std::chrono::duration<double>(multiDecompressEnd - multiCompressEnd).count();
            double singleCompressTime = std::chrono::duration<double>(singleCompressEnd - singleStart).count();
            double singleDecompressTime = std::chrono::duration<double>(singleDecompressEnd - singleCompressEnd).count();
            
            // Comparison results
            bool multiMatch = Compressor::compareFiles(testFile, multiDecompressed);
            bool singleMatch = Compressor::compareFiles(testFile, singleDecompressed);
            
            // Print results
            printTestResults(getFileName(testFile), 
                           multiCompressTime, multiDecompressTime, multiMatch,
                           singleCompressTime, singleDecompressTime, singleMatch);
        }
        
        return 0;
    }
    
    std::cout << "Multithreaded File Compression Utility\n";
    std::cout << "-------------------------------------\n";

    if (argc != 6) {
        std::cout << "[ERROR] Invalid number of arguments!\n";
        printUsage(argv[0]);
        return 1;
    }

    std::string operation = argv[1];
    std::string mode = argv[2];
    std::string inputFile = argv[3];
    std::string outputFile = argv[4];
    size_t chunkSize = std::strtoull(argv[5], nullptr, 10);

    // Operation check
    if (operation != "compress" && operation != "decompress") {
        std::cout << "[ERROR] Invalid operation! Use 'compress' or 'decompress'.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Mode check
    if (mode != "single" && mode != "multi") {
        std::cout << "[ERROR] Invalid mode! Use 'single' or 'multi'.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Chunk size check
    if (chunkSize < MIN_CHUNK_SIZE || chunkSize > MAX_CHUNK_SIZE) {
        std::cout << "[ERROR] Invalid chunk size! Must be between " 
                  << MIN_CHUNK_SIZE << " and " << MAX_CHUNK_SIZE << " bytes.\n";
        printUsage(argv[0]);
        return 1;
    }

    // Input file check
    if (!std::filesystem::exists(inputFile)) {
        std::cout << "[ERROR] Input file does not exist: " << inputFile << std::endl;
        return 1;
    }

    // Input file permissions check
    if (!checkFilePermissions(inputFile, false) || !testFileAccess(inputFile, false)) {
        std::cout << "[ERROR] Cannot access input file: " << inputFile << std::endl;
        return 1;
    }

    bool multithread = (mode == "multi");

    // Normalize filename
    size_t lastSlash = outputFile.find_last_of("/\\");
    std::string fileName = (lastSlash != std::string::npos) ? outputFile.substr(lastSlash + 1) : outputFile;

    try {
        // Determine output file path
        if (operation == "compress") {
            outputFile = ensure_output_dir(fileName);
        } else if (operation == "decompress") {
            outputFile = ensure_decompress_dir(fileName);
        }

        // Output file write permission check
        if (!testFileAccess(outputFile, true)) {
            std::cout << "[ERROR] Cannot write to output file: " << outputFile << std::endl;
            return 1;
        }

        Compressor compressor(inputFile, outputFile, chunkSize);

        // Benchmark and comparison
        double elapsed = 0.0;
        if (operation == "compress") {
            std::cout << "[INFO] Compression mode: " << (multithread ? "Multithreaded" : "Singlethreaded") << std::endl;
            std::cout << "[INFO] Output file: " << outputFile << std::endl;
            elapsed = Compressor::benchmark(&Compressor::compress, compressor, multithread);
            std::cout << "[REPORT] Compression time: " << elapsed << " seconds\n";
        } else if (operation == "decompress") {
            std::cout << "[INFO] Decompression mode: " << (multithread ? "Multithreaded" : "Singlethreaded") << std::endl;
            std::cout << "[INFO] Output file: " << outputFile << std::endl;
            elapsed = Compressor::benchmark(&Compressor::decompress, compressor, multithread);
            std::cout << "[REPORT] Decompression time: " << elapsed << " seconds\n";
        }

        // Compare decompressed files with each other and input
        if (operation == "decompress") {
            size_t pos = fileName.find(".decompressed");
            std::string baseName = (pos != std::string::npos) ? fileName.substr(0, pos) : fileName;
            std::string inputRef = "input/" + baseName + ".txt";
            std::string multiDecomp = "output/decompress/" + baseName + ".decompressed.txt";
            std::string singleDecomp = "output/decompress/" + baseName + ".decompressed.single.txt";
            
            // Compare with each other
            if (std::filesystem::exists(multiDecomp) && std::filesystem::exists(singleDecomp)) {
                bool same = Compressor::compareFiles(multiDecomp, singleDecomp);
                std::cout << "[REPORT] Multi vs Single decompress: " << (same ? "MATCH" : "DIFFER") << std::endl;
            }
            
            // Compare with original input
            if (std::filesystem::exists(inputRef)) {
                bool same = Compressor::compareFiles(inputRef, outputFile);
                std::cout << "[REPORT] Decompressed file vs input: " << (same ? "MATCH" : "DIFFER") << std::endl;
            } else {
                std::cout << "[WARN] Reference file for comparison not found: " << inputRef << std::endl;
            }
        }
    } catch (const std::bad_alloc& e) {
        std::cout << "[ERROR] Memory allocation failed: " << e.what() << std::endl;
        std::cout << "[INFO] Try using a smaller chunk size or process the file in smaller parts.\n";
        return 1;
    } catch (const std::exception& e) {
        std::cout << "[ERROR] An error occurred: " << e.what() << std::endl;
        return 1;
    }

    return 0;
} 