#pragma once

#include <string>
#include <cstdint>
#include <cstddef>
#include <cstring>

class Print {
public:
    virtual size_t write(uint8_t) = 0;
    virtual size_t write(const uint8_t* data, size_t len) {
        for (int i = 0; i < len; ++i) {
            write(data[i]);
        }
        return len;
    };
    void print(const char* str){
        write(reinterpret_cast<const uint8_t*>(str), strlen(str));
    }
};

class __FlashStringHelper {};

using String = std::string;
