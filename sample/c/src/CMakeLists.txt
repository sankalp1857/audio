# Sensory Confidential
# Copyright (C)2025 Sensory, Inc. https://sensory.com/
#
# This is not a stand-alone configuration. See by ../CMakeLists.txt

add_executable(live-enroll live-enroll.c)
target_link_libraries(live-enroll SnsrLibraryOmitOSS)
install(TARGETS live-enroll DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(live-segment live-segment.c)
target_link_libraries(live-segment SnsrLibraryOmitOSS)
install(TARGETS live-segment DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(live-spot live-spot.c)
target_link_libraries(live-spot SnsrLibrary)
install(TARGETS live-spot DESTINATION ${SAMPLE_BINARY_DIR})

if (UNIX AND NOT APPLE)
  add_executable(live-spot-stream live-spot-stream.c alsa-stream.c)
  target_link_libraries(live-spot-stream SnsrLibrary)
  install(TARGETS live-spot-stream DESTINATION ${SAMPLE_BINARY_DIR})
elseif (WIN32)
  add_executable(live-spot-stream live-spot-stream.c wmme-stream.c)
  target_link_libraries(live-spot-stream SnsrLibrary)
  install(TARGETS live-spot-stream DESTINATION ${SAMPLE_BINARY_DIR})
endif ()

add_executable(push-audio push-audio.c)
target_link_libraries(push-audio SnsrLibrary)
install(TARGETS push-audio DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(snsr-edit snsr-edit.c)
target_link_libraries(snsr-edit SnsrLibrary)
install(TARGETS snsr-edit DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(snsr-eval snsr-eval.c)
target_link_libraries(snsr-eval SnsrLibrary)
install(TARGETS snsr-eval DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(spot-convert spot-convert.c)
target_link_libraries(spot-convert SnsrLibraryOmitOSS)
install(TARGETS spot-convert DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(spot-data spot-data.c spot-hbg-enUS-1.4.0-m.c data.c)
target_link_libraries(spot-data SnsrLibrary)
install(TARGETS spot-data DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(spot-data-stream spot-data-stream.c data-stream.c
               spot-hbg-enUS-1.4.0-m.c data.c)
target_link_libraries(spot-data-stream SnsrLibrary)
install(TARGETS spot-data-stream DESTINATION ${SAMPLE_BINARY_DIR})

add_executable(spot-enroll spot-enroll.c)
target_link_libraries(spot-enroll SnsrLibraryOmitOSS)
install(TARGETS spot-enroll DESTINATION ${SAMPLE_BINARY_DIR})
