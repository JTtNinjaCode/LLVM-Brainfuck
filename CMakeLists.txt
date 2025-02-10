cmake_minimum_required(VERSION 3.10)
project(SimpleLLVMProject)

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "LLVM include directory: ${LLVM_INCLUDE_DIRS}")
message(STATUS "LLVM libraries: ${LLVM_LIBRARY_DIRS}")
message(STATUS "LLVM definitions: ${LLVM_DEFINITIONS}")

include_directories(${LLVM_INCLUDE_DIRS})
add_definitions(${LLVM_DEFINITIONS})

add_executable(my_llvm_program main.cpp)
target_compile_features(my_llvm_program PRIVATE cxx_std_17)

llvm_map_components_to_libnames(LLVM_LIBS core irreader support)
target_link_libraries(my_llvm_program ${LLVM_LIBS})
