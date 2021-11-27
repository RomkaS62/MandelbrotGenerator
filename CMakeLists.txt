cmake_minimum_required(VERSION 3.16)

project(MandelbrotGenerator VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

if (DEFINED GCC_FAST_MATH)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
endif ()

if (DEFINED GCC_ARCH)
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${GCC_ARCH}")
endif ()

add_executable(mandelbrot mandelbrot.cpp bmp.c hue.c arrshift.c iterated.c
	iteratef.c fixed.c big_int.c parse.c global.c)
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

add_executable(tst_fixed_mul tst_fixed_mul.c)
add_test(tst_fixed_mul tst_fixed_mul)

add_executable(tst_fixed_sqr tst_fixed_sqr.c)
add_test(tst_fixed_sqr tst_fixed_sqr)

add_executable(tst_fcmplx_sqr tst_fcmplx_sqr.c)
add_test(tst_fcmplx_sqr tst_fcmplx_sqr)

add_executable(bezier bezier.c)

install(TARGETS bezier)
install(TARGETS mandelbrot)
