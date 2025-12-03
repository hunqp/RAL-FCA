CXXFLAGS += -Isources/app/av

VPATH += sources/app/av

OBJ	+= $(OBJ_DIR)/fca_isp.o
# OBJ	+= $(OBJ_DIR)/fca_osd.o
OBJ	+= $(OBJ_DIR)/fca_gpio.o
OBJ	+= $(OBJ_DIR)/fca_video.o
OBJ	+= $(OBJ_DIR)/fca_audio.o
OBJ	+= $(OBJ_DIR)/fca_motion.o
OBJ	+= $(OBJ_DIR)/fca_parameter.o