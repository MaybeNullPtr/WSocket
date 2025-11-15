cmake_minimum_required(VERSION 3.20)
project(WSocket)

set(CMAKE_CXX_STANDARD 17)

if (WIN32)
    set(ASIO_ROOT "I:/asio-1.36.0")
    set(ZSTD_ROOT "I:/zstd-v1.5.7")

    if (MSVC)
        add_compile_options(/Zc:preprocessor /utf-8 /DNOMINMAX /D_USE_MATH_DEFINES)
    endif()
else ()
    set(ASIO_ROOT "/mnt/i/asio-1.36.0")
endif ()

# asio
set(ASIO_INCLUDE_DIR "${ASIO_ROOT}/include")

# zstd
find_package(ZSTD REQUIRED)
if (ZSTD_FOUND)
    message(STATUS "zstd found")
    add_definitions(
            -DWITH_ZSTD
    )
    link_directories(
            ${ZSTD_ROOT}/lib
    )
    link_libraries(
            zstd_static
    )
endif ()

add_executable(test
        test.cpp
        include/WSocketContext.hpp
)
target_include_directories(
        test
        PRIVATE ${ASIO_INCLUDE_DIR}
        PRIVATE ${ZSTD_ROOT}/include
)
target_compile_definitions(
        test
        PRIVATE WITH_ASIO
)

