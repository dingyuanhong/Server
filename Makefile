CORSS_PREFIX :=
SYSROOT :=

$(warning $(CORSS_PREFIX))
$(warning $(SYSROOT))

CC = $(CORSS_PREFIX)gcc
AR = $(CORSS_PREFIX)ar
RM = rm -rf

#原始目录
SRC_PATH :=.
MODULES :=
#源文件
MODULES += $(wildcard $(SRC_PATH)/Core/*.c)
MODULES += $(wildcard $(SRC_PATH)/Core/Lock/*.c)
MODULES += $(wildcard $(SRC_PATH)/Core/Queue/*.c)
MODULES += $(wildcard $(SRC_PATH)/Event/*.c)
MODULES += $(wildcard $(SRC_PATH)/Module/*.c)
MODULES += $(wildcard $(SRC_PATH)/Function/*.c)
#目标名
TARGET =

SRCS = $(MODULES)
SRCS += $(wildcard $(SRC_PATH)/*.c)
#中间文件
OBJS = $(SRCS:.c=.o)

MODULE_OBJS := $(MODULES:.c=.o)
OBJ_TEST=server.o
OBJS_TEST=$(MODULE_OBJS) $(OBJ_TEST)
TARGET_TEST=server

OBJ_INFO=client.o
OBJS_INFO=$(MODULE_OBJS) $(OBJ_INFO)
TARGET_INFO=client

ALL_OBJS=$(OBJS) $(OBJS_TEST) $(OBJ_INFO)

#动态库
LIBS := pthread rt

#头文件路径
INCLUDE_PATH := $(SYSROOT)/usr/include
INCLUDE_PATH +=.
INCLUDE_PATH += ./include
INCLUDE_PATH += ./Core
INCLUDE_PATH += ./Event
INCLUDE_PATH += ./Module

#动态库路径
LIBRARY_PATH := $(SYSROOT)/usr/lib/ $(SYSROOT)/usr/local/lib/
LIBRARY_PATH += ./

RELEASE = 1
BITS =

#ifeq ( 1 , ${DBG_ENABLE} )
#	CFLAGS += -D_DEBUG -O0 -g -DDEBUG=1
#endif

CFLAGS = -Wall -DMAIN_TEST -DUSE_BOOL -std=gnu99 -D_POSIX_C_SOURCE=199309L
LFLAGS =

#头文件
CFLAGS += $(foreach dir,$(INCLUDE_PATH),-I$(dir))

#库路径
LDFLAGS += $(foreach lib,$(LIBRARY_PATH),-L$(lib))

#库名
LDFLAGS += $(foreach lib,$(LIBS),-l$(lib))

#检查版本
ifeq ($(RELEASE),0)
	#debug
	CFLAGS += -g
else
	#release
	CFLAGS += -O3 -DNDEBUG
endif

#检查位宽
ifeq ($(BITS),32)
	CFLAGS += -m32
	LFLAGS += -m32
else
	ifeq ($(BITS),64)
		CFLAGS += -m64
		LFLAGS += -m64
	else
	endif
endif

$(warning $(OBJS))

#操作命令
all:clean build

$(ALL_OBJS):%.o:%.c
	$(CC) $(CFLAGS) -c $^ -o $@

build_static:$(OBJS)
	# $(AR) -cru $(TARGET) $(OBJS)

build_test:build_static $(OBJS_TEST)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET_TEST) $(OBJS_TEST) $(LDFLAGS)

build_info:build_static $(OBJS_INFO)
	$(CC) $(CFLAGS) $(LFLAGS) -o $(TARGET_INFO) $(OBJS_INFO) $(LDFLAGS)

build:build_test build_info
	$(RM) $(ALL_OBJS)

clean:
	echo $(SRCS)
	$(RM) $(ALL_OBJS) $(TARGET) $(TARGET_TEST) $(TARGET_INFO)
