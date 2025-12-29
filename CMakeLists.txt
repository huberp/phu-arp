cmake_minimum_required(VERSION 3.15)
project(phu-arp LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# JUCE
set(JUCE_ROOT $ENV{JUCE_ROOT} CACHE PATH "Path to JUCE")
if(NOT JUCE_ROOT)
    message(FATAL_ERROR "JUCE_ROOT not set")
endif()

include_directories(${JUCE_ROOT}/modules)

add_subdirectory(src)