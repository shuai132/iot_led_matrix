set(CMAKE_CXX_STANDARD 14)

idf_component_register(SRCS "main2.cpp"
        INCLUDE_DIRS ".")

set(MYAPP __idf_main)

target_compile_options(${MYAPP} PRIVATE -Wno-missing-field-initializers -Wno-unknown-pragmas)
