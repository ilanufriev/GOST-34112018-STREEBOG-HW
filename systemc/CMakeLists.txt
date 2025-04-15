cmake_minimum_required(VERSION 3.5)
project(gost34112018_proto CXX)

find_package(SystemCLanguage CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 17) 
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(gost34112018_test 
    src/tb.cxx
)

target_link_libraries(gost34112018_test SystemC::systemc)
target_include_directories(gost34112018_test PRIVATE src)
