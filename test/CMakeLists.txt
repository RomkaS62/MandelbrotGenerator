function(create_test)
	cmake_parse_arguments(ARG "" "NAME" "SOURCES" ${ARGN})

	add_executable("${ARG_NAME}" "${ARG_SOURCES}")
	target_include_directories("${ARG_NAME}" PRIVATE "${CMAKE_SOURCE_DIR}/include")
	add_test("${ARG_NAME}" "${ARG_NAME}")
endfunction()

create_test(NAME tst_arr_shift SOURCES tst_arr_shift.c "${CMAKE_SOURCE_DIR}/frgen/arrshift.c")
create_test(NAME tst_fixed_mul SOURCES tst_fixed_mul.c)
create_test(NAME tst_fixed_sqr SOURCES tst_fixed_sqr.c)
create_test(NAME tst_fcmplx_sqr SOURCES tst_fcmplx_sqr.c)

add_executable(bezier bezier.c)
