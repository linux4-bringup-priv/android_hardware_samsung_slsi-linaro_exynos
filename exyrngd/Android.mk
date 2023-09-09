LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := exyrngd
ifeq ($(BOARD_USES_FIPS_COMPLIANCE_RNG_DRV),true)
	LOCAL_CFLAGS += -DUSES_FIPS_COMPLIANCE_RNG_DRV
endif
LOCAL_SRC_FILES := \
		exyrngd.c
LOCAL_SHARED_LIBRARIES := libc libcutils
#LOCAL_CFLAGS := -DANDROID_CHANGES
LOCAL_MODULE_TAGS := optional
LOCAL_PROPRIETARY_MODULE := true
include $(BUILD_EXECUTABLE)

