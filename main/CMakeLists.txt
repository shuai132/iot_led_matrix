set(CMAKE_CXX_STANDARD 14)

idf_component_register(SRCS "main.cpp"
        matrix/LedMatrix.cpp
        matrix/LEDCanvas.cpp
        gfx/Adafruit_GFX.cpp
        wifi/smartconfig.cpp
        wifi/wifi_station.cpp
        wifi/sntp.cpp
        adc/adc_dma.cpp
        adc/fft.c
        esp_misc.cpp
        EventLoop.cpp
        INCLUDE_DIRS ".")

set(MYAPP __idf_main)

target_compile_options(${MYAPP} PRIVATE -Wno-missing-field-initializers -Wno-unknown-pragmas)
