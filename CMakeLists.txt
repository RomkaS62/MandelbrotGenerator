cmake_minimum_required(VERSION 3.16)

project(MandelbrotGenerator VERSION 1.0)

find_package(Threads REQUIRED)

add_executable(mandelbrot mandelbrot.cpp bmp.c hue.c)
target_link_libraries(mandelbrot Threads::Threads)

add_executable(bezier bezier.c)

install(TARGETS bezier)
install(TARGETS mandelbrot)
