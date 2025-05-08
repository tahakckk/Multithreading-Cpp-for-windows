#include <iostream>
#include <fstream>
#include <filesystem>
#include <string>
#include <vector>

void createTestFiles() {
    std::cout << "Creating test files for error testing...\n";
    
    // Test klasörlerini oluştur
    std::filesystem::create_directories("test_input");
    std::filesystem::create_directories("test_output");
    std::filesystem::create_directories("test_output/readonly");
    
    // Boş dosya
    std::ofstream emptyFile("test_input/empty.txt");
    emptyFile.close();
    
    // Çok küçük dosya (1 byte)
    std::ofstream tinyFile("test_input/tiny.txt");
    tinyFile << "A";
    tinyFile.close();
    
    // Çok büyük dosya (1GB)
    std::ofstream hugeFile("test_input/huge.txt", std::ios::binary);
    const int bufferSize = 1024 * 1024; // 1MB buffer
    std::vector<char> buffer(bufferSize, 'X');
    for (int i = 0; i < 1024; i++) { // 1024 * 1MB = 1GB
        hugeFile.write(buffer.data(), bufferSize);
    }
    hugeFile.close();
    
    // Geçersiz karakterler içeren dosya
    std::ofstream invalidFile("test_input/invalid.txt", std::ios::binary);
    for (int i = 0; i < 1000; i++) {
        invalidFile << static_cast<char>(i % 256);
    }
    invalidFile.close();
    
    // Sadece okunabilir dosya
    std::ofstream readonlyFile("test_output/readonly/test.txt");
    readonlyFile << "This is a read-only test file";
    readonlyFile.close();
    
    // Windows'ta dosyayı salt okunur yap
    #ifdef _WIN32
    system("attrib +r test_output\\readonly\\test.txt");
    #endif
    
    std::cout << "Test files created successfully!\n";
}

int main() {
    createTestFiles();
    return 0;
} 