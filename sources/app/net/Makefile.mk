-include sources/app/net/eth/Makefile.mk
-include sources/app/net/wifi/Makefile.mk
-include sources/app/net/ble/Makefile.mk

CXXFLAGS += -Isources/app/net
VPATH += sources/app/net

OBJ += $(OBJ_DIR)/network_manager.o
OBJ += $(OBJ_DIR)/network_data_encrypt.o
