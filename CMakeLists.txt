cmake_minimum_required(VERSION 3.5)

# Set the project name
project (proto-lisp)

set(CMAKE_CXX_STANDARD 11) # C++11...
set(CMAKE_CXX_STANDARD_REQUIRED ON) #...is required...
set(CMAKE_CXX_EXTENSIONS OFF) 

include_directories(${CMAKE_CURRENT_SOURCE_DIR}) 
# Add an executable
add_executable(proto-lisp main.cpp)