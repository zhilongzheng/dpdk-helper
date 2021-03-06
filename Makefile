## Derive from Netbricks

CC = gcc

#ifndef RTE_SDK
#$(error RTE_SDK is undefined)
#endif

ifndef RTE_TARGET
	RTE_TARGET="build"
endif

ifneq ($(wildcard $(RTE_SDK)/$(RTE_TARGET)*),)
	DPDK_INC_DIR = $(RTE_SDK)/$(RTE_TARGET)/include
	DPDK_LIB_DIR = $(RTE_SDK)/$(RTE_TARGET)/lib
else
	DPDK_INC_DIR = $(RTE_SDK)/build/include
	DPDK_LIB_DIR = $(RTE_SDK)/build/lib
endif

LDFLAGS += -L$(DPDK_LIB_DIR)
LIBS += -Wl,--whole-archive -ldpdk -Wl,--no-whole-archive -Wl,-rpath=$(DPDK_LIB_DIR)
#LIBS += -ldpdk -Wl,-rpath=$(DPDK_LIB_DIR)

# change fpic to fPIC if something fails
CFLAGS = -std=gnu99 -g3 -ggdb3 -O3 -Wall -Werror -m64 -march=native \
	 -Wno-unused-function -Wno-unused-but-set-variable \
	 -I$(DPDK_INC_DIR) -Iincludes/\
	 -D_GNU_SOURCE \
	 -fPIC

INCLUDES = -I./includes/

SRCS = $(wildcard *.c)
OBJS = $(SRCS:.c=.o)
HEADERS = $(wildcard includes/*.h)
PROD = libgolfdpdk.so
PROD_STATIC = libgolfdpdk.a

DEPS = .make.dep

# if multiple targets are specified, do them one by one */
ifneq ($(words $(MAKECMDGOALS)),1)

.NOTPARALLEL:
$(sort all $(MAKECMDGOALS)):
	@$(MAKE) --no-print-directory -f $(firstword $(MAKEFILE_LIST)) $@

else

# parallel build by default
CORES ?= $(shell nproc || echo 1)
MAKEFLAGS += -j $(CORES)
INSTALL_PATH = ../../target/libs/

.PHONY: all clean tags cscope all-static

#all: $(DEPS) $(PROD)  # Some prolem happened when producing .so

all: $(DEPS) $(PROD_STATIC)

install: $(DEPS) $(PROD_STATIC) | $(INSTALL_PATH)
	cp $(PROD_STATIC) $(INSTALL_PATH)

$(INSTALL_PATH):
	mkdir -p $(INSTALL_PATH)


$(DEPS): $(SRCS) $(HEADERS)
	@echo $(RTE_SDK) $(DPDK_INC_DIR)
	@$(CC) $(CFLAGS) $(INCLUDES) -MM $(SRCS) | sed 's|\(.*\)\.o: \(.*\)\.c|\2.o: \2.c|' > $(DEPS);

$(PROD_STATIC): $(OBJS)
	ar rcs $(PROD_STATIC) $(OBJS)

#$(PROD): $(OBJS)
#	$(CC) -shared  $(OBJS) -o $@ $(LDFLAGS) $(LIBS)

-include $(DEPS)

clean:
	rm -f $(DEPS) $(PROD) $(PROD_STATIC) *.o || true
	rm -f $(INSTALL_PATH)/$(PROD) || true
	rmdir $(INSTALL_PATH) || true

tags:
	@ctags -R *

cscope:
	@rm -f cscope.*
	@find . -name "*.c" -o -name "*.h" > cscope.files
	cscope -b -q -k
	@rm -f cscope.files
endif
