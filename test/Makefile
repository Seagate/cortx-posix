#sudo
SUDO = sudo
#or not
undefine SUDO

#executables
EXE = io_test

#archieve/node names
TARF = m0trace_$(shell date +%Y%m%d-%H%M%S).tar.bz2
TARN = $(shell ls -la m0trace.* &> /dev/null | wc -l)
NODE = $(shell eval uname -n)

#compiler/linker options
LFLAGS += -lm -lpthread -lrt -lgf_complete -lyaml -luuid -lmero
CFLAGS += -D_REENTRANT -D_GNU_SOURCE -DM0_INTERNAL='' -DM0_EXTERN=extern
CFLAGS += -fno-common -Wall -Werror -Wno-attributes -fno-strict-aliasing
CFLAGS += -fno-omit-frame-pointer -g -O2 -Wno-unused-but-set-variable
CFLAGS += -rdynamic
ifneq ($(M0_SRC_DIR),)
LFLAGS += -L$(M0_SRC_DIR)/mero/.libs -Wl,-rpath,$(M0_SRC_DIR)/mero/.libs
LFLAGS += -L$(M0_SRC_DIR)/extra-libs/gf-complete/src/.libs -Wl,-rpath,$(M0_SRC_DIR)/extra-libs/gf-complete/src/.libs
CFLAGS += -I$(M0_SRC_DIR)
endif

all: $(EXE)
.PHONY: all

$(EXE):
	gcc io_test.c -I/usr/include/mero $(CFLAGS) $(LFLAGS) -o $(EXE)

clean:
	rm -f $(EXE)
