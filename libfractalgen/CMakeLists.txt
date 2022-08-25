add_library(fractalgen SHARED param_set.c memmove.c)
target_include_directories(fractalgen PUBLIC ${INCLUDE_DIRS})
install(TARGETS fractalgen)
