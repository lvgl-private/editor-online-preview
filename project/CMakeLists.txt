# Minimum CMake version required
cmake_minimum_required(VERSION 3.10)

# Project name
project(HelloWorld)

# Specify the C standard
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)

# Add executable target
add_executable(hello main.c)
