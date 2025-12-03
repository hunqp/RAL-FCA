CXXFLAGS += -Isources/common/lwIP

VPATH += sources/common/lwIP

OBJ += $(OBJ_DIR)/lwcgi.o
OBJ += $(OBJ_DIR)/lwNTPd.o