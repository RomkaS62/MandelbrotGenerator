add_library(mandelbrot-itr-double SHARED mandelbrot-itr-double.c)
target_include_directories(mandelbrot-itr-double PUBLIC "${INCLUDE_DIRS}")
target_link_libraries(mandelbrot-itr-double fractalgen)
install(TARGETS mandelbrot-itr-double DESTINATION "${PLUGIN_DIR}")

add_library(mandelbrot-itr-float SHARED mandelbrot-itr-float.c)
target_include_directories(mandelbrot-itr-float PUBLIC "${INCLUDE_DIRS}")
target_link_libraries(mandelbrot-itr-float fractalgen)
install(TARGETS mandelbrot-itr-float DESTINATION "${PLUGIN_DIR}")

add_library(render-rgb SHARED render-rgb.c)
target_include_directories(render-rgb PUBLIC "${INCLUDE_DIRS}")
target_link_libraries(render-rgb fractalgen)
install(TARGETS render-rgb DESTINATION "${PLUGIN_DIR}")
