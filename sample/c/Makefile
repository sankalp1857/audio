# Sensory Confidential
# Copyright (C)2015-2025 Sensory, Inc. http://sensory.com/
#
# TrulyNatural SDK GNU make build script

SNSR_ROOT := ../..


SNSR_EDIT = $(BIN_DIR)/snsr-edit
# This Makefile is meant to run on the target platform.
# Uncomment the following line if cross-compiling instead.
# SNSR_EDIT = $(TOOL_DIR)/snsr-edit

# OS-specific compiler defaults
OS_NAME := $(shell uname -s)

ifeq ($(OS_NAME),Linux)
# Linux
ARCH_NAME := $(shell $(CC) -dumpmachine)
OS_CFLAGS := -O3 -fPIC -DNDEBUG
OS_CFLAGS += -Wall -Werror
OS_CFLAGS += -fdata-sections -ffunction-sections
OS_LIBS   := -lsnsr -lasound -lpthread -lm -ldl
OS_LDFLAGS+= -Wl,--gc-sections
STATSIZE  := stat -c %s

else ifeq ($(OS_NAME),Darwin)
# macOS
ARCH_NAME := macos
ARCH := $(shell uname -m)
XCODE := /Applications/Xcode.app/Contents/Developer
SYSROOT := $(XCODE)/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
CC := $(XCODE)/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang
OS_ARCH   := -arch $(ARCH)
OS_CFLAGS := -O3 -fPIC -DNDEBUG
OS_CFLAGS += $(OS_ARCH)
OS_CFLAGS += -Wall -Werror
OS_CFLAGS += -isysroot $(SYSROOT)
OS_CFLAGS += -fdata-sections -ffunction-sections
OS_LDFLAGS+= -isysroot $(SYSROOT)
OS_LDFLAGS+= -dead_strip
OS_LDFLAGS+= $(OS_ARCH)
OS_LIBS   := -lsnsr -framework AudioToolbox -framework CoreFoundation
OS_LIBS   += -framework Foundation -framework Accelerate
OS_LIBS   += -lm
STATSIZE  := stat -f %z

else
$(error This operating system ($(OS_NAME)) is not supported)
endif

OS_CFLAGS  += -I$(SNSR_ROOT)/include
OS_LDFLAGS += -L$(SNSR_ROOT)/lib/$(ARCH_NAME)

TARGET_DIR := .
BIN_DIR    = $(TARGET_DIR)/bin
SRC_DIR    = $(TARGET_DIR)/src
OBJ_DIR    = $(BUILD_DIR)/obj
OUT_DIR    = $(BUILD_DIR)/out
BUILD_DIR  = $(TARGET_DIR)/build
TEST_DIR   = $(TARGET_DIR)/test

MODEL_DIR  = $(SNSR_ROOT)/model
DATA_DIR   = $(SNSR_ROOT)/data
TOOL_DIR   = $(SNSR_ROOT)/bin

# $(call audio-files,filename-prefix,index-list)
# e.g. $(call audio-files,armadillo-6-,0 1 2)
# returns a list of absolute paths to SDK enrollment test data
audio-files = $(addsuffix .wav,$(addprefix $(DATA_DIR)/enrollments/$1,$2))

TEST_DATA := $(call audio-files,armadillo-1-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,armadillo-6-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,jackalope-1-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,jackalope-4-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,terminator-2-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,terminator-6-,0 1 2 3 4 5)
TEST_DATA += $(call audio-files,armadillo-1-,0-c 1-c 2-c 3-c 4-c 5-c)
TEST_DATA += $(call audio-files,jackalope-1-,0-c 1-c 2-c 3-c 4-c 5-c)

UDT_MODEL   = $(MODEL_DIR)/udt-universal-3.66.1.9.snsr
UDT_MODEL_5 = $(MODEL_DIR)/udt-enUS-5.1.1.9.snsr
VTPL_MODEL  = $(MODEL_DIR)/tpl-spot-vad-3.8.0.snsr
HBG_MODEL   = $(MODEL_DIR)/spot-hbg-enUS-1.4.0-m.snsr
VG_MODEL    = $(MODEL_DIR)/spot-voicegenie-enUS-6.5.1-m.snsr
BASE_MODEL  = $(OUT_DIR)/enrolled-sv

.PHONY: all clean debug help test
.PHONY: test-enroll-0 test-enroll-1 test-enroll-2 test-enroll-3
.PHONY: test-convert-0
.PHONY: test-push-0 test-push-1

define help
Make targets:

  make all      # build all executables in $(BIN_DIR)
  make clean    # remove build artifacts
  make debug    # build all with debugging enabled
  make help     # display this help message
  make test     # run enrollment and spotting tests

Building for $(ARCH_NAME) from SDK root directory
$(SNSR_ROOT)

endef

# Adjust test program verbosity
# Resolves to -v, unless make is run with the -s (silent) flag.
v = $(if $(findstring s,$(MAKEFLAGS)),,-v)

# Default target
help:; $(info $(help))

clean:
	rm -rf $(BIN_DIR) $(BUILD_DIR) $(OBJ_DIR) $(OUT_DIR) segmented-audio.wav
	rm -f $(SRC_DIR)/snsr-custom-init.c

debug: all
debug: CFLAGS=-O0 -g -UNDEBUG

test: test-enroll-0 test-enroll-1 test-enroll-2 test-enroll-3\
      test-convert-0 test-push-0 test-push-1 test-data-0 test-data-1\
      test-subset-0
	$(info SUCCESS: All tests passed.)

# End-to-end UDT enrollment test
test-enroll-0: $(BIN_DIR)/spot-enroll $(BIN_DIR)/snsr-eval | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-enroll $v $v -t $(UDT_MODEL)\
	  -o $(BASE_MODEL)-0.snsr\
	  +armadillo-6 $(call audio-files,armadillo-6-,0 1 2 3)\
	  +jackalope-4 $(call audio-files,jackalope-4-,0 1 2 3)\
	  +terminator-2 $(call audio-files,terminator-2-,0 1 2 3)\
	  +terminator-6 $(call audio-files,terminator-6-,0 1 2 3)\
	  +armadillo-1 $(call audio-files,armadillo-1-,0 1)\
	   -c $(call audio-files,armadillo-1-,0-c)\
	   -c $(call audio-files,armadillo-1-,1-c)\
	  +jackalope-1 $(call audio-files,jackalope-1-,0 1)\
	   -c $(call audio-files,jackalope-1-,0-c)\
	   -c $(call audio-files,jackalope-1-,1-c)
	$(BIN_DIR)/snsr-eval -t $(BASE_MODEL)-0.snsr $(TEST_DATA)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 100)

# End-to-end UDT enrollment test, using adapted enrollment contexts
test-enroll-1: $(BIN_DIR)/spot-enroll $(BIN_DIR)/snsr-eval | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/armadillo-6.snsr\
	  -o $(OUT_DIR)/enrolled-armadillo-6.snsr\
	  +armadillo-6 $(call audio-files,armadillo-6-,0 1 2 3)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/jackalope-4.snsr\
	  -o $(OUT_DIR)/enrolled-jackalope-4.snsr\
	  +jackalope-4 $(call audio-files,jackalope-4-,0 1 2 3)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/terminator-2.snsr\
	  -o $(OUT_DIR)/enrolled-terminator-2.snsr\
	  +terminator-2 $(call audio-files,terminator-2-,0 1 2 3)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/terminator-6.snsr\
	  -o $(OUT_DIR)/enrolled-terminator-6.snsr\
	  +terminator-6 $(call audio-files,terminator-6-,0 1 2 3)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/armadillo-1.snsr\
	  -o $(OUT_DIR)/enrolled-armadillo.snsr\
	  +armadillo-1 $(call audio-files,armadillo-1-,0 1)\
	   -c $(call audio-files,armadillo-1-,0-c)\
	   -c $(call audio-files,armadillo-1-,1-c)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -a $(OUT_DIR)/jackalope-1.snsr\
	  -o $(OUT_DIR)/enrolled-jackalope-1.snsr\
	  +jackalope-1 $(call audio-files,jackalope-1-,0 1)\
	   -c $(call audio-files,jackalope-1-,0-c)\
	   -c $(call audio-files,jackalope-1-,1-c)
	$(BIN_DIR)/spot-enroll $v -t $(UDT_MODEL)\
	  -t $(OUT_DIR)/armadillo-6.snsr\
	  -t $(OUT_DIR)/jackalope-4.snsr\
	  -t $(OUT_DIR)/terminator-2.snsr\
	  -t $(OUT_DIR)/terminator-6.snsr\
	  -t $(OUT_DIR)/armadillo-1.snsr\
	  -t $(OUT_DIR)/jackalope-1.snsr\
	  -o $(BASE_MODEL)-1.snsr
	$(BIN_DIR)/snsr-eval -t $(BASE_MODEL)-1.snsr $(TEST_DATA)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt >/dev/null\
	  || diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@-alt.txt >/dev/null\
	  || (echo ERROR: $@ validation failed; exit 101)

# Live end-to-end UDT enrollment test.
test-enroll-2: $(BIN_DIR)/live-enroll $(BIN_DIR)/snsr-eval | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/live-enroll $v $v -t $(UDT_MODEL)\
	  -o $(BASE_MODEL)-2.snsr\
	  +armadillo-1 $(call audio-files,armadillo-1-,0 1 0-c 1-c)
	$(BIN_DIR)/snsr-eval -t $(BASE_MODEL)-2.snsr $(TEST_DATA)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 102)

# Test old UDT model
test-enroll-3: $(BIN_DIR)/spot-enroll $(BIN_DIR)/snsr-eval | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-enroll $v $v -t $(UDT_MODEL_5)\
	  -o $(BASE_MODEL)-3.snsr\
	  +armadillo-1 $(call audio-files,armadillo-1-,0 1 2 3)
	$(BIN_DIR)/snsr-eval -t $(BASE_MODEL)-3.snsr $(TEST_DATA)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 103)

# Validate DSP conversion
test-convert-0: $(BIN_DIR)/spot-convert | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-convert -t $(HBG_MODEL) -p $(OUT_DIR)/dsp pc38
	tail -10 $(OUT_DIR)/dsp-pc38-3.4.0-op10-prod-search.h\
	  > $(OUT_DIR)/dsp-search-check.h
	shasum -c $(TEST_DIR)/dsp-checksum.txt

# Push audio samples instead of the default pull
# Uses test-enroll-0 models
test-push-0: test-enroll-0 $(BIN_DIR)/push-audio | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/push-audio $(BASE_MODEL)-0.snsr\
	  $(call audio-files,jackalope-4-,0)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 104)

# Push audio samples instead of the default pull
# Uses test-enroll-0 models and the tpl-spot-vad-*.snsr template
test-push-1: test-enroll-0 $(BIN_DIR)/push-audio $(SNSR_EDIT) | $(OUT_DIR)
	$(info Running $@.)
	$(SNSR_EDIT) -t $(VTPL_MODEL)\
	  -f 0 $(BASE_MODEL)-0.snsr -o $(OUT_DIR)/spot-vad.snsr
	$(BIN_DIR)/push-audio $(OUT_DIR)/spot-vad.snsr\
	  $(call audio-files,armadillo-1-,1-c)\
	  > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 105)

test-data-0: $(BIN_DIR)/spot-data | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-data > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 104)

test-data-1: $(BIN_DIR)/spot-data-stream | $(OUT_DIR)
	$(info Running $@.)
	$(BIN_DIR)/spot-data-stream > $(OUT_DIR)/$@.txt
	diff $(OUT_DIR)/$@.txt $(TEST_DIR)/$@.txt\
	  || (echo ERROR: $@ validation failed; exit 104)

test-subset-0: $(BIN_DIR)/snsr-eval-subset $(BIN_DIR)/snsr-eval | $(OUT_DIR)
	$(info Running $@.)
	test $(shell $(STATSIZE) $(BIN_DIR)/snsr-eval-subset) -lt \
	     $(shell $(STATSIZE) $(BIN_DIR)/snsr-eval) ||\
	  (echo ERROR: $@ size validation failed; exit 105)
	$(BIN_DIR)/snsr-eval-subset -t $(HBG_MODEL) /dev/null ||\
	  (echo ERROR: $@ validation failed; exit 106)
	$(BIN_DIR)/snsr-eval-subset -t $(VG_MODEL) /dev/null 2>&1 |\
	  grep SNSR_USE_SUBSET >/dev/null ||\
	  (echo ERROR: $@ validation failed; exit 107)

# Create a rule for building name from source, in $(BIN_DIR)
# $(call add-target-rule,name,source1.c source2.c ...)
add-target-rule = $(eval $(call emit-target-rule,$1,$2))
define emit-target-rule
all: $$(BIN_DIR)/$(strip $1)
$$(BIN_DIR)/$(strip $1): $$(addprefix $$(OBJ_DIR)/,$(2:.c=.o)) | $$(BIN_DIR)
	$$(CC) $$(OS_LDFLAGS) $$(LDFLAGS) -o $$@ $$^ $$(OS_LIBS) $$(LIBS)
endef

# Command-line application targets
$(call add-target-rule, spot-convert, spot-convert.c)
$(call add-target-rule, snsr-edit,    snsr-edit.c)
$(call add-target-rule, spot-enroll,  spot-enroll.c)
$(call add-target-rule, snsr-eval,    snsr-eval.c)
$(call add-target-rule, snsr-eval-subset,\
       snsr-eval-subset.c snsr-custom-init.c)
$(call add-target-rule, live-enroll,  live-enroll.c)
$(call add-target-rule, live-segment, live-segment.c)
$(call add-target-rule, live-spot,    live-spot.c)
$(call add-target-rule, push-audio,    push-audio.c)
$(call add-target-rule, spot-data,\
       spot-data.c spot-hbg-enUS-1.4.0-m.c data.c)
$(call add-target-rule, spot-data-stream,\
       spot-data-stream.c data-stream.c spot-hbg-enUS-1.4.0-m.c data.c)

ifeq ($(OS_NAME),Linux)
# The custom stream sample uses ALSA and compiles on Linux only.
$(call add-target-rule, live-spot-stream, live-spot-stream.c alsa-stream.c)
endif

# Build object files from C sources
$(OBJ_DIR)/%.o : $(SRC_DIR)/%.c  | $(OBJ_DIR)
	$(CC) -c $(OS_CFLAGS) $(CFLAGS) -o $@ $<

# spot-enroll doesn't use OSS modules
$(OBJ_DIR)/spot-enroll.o : $(SRC_DIR)/spot-enroll.c  | $(OBJ_DIR)
	$(CC) -DSNSR_OMIT_OSS_COMPONENTS -c $(OS_CFLAGS) $(CFLAGS) -o $@ $<

# Create $(SRC_DIR)/snsr-custom-init.c using snsr-edit,
# limit support to those modules needed for $(HBG_MODEL)
$(SRC_DIR)/snsr-custom-init.c: $(SNSR_EDIT)
	$(SNSR_EDIT) -v -i -t $(HBG_MODEL) -o $@

# Build snsr-eval-subset object files with -DSNSR_USE_SUBSET
$(OBJ_DIR)/snsr-eval-subset.o: $(SRC_DIR)/snsr-eval.c | $(OBJ_DIR)
	$(CC) -c  $(OS_CFLAGS) $(CFLAGS) -DSNSR_USE_SUBSET -o $@ $<

# Create output directories
$(BIN_DIR) $(BUILD_DIR) $(OBJ_DIR) $(OUT_DIR):
	mkdir -p $@
