#pragma once
#include <iostream>

class ProgressBar {
public:
    ProgressBar(size_t total, size_t width = 50)
        : total_(total), width_(width), progress_(0) {}

    void update(size_t value) {
        progress_ = value;
        display();
    }

    void display() const {
        float ratio = static_cast<float>(progress_) / total_;
        size_t c = static_cast<size_t>(ratio * width_);
        std::cout << "[";
        for (size_t x = 0; x < c; ++x) std::cout << "=";
        for (size_t x = c; x < width_; ++x) std::cout << " ";
        std::cout << "] " << int(ratio * 100.0) << "%\r";
        std::cout.flush();
    }

    void done() const {
        std::cout << std::endl;
    }
private:
    size_t total_;
    size_t width_;
    size_t progress_;
}; 