cmake_minimum_required(VERSION 3.16)

project(MandelbrotGenerator VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

add_executable(mandelbrot mandelbrot.cpp bmp.c hue.c big_float.c arrshift.c
		big_int.c)
target_link_libraries(mandelbrot Threads::Threads)

add_executable(tst_arr_shift tst_arr_shift.c arrshift.c)
add_test(tst_arr_shift tst_arr_shift)

add_executable(tst_big_int_mul tst_big_int_mul.c big_int.c)
add_test(tst_big_int_mul tst_big_int_mul)
add_executable(bezier bezier.c)

install(TARGETS bezier)
install(TARGETS mandelbrot)
