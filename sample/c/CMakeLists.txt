# Sensory Confidential
# Copyright (C)2025 Sensory, Inc. https://sensory.com/
#
# TrulyNatural SDK sample code build configuration
#
# Configure, build, and install these samples with:
#
# cmake -S . -B build-sample
# cmake --build build-sample --parallel --config Release
# cmake --install build-sample
#
# Then find the sample executables in build-sample/bin/

cmake_minimum_required(VERSION 3.10.0)
project(SnsrSamples)

list(APPEND CMAKE_MODULE_PATH "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0")
include(SnsrLibrary)

set(SAMPLE_BINARY_DIR ${PROJECT_BINARY_DIR}/bin)
add_subdirectory(src)
