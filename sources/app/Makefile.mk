-include sources/app/net/Makefile.mk
-include sources/app/mqtt/Makefile.mk
-include sources/app/json/Makefile.mk
-include sources/app/ota/Makefile.mk
-include sources/app/webrtc/Makefile.mk
-include sources/app/gpio/Makefile.mk
-include sources/app/av/Makefile.mk
-include sources/app/button/Makefile.mk
-include sources/app/rtspd/Makefile.mk
-include sources/app/storage/Makefile.mk

CXXFLAGS += -I./sources/app

VPATH += sources/app

OBJ += $(OBJ_DIR)/app.o
OBJ += $(OBJ_DIR)/app_config.o
OBJ += $(OBJ_DIR)/app_data.o
OBJ += $(OBJ_DIR)/app_capture.o
OBJ += $(OBJ_DIR)/task_list.o

OBJ += $(OBJ_DIR)/task_sys.o
OBJ += $(OBJ_DIR)/task_cloud.o
OBJ += $(OBJ_DIR)/task_av.o
OBJ += $(OBJ_DIR)/task_detect.o
OBJ += $(OBJ_DIR)/task_config.o
OBJ += $(OBJ_DIR)/task_upload.o
OBJ += $(OBJ_DIR)/task_network.o
OBJ += $(OBJ_DIR)/task_webrtc.o
OBJ += $(OBJ_DIR)/task_fw.o
OBJ += $(OBJ_DIR)/task_record.o

