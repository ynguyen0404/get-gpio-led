LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_MODULE := gpio_led
LOCAL_MODULE_TAGS := optional
LOCAL_MODULE_CLASS := EXECUTABLES
LOCAL_CLANG := true
LOCAL_CFLAGS := \
                -Wall \
				-Werror \
                -Wno-unused-parameter \
                -Wno-unused-variable \
                -Wno-unused-function \

LOCAL_CPPFLAGS := -Wall -Werror


SRC_FILES := \
        app_test_driver.c \

LOCAL_SRC_FILES := \
        $(SRC_FILES) 

LOCAL_MODULE_PATH   := $(PRODUCT_OUT)/vendor/bin/
include $(BUILD_EXECUTABLE)
