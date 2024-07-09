cmake_minimum_required (VERSION 3.8)

if(CMAKE_CURRENT_SOURCE_DIR STREQUAL CMAKE_CURRENT_BINARY_DIR)
  message(FATAL_ERROR
    "In-source builds are not supported.  Please create a build directory "
    "separate from the source directory")
endif()

project(fastbit_query)
set (CMAKE_CXX_STANDARD 11)
set (CMAKE_CXX_STANDARD_REQUIRED ON)
set (CMAKE_CXX_FLAGS "-g -O3")

find_package(adios2 REQUIRED)
find_package(MPI REQUIRED)
find_library(FASTBIT_LIB fastbit HINTS "/home/4cv/project/fastbit/install_dir/lib/")
set (FASTBIT_INCLUDES "/home/4cv/project/fastbit/install_dir/include")

add_executable(fastbit_build_index fastbit_build_index.cpp)
target_include_directories(fastbit_build_index PRIVATE ${FASTBIT_INCLUDES})
target_link_libraries(fastbit_build_index adios2::cxx11 ${FASTBIT_LIB})

add_executable(fastbit_query fastbit_query.cpp)
target_include_directories(fastbit_query PRIVATE ${FASTBIT_INCLUDES})
target_link_libraries(fastbit_query adios2::cxx11 ${FASTBIT_LIB})


add_executable(generate_data generate_data.cpp)
target_link_libraries(generate_data adios2::cxx11_mpi)