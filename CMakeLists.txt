cmake_minimum_required(VERSION 3.15)
project(phu-arp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE
set(JUCE_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/JUCE" CACHE PATH "Path to JUCE root")
add_subdirectory("${JUCE_ROOT}")

add_subdirectory(src)