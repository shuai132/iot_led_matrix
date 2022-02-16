idf_component_register(SRCS "main.cpp"
        matrix/LedMatrix.cpp
        INCLUDE_DIRS ".")

set(MYAPP __idf_main)

target_compile_options(${MYAPP} PRIVATE -Wno-missing-field-initializers)
