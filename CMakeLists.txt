cmake_minimum_required(VERSION 3.16)

project(fractalgen VERSION 1.0)

find_package(Threads REQUIRED)
enable_testing()

set(PLUGIN_DIR "${CMAKE_INSTALL_PREFIX}/lib/fractalgen")

if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
	add_compile_definitions(_ISOC11_SOURCE)
endif ()

set(INCLUDE_DIRS "${CMAKE_SOURCE_DIR}" "${CMAKE_SOURCE_DIR}/include")

add_subdirectory(libfractalgen)
add_subdirectory(frgen)
add_subdirectory(test)
add_subdirectory(plugins)

install(TARGETS bezier)
install(TARGETS frgen)
install(DIRECTORY include/fractalgen DESTINATION "${CMAKE_INSTALL_PREFIX}/include")
