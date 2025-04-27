cmake_minimum_required(VERSION 3.5)
project(gost34112018_proto CXX)

find_package(SystemCLanguage CONFIG REQUIRED)
find_package(Boost REQUIRED COMPONENTS program_options)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED true)

add_executable(gost34112018_test
    src/common.cxx
    src/control_logic.cxx
    src/stage.cxx
    src/tb.cxx
    src/utils.cxx
    src/gost34112018_hw.cxx
    src/transformations.cxx
)

target_include_directories(gost34112018_test PRIVATE
    res/include)

target_link_directories(gost34112018_test PRIVATE res/lib)

target_link_libraries(gost34112018_test PRIVATE
    gost34112018
    SystemC::systemc
    Boost::program_options
)
target_include_directories(gost34112018_test PRIVATE
    src
    Boost_INCLUDE_DIRS
)
