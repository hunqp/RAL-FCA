###############################################################################
# author: ThanNT
# date: 6/1/2017
###############################################################################

-include config.mk
-include libraries/Makefile.mk
-include sources/Makefile.mk

CXX	  := $(CROSS_COMPILE)g++
CC	  := $(CROSS_COMPILE)gcc
AR	  := $(CROSS_COMPILE)ar
STRIP := $(CROSS_COMPILE)strip
OBJNM := $(CROSS_COMPILE)nm

# Release build option
OBJ_DIR 	:= build
NAME_MODULE	:= PeerConnection

# Library option
LDFLAGS	+= -Wl,-Map=$(OBJ_DIR)/$(NAME_MODULE).map -Wl,--gc-sections 
LDLIBS	+= -lpthread -lrt -lm

# General flag options
ifeq ($(FEATURE_RECORD),1)
GENERAL_FLAGS += -DFEATURE_RECORD
endif

ifeq ($(FEATURE_BLE),1)
GENERAL_FLAGS += -DFEATURE_BLE
endif

ifeq ($(SUPPORT_ETH),1)
GENERAL_FLAGS += -DSUPPORT_ETH
endif

ifeq ($(SUPPORT_PTZ),1)
GENERAL_FLAGS += -DSUPPORT_PTZ
endif

ifeq ($(SUPPORT_FLOOD_LIGHT),1)
GENERAL_FLAGS += -DSUPPORT_FLOOD_LIGHT
endif

#|---------------------------------------------------------------------------------------------------|
#| OPTIMIZE LEVELS                                                                                   |
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   option   | optimization levels              |execution time|code size|memory usage|complile time|
#|------------|----------------------------------|--------------|---------|------------|-------------|
#|   -O0      | compilation time                 |     (+)      |   (+)   |     (-)    |    (-)      |
#| -O1 || -O  | code size && execution time      |     (-)      |   (-)   |     (+)    |    (+)      |
#|   -O2      | more code size && execution time |     (--)     |         |     (+)    |    (++)     |
#|   -O3      | more code size && execution time |     (---)    |         |     (+)    |    (+++)    |
#|   -Os      | code size                        |              |   (--)  |            |    (++)     |
#|  -Ofast    | O3 with none math cals           |     (---)    |         |     (+)    |    (+++)    |
#|------------|----------------------------------|--------------|---------|------------|-------------|
OPTIMIZE_OPTION	:= -g -s -O3
# WARNNING_OPTS += -Werror -W -Wno-missing-field-initializers -Wextra

ifeq ($(RELEASE),1)
	GENERAL_FLAGS += -DRELEASE=1
	WARNNING_OPTS +=
endif

CXXFLAGS += $(OPTIMIZE_OPTION)	\
			$(WARNNING_OPTS)	\
			$(GENERAL_FLAGS)	\
			-Wall -pipe -ffunction-sections	-fdata-sections

.PHONY: all create copy clean

all: create $(OBJ_DIR)/$(NAME_MODULE)
	@$(STRIP) $(OBJ_DIR)/$(NAME_MODULE)
	@size $(OBJ_DIR)/$(NAME_MODULE)

create:
	@echo mkdir -p $(OBJ_DIR)
	@mkdir -p $(OBJ_DIR)
	rm -rf $(OBJ_DIR)/$(NAME_MODULE)

$(OBJ_DIR)/%.o: %.cpp
	@echo CXX $<
	@$(CXX) -c -o $@ $< $(CXXFLAGS) -std=c++17 $(LDFLAGS)

$(OBJ_DIR)/%.o: %.c
	@echo CC $<
	@$(CC) -c -o $@ $< $(CXXFLAGS) $(LDFLAGS)

$(OBJ_DIR)/$(NAME_MODULE): $(OBJ)
	@echo ---------- START LINK PROJECT ----------
	@echo LIB $(LDLIBS) "\n"
	@echo $(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)
	@$(CXX) -o $@ $^ $(CXXFLAGS) $(LDFLAGS) $(LDLIBS)

flash: $(OBJ_DIR)/$(NAME_MODULE)
	@cp $(OBJ_DIR)/$(NAME_MODULE) $(HOME)/Shared/net
	@echo "COPY $^"

clean: 
	@echo rm -rf $(OBJ_DIR)
	@rm -rf $(OBJ_DIR)

