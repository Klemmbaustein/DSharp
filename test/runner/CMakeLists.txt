add_executable(DSharpTest "test.cpp")
target_link_libraries(DSharpTest PRIVATE DSharp)

function(test_category category_name)

	file(GLOB TEST_FILES "${CMAKE_SOURCE_DIR}/test/${category_name}/*.ds")

	foreach(FILE IN ITEMS ${TEST_FILES})
		cmake_path(GET FILE FILENAME OUT_NAME)
		cmake_path(REMOVE_EXTENSION OUT_NAME LAST_ONLY)

		add_test(NAME "ds.${category_name}.${OUT_NAME}" COMMAND DSharpTest ${FILE} WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/test/${category_name}")
	endforeach()
endfunction()

test_category("language")
test_category("library")