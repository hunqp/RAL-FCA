#############################################################################################
# HungPNQ - Build librtspd555.a (Static Library Only, all sources)
#############################################################################################

.SUFFIXES:

-include $(shell pwd)/../../config.mk

#############################################################################################
# Paths and Toolchain
#############################################################################################

OPENSSL_DIR     := $(shell pwd)/../../libraries/libopenssl-1.1.1h
OPENSSL_INCLUDE := $(OPENSSL_DIR)/include
OPENSSL_DEPS    := -L$(OPENSSL_DIR)/lib -lssl -lcrypto

AR      := $(CROSS_COMPILE)ar
CC      := $(CROSS_COMPILE)gcc
CXX     := $(CROSS_COMPILE)g++
RANLIB  := $(CROSS_COMPILE)ranlib

#############################################################################################
# Directories
#############################################################################################

GROUPSOCK_DIR        := groupsock
LIVEMEDIA_DIR        := liveMedia
USAGE_ENV_DIR        := UsageEnvironment
BASIC_USAGE_ENV_DIR  := BasicUsageEnvironment

#############################################################################################
# Compiler Flags
#############################################################################################

COMPILE_FLAGS += -Os
COMPILE_FLAGS += -DSOCKLEN_T=socklen_t -DNO_SSTREAM=1 \
                 -D_LARGEFILE_SOURCE=1 -D_FILE_OFFSET_BITS=64 \
                 -DBSD=1 -DLOCALE_NOT_USED -DSO_REUSEPORT \
                 -DALLOW_SERVER_PORT_REUSE -DALLOW_RTSP_SERVER_PORT_REUSE

INCLUDE_FLAGS += -I$(GROUPSOCK_DIR)/include -I$(LIVEMEDIA_DIR)/include \
                 -I$(USAGE_ENV_DIR)/include -I$(BASIC_USAGE_ENV_DIR)/include \
                 -I. -Icommon -I$(OPENSSL_INCLUDE)

CFLAGS   := $(COMPILE_FLAGS) $(INCLUDE_FLAGS)
CXXFLAGS := $(COMPILE_FLAGS) $(INCLUDE_FLAGS) -std=c++11

TARGET_LIB := librtspd555.a

#############################################################################################
# Source Collection (recursive)
#############################################################################################

# Find all .cpp and .c files in these directories
SRC_DIRS := . $(GROUPSOCK_DIR) $(LIVEMEDIA_DIR) $(USAGE_ENV_DIR) $(BASIC_USAGE_ENV_DIR)
SRCS     := $(foreach dir,$(SRC_DIRS),$(wildcard $(dir)/*.cpp) $(wildcard $(dir)/*.c))

OBJS := $(SRCS:.cpp=.o)
OBJS := $(OBJS:.c=.o)

#############################################################################################
# Sanity Check (important LIVE555 dependency)
#############################################################################################

ifeq ($(wildcard $(GROUPSOCK_DIR)/NetAddress.cpp),)
$(error ❌ Missing required file: $(GROUPSOCK_DIR)/NetAddress.cpp — cannot build librtspd555.a)
endif

#############################################################################################
# Build Rules
#############################################################################################

all: $(TARGET_LIB)
	@echo "Built $(TARGET_LIB)"
	@ls -lh $(TARGET_LIB)

$(TARGET_LIB): $(OBJS)
	@echo "📦 Archiving $@ ($(words $(OBJS)) objects)"
	@$(AR) rcs $@ $(OBJS)
	@$(RANLIB) $@

# Compile C source
%.o: %.c
	@echo "CC  $<"
	@mkdir -p $(dir $@)
	@$(CC) $(CFLAGS) -c $< -o $@

# Compile C++ source
%.o: %.cpp
	@echo "CXX $<"
	@mkdir -p $(dir $@)
	@$(CXX) $(CXXFLAGS) -c $< -o $@

#############################################################################################
# Clean Rules
#############################################################################################

clean:
	@echo "🧹 Cleaning object files..."
	@find . -name "*.o" -delete

dist-clean: clean
	@echo "🧹 Removing $(TARGET_LIB)"
	@rm -f $(TARGET_LIB)

.PHONY: all clean dist-clean
