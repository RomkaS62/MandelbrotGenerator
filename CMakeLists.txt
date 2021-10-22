cmake_minimum_required(VERSION 3.16)

project(MandelbrotGenerator VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

add_executable(mandelbrot mandelbrot.cpp bmp.c hue.c big_float.c arrshift.c
		big_int.c parse.c)
target_link_libraries(mandelbrot Threads::Threads)
if (NOT MSVC)
	target_link_libraries(mandelbrot m)
endif ()

add_executable(bexpr bexpr.c big_int.c parse.c)
if (NOT MSVC)
	target_link_libraries(bexpr m)
endif()

add_executable(tst_arr_shift tst_arr_shift.c arrshift.c)
add_test(tst_arr_shift tst_arr_shift)

add_executable(tst_big_int_mul tst_big_int_mul.c big_int.c parse.c)
add_test(tst_big_int_mul tst_big_int_mul)
if (NOT MSVC)
	target_link_libraries(tst_big_int_mul m)
endif ()

add_executable(bezier bezier.c)

install(TARGETS bezier)
install(TARGETS mandelbrot)
