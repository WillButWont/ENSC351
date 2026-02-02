# CMake Build Configuration for root of project
cmake_minimum_required(VERSION 3.18)
project(reaction_timer VERSION 1.0 DESCRIPTION "Starter project" LANGUAGES C)

# ======================================================
# Use the BeagleY-AI cross-compiler
# Comment out this line if building for your host system
#set(CMAKE_C_COMPILER aarch64-linux-gnu-gcc)
# ======================================================

# Compiler options (inherited by sub-folders)
set(CMAKE_C_STANDARD 11)
add_compile_options(-Wall -Werror -Wpedantic -Wextra)
add_compile_options(-fdiagnostics-color)

# Enable address sanitizer
# (Comment this out to make your code faster)
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)

# Enable PThread library for linking
add_compile_options(-pthread)
add_link_options(-pthread)

# What folders to build
add_subdirectory(hal)  
add_subdirectory(app)

