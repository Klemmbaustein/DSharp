# CMakeList.txt : CMake project for DSharp, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.20)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CMAKE_CXX_STANDARD 23)

project(DSharp)

option(WITH_LANGUAGE_SERVICE "Include LanguageService class and related functionality." ON)
option(HAS_CPP_FORMAT "Has C++23 <format> library. Used for better compiler messages." ON)
option(MODULE_FS "Include system::fs module." ON)
option(HAS_POPEN "Has popen or _popen function." ON)
option(HAS_FS_CURRENT_PATH "Has std::filesystem::current_path()." ON)

file(
	GLOB_RECURSE
	SRCS
	"${SRC_DIR}/**.cpp"
)
file(
	GLOB_RECURSE
	HEADERS
	"${CMAKE_CURRENT_SOURCE_DIR}/include/**.hpp"
)

add_library(
	DSharp
	${SRCS} ${HEADERS})

if(WITH_LANGUAGE_SERVICE)
	target_compile_definitions(DSharp PUBLIC WITH_LANGUAGE_SERVICE=1)
endif()

if(HAS_CPP_FORMAT)
	target_compile_definitions(DSharp PUBLIC HAS_CPP_FORMAT=1)
endif()

if(MODULE_FS)
	target_compile_definitions(DSharp PUBLIC MODULE_FS=1)
endif()

if(HAS_POPEN)
	target_compile_definitions(DSharp PUBLIC HAS_POPEN=1)
endif()

if(HAS_FS_CURRENT_PATH)
	target_compile_definitions(DSharp PUBLIC HAS_FS_CURRENT_PATH=1)
endif()

target_include_directories(DSharp PUBLIC "include")

set(ASMJIT_STATIC ON)
add_subdirectory("dependencies/asmjit")
target_link_libraries(DSharp PUBLIC asmjit::asmjit)

if (CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
	enable_testing()
	add_subdirectory("test/runner")
endif()