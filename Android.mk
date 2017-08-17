LOCAL_PATH := $(my-dir)
#这样编译有个错误，不知道怎么修改，只能一个目录一个目录手动的去编译吧
#include $(call first-makefiles-under,$(LOCAL_PATH))

include $(CLEAR_VARS)

LOCAL_SHARED_LIBRARIES := libcutils liblog

LOCAL_CFLAGS  += -DQEMU_HARDWARE -std=c99
QEMU_HARDWARE := true

LOCAL_SRC_FILES += yang.c \
				   usb.c  \
				   blockutl.c \
				   roots.c \
				   uevent_demon.c

LOCAL_STATIC_LIBRARIES += \
		libfs_mgr \
		libmtdutils \
		libext4_utils_static


LOCAL_MODULE:= yang
LOCAL_MODULE_PATH := $(TARGET_OUT_EXECUTABLES)

include $(BUILD_EXECUTABLE)
