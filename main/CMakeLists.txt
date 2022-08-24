set(CMAKE_CXX_STANDARD 14)

idf_component_register(SRCS "esp_hid_host_main.c"
        esp_hid_gap.c
        INCLUDE_DIRS ".")

set(MYAPP __idf_main)

target_compile_options(${MYAPP} PRIVATE -Wno-missing-field-initializers -Wno-unknown-pragmas)
