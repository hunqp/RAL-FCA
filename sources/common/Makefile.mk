-include sources/common/lwIP/Makefile.mk
-include sources/common/RNCryptor-C/Makefile.mk

CXXFLAGS += -Isources/common

VPATH += sources/common

OBJ += $(OBJ_DIR)/ntpd.o
OBJ += $(OBJ_DIR)/cmd_line.o
OBJ += $(OBJ_DIR)/firmware.o
OBJ += $(OBJ_DIR)/utils.o
OBJ += $(OBJ_DIR)/STUNExternalIP.o
OBJ += $(OBJ_DIR)/g711.o
OBJ += $(OBJ_DIR)/h26xparsec.o
OBJ += $(OBJ_DIR)/mediabuffer.o
OBJ += $(OBJ_DIR)/rncryptor_c.o
OBJ += $(OBJ_DIR)/mutils.o
