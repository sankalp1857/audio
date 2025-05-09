# Install script for directory: /Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-enroll")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/live-enroll")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-enroll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-enroll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-enroll")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-segment")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/live-segment")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-segment" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-segment")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-segment")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-spot")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/live-spot")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-spot" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-spot")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/live-spot")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/push-audio")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/push-audio")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/push-audio" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/push-audio")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/push-audio")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-edit")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/snsr-edit")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-edit" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-edit")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-edit")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-eval")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/snsr-eval")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-eval" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-eval")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/snsr-eval")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-convert")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/spot-convert")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-convert" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-convert")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-convert")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/spot-data")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data-stream")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/spot-data-stream")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data-stream" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data-stream")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-data-stream")
    endif()
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  list(APPEND CMAKE_ABSOLUTE_DESTINATION_FILES
   "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-enroll")
  if(CMAKE_WARN_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(WARNING "ABSOLUTE path INSTALL DESTINATION : ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  if(CMAKE_ERROR_ON_ABSOLUTE_INSTALL_DESTINATION)
    message(FATAL_ERROR "ABSOLUTE path INSTALL DESTINATION forbidden (by caller): ${CMAKE_ABSOLUTE_DESTINATION_FILES}")
  endif()
  file(INSTALL DESTINATION "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin" TYPE EXECUTABLE FILES "/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/src/spot-enroll")
  if(EXISTS "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-enroll" AND
     NOT IS_SYMLINK "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-enroll")
    if(CMAKE_INSTALL_DO_STRIP)
      execute_process(COMMAND "/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/strip" -u -r "$ENV{DESTDIR}/Users/sankalp/Sensory/TrulyNaturalSDK/7.4.0/sample/c/build-sample/bin/spot-enroll")
    endif()
  endif()
endif()

