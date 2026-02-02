cmake_minimum_required(VERSION 3.10)
project(BeatBox C)

# Set C Standard
set(CMAKE_C_STANDARD 99)

# Define _GNU_SOURCE to fix CLOCK_BOOTTIME and other Linux extensions
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D_GNU_SOURCE")

# Add the sub-projects
# Order matters: Build HAL first, then the App
add_subdirectory(hal)
add_subdirectory(app)