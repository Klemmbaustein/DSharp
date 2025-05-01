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

file(
    GLOB_RECURSE
    SRCS
    "${SRC_DIR}/**.cpp"
)

add_executable(
   ProgrammingLanguage
   ${SRCS}
)

target_include_directories(ProgrammingLanguage PUBLIC "include")