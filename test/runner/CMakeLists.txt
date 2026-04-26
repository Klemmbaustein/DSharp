add_executable(DSharpTest "test.cpp")
target_link_libraries(DSharpTest PRIVATE DSharp)

file(GLOB TEST_FILES "${CMAKE_SOURCE_DIR}/test/*.ds")

foreach(FILE IN ITEMS ${TEST_FILES})
	cmake_path(GET FILE FILENAME OUT_NAME)
	cmake_path(REMOVE_EXTENSION OUT_NAME)
	add_test(NAME "ds.test.${OUT_NAME}" COMMAND DSharpTest ${FILE})
endforeach()