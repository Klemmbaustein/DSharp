# CMakeList.txt : CMake project for ProgrammingLanguage, include source and define
# project specific logic here.
#
cmake_minimum_required(VERSION 3.15)

set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(CMAKE_CXX_STANDARD 23)


project(ProgrammingLanguage)

# Enable Hot Reload for MSVC compilers if supported.
if(POLICY CMP0141)
	cmake_policy(SET CMP0141 NEW)
	set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

option(BUILD_WINAPI_MODULE "Include Windows API standard module" ON)
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

add_executable(
	ProgrammingLanguage
	${SRCS} ${HEADERS})

if(BUILD_WINAPI_MODULE AND WIN32)
	target_compile_definitions(ProgrammingLanguage PUBLIC WITH_WINAPI=1)
endif()

if(HAS_CPP_FORMAT)
	target_compile_definitions(ProgrammingLanguage PUBLIC HAS_CPP_FORMAT=1)
endif()

if(MODULE_FS)
	target_compile_definitions(ProgrammingLanguage PUBLIC MODULE_FS=1)
endif()

if(HAS_POPEN)
	target_compile_definitions(ProgrammingLanguage PUBLIC HAS_POPEN=1)
endif()

if(HAS_FS_CURRENT_PATH)
	target_compile_definitions(ProgrammingLanguage PUBLIC HAS_FS_CURRENT_PATH=1)
endif()

target_include_directories(ProgrammingLanguage PUBLIC "include")