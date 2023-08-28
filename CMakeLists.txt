cmake_minimum_required(VERSION 3.16)

project(fractalgen VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

set(PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/lib/fractalgen")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	set(EXTRA_FLAGS "-ffast-math -fopenmp -fcf-protection=none -fno-stack-protector")
	set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${EXTRA_FLAGS}")
	add_compile_definitions(_ISOC11_SOURCE)

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

if (MSVC)
	set(CMAKE_C_FLAGS, "${CMAKE_C_FLAGS} /openmp")
endif ()

set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(libfractalgen)
add_subdirectory(frgen)
add_subdirectory(test)
add_subdirectory(plugins)

install(TARGETS bezier)
install(TARGETS frgen)
install(DIRECTORY include/fractalgen DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
