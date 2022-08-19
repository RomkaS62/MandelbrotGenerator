cmake_minimum_required(VERSION 3.16)

project(fractalgen VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

set(PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/lib/fractalgen")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	if (DEFINED FAST_MATH)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -ffast-math")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
	endif ()

	if (DEFINED ARCH)
		set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=${GCC_ARCH}")
		set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=${GCC_ARCH}")
	endif ()

	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra")
endif ()

set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(libfractalgen)

add_executable(frgen fractalgen.cpp bmp.c parse.c global.c frgen_string.c)
target_link_libraries(frgen Threads::Threads dl gramas fractalgen)
target_include_directories(frgen PRIVATE "${INCLUDE_DIRS}")

if (NOT MSVC)
	target_link_libraries(fractalgen m)
endif ()

add_executable(bexpr bexpr.c big_int.c parse.c)

if (NOT MSVC)
	target_link_libraries(bexpr m)
endif()

add_executable(tst_arr_shift tst_arr_shift.c arrshift.c)
add_test(tst_arr_shift tst_arr_shift)

add_executable(tst_fixed_mul tst_fixed_mul.c)
add_test(tst_fixed_mul tst_fixed_mul)

add_executable(tst_fixed_sqr tst_fixed_sqr.c)
add_test(tst_fixed_sqr tst_fixed_sqr)

add_executable(tst_fcmplx_sqr tst_fcmplx_sqr.c)
add_test(tst_fcmplx_sqr tst_fcmplx_sqr)

add_executable(bezier bezier.c)

add_subdirectory(plugins)

install(TARGETS bezier)
install(TARGETS frgen)
install(DIRECTORY include/fractalgen DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
