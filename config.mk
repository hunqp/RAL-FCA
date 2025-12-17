# FIRMWARE NAME: 1.1.6-2220-P03S-R18N-2024.09.16T10.16.44
# COMMENT
# 1. Version
# 2. Model apply
# 3. Datetime build

# Compiler path --====================================================================================
CROSS_COMPILE := $(HOME)/Toolchains/mips-xburst1-uclibc-ng1.0.50-toolchain-b6.3.1/bin/mipsel-linux-uclibc-
# --==================================================================================================

# Common definition
RELEASE		:= 0
VERSION		:= 1.0.0
BUILD_TIME 	:= $(shell date +"%Y.%m.%dT%H.%M.%S")

BUILD_INDOOR 	:= 1
BUILD_OUDOOR 	:= 0

# List software features
FEATURE_BLE		:= 1
FEATURE_RECORD	:= 0

# List hardware supports
SUPPORT_ETH			:= 0
SUPPORT_PTZ			:= 0
SUPPORT_FLOOD_LIGHT	:= 0

#######################################################
# Production models
#######################################################
PROD_MODEL		:= 
PROD_NAME		:= 
PROD_HARDWARE 	:=

ifeq ($(BUILD_INDOOR),1)
	PROD_MODEL		:= RAL-CMR-ID
	PROD_NAME		:= RAL Camera Indoor
	PROD_HARDWARE 	:= 1.0.0
else ifeq ($(BUILD_OUDOOR),1)
	PROD_MODEL		:= RAL-CMR-OD
	PROD_NAME		:= RAL Camera Outdoor
	PROD_HARDWARE 	:= 1.0.0
endif

RELEASE_PROD_MODEL  := $(PROD_MODEL)
RELEASE_PROD_NAME   := $(PROD_NAME)
RELEASE_PROD_FW  	:= $(RELEASE_PROD_MODEL)-$(VERSION).bin
