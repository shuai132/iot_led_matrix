#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <cstring>

class Print {
public:
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const char* data, size_t len) {
        for (int i = 0; i < len; ++i) {
            write(data[i]);
        }
        return len;
    };
    inline void print(const char* str) {
        write(str, strlen(str));
    }
};

class __FlashStringHelper {};

using String = std::string;
