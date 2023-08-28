add_executable(frgen fractalgen.cpp bmp.c parse.c global.c frgen_string.c plugin.c)
target_link_libraries(frgen Threads::Threads dl gramas fractalgen)
target_include_directories(frgen PRIVATE "${CMAKE_SOURCE_DIR}/include")

if (NOT MSVC)
	target_link_libraries(fractalgen m)
endif ()

add_executable(bexpr bexpr.c big_int.c parse.c)
target_include_directories(bexpr PRIVATE "${CMAKE_SOURCE_DIR}/include")

if (NOT MSVC)
	target_link_libraries(bexpr m)
endif()

