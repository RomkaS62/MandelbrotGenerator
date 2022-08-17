add_library("mandelbrot-itr-double" SHARED mandelbrot-itr-double.c)
add_library("render-rgb" SHARED render-rgb.c)

target_include_directories("mandelbrot-itr-double" PUBLIC "${INCLUDE_DIRS}")
target_include_directories("render-rgb" PUBLIC "${INCLUDE_DIRS}")

install(TARGETS mandelbrot-itr-double DESTINATION "${PLUGIN_DIR}")
install(TARGETS render-rgb DESTINATION "${PLUGIN_DIR}")
