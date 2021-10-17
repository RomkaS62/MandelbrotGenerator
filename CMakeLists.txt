cmake_minimum_required(VERSION 3.16)

project(MandelbrotGenerator)

find_package(Threads REQUIRED)

add_executable(mandelbrot mandelbrot.cpp bmp.c hue.c)
target_link_libraries(mandelbrot Threads::Threads)
