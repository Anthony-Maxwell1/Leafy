# Developer's makefile for building Lua
# see luaconf.h for further customization

# == USER SETTINGS ============================================================

CWARNSCPP= \
	-Wfatal-errors \
	-Wextra \
	-Wshadow \
	-Wundef \
	-Wwrite-strings \
	-Wredundant-decls \
	-Wdisabled-optimization \
	-Wdouble-promotion \
	-Wmissing-declarations \
	-Wconversion \
	# -Wstrict-overflow=2 \
	# -Werror \
	# -pedantic \
	# -Wformat=2 \
	# -Wcast-qual

CWARNGCC= \
	-Wlogical-op \
	-Wno-aggressive-loop-optimizations

CWARNSC= \
	-Wdeclaration-after-statement \
	-Wmissing-prototypes \
	-Wnested-externs \
	-Wstrict-prototypes \
	-Wc++-compat \
	-Wold-style-definition

CWARNS= $(CWARNSCPP) $(CWARNSC) $(CWARNGCC)

LOCAL = $(TESTS) $(CWARNS)

MYCFLAGS= $(LOCAL) -std=c99 -DLUA_USE_LINUX
MYLDFLAGS= -Wl,-E
MYLIBS= -ldl

# Cross compiler tools
CC=arm-kindlehf-linux-gnueabihf-gcc
AR=arm-kindlehf-linux-gnueabihf-ar
ARFLAGS=rc
RANLIB=arm-kindlehf-linux-gnueabihf-ranlib

RM=rm -f

# Install location (ONE DIRECTORY UP)
INSTALL_DIR=../

# == END USER SETTINGS ========================================================

LIBS = -lm

CORE_T=	liblua.a
CORE_O=	lapi.o lcode.o lctype.o ldebug.o ldo.o ldump.o lfunc.o lgc.o llex.o \
	lmem.o lobject.o lopcodes.o lparser.o lstate.o lstring.o ltable.o \
	ltm.o lundump.o lvm.o lzio.o ltests.o

AUX_O=	lauxlib.o
LIB_O=	lbaselib.o ldblib.o liolib.o lmathlib.o loslib.o ltablib.o lstrlib.o \
	lutf8lib.o loadlib.o lcorolib.o linit.o

LUA_T=	lua
LUA_O=	lua.o

ALL_T= $(CORE_T) $(LUA_T)
ALL_O= $(CORE_O) $(LUA_O) $(AUX_O) $(LIB_O)

# ----------------------------------------------------------------------------

all: $(ALL_T)

o: $(ALL_O)

a: $(CORE_T)

# ----------------------------------------------------------------------------
# Build static library
# ----------------------------------------------------------------------------

$(CORE_T): $(CORE_O) $(AUX_O) $(LIB_O)
	$(AR) $(ARFLAGS) $@ $?
	$(RANLIB) $@

# ----------------------------------------------------------------------------
# Build interpreter (optional)
# ----------------------------------------------------------------------------

$(LUA_T): $(LUA_O) $(CORE_T)
	$(CC) -o $@ $(MYLDFLAGS) $(LUA_O) $(CORE_T) $(LIBS) $(MYLIBS)

# ----------------------------------------------------------------------------
# Install step (THIS IS WHAT YOU ASKED FOR)
# ----------------------------------------------------------------------------

install: $(CORE_T)
	@echo "Installing Lua to $(INSTALL_DIR)..."

	@mkdir -p $(INSTALL_DIR)

	cp lua.h luaconf.h lualib.h lauxlib.h lprefix.h $(INSTALL_DIR)
	cp liblua.a $(INSTALL_DIR)

	@echo "Lua installed successfully."

# ----------------------------------------------------------------------------

clean:
	$(RM) $(ALL_T) $(ALL_O)

depend:
	@$(CC) $(CFLAGS) -MM *.c

echo:
	@echo "CC = $(CC)"
	@echo "CFLAGS = $(CFLAGS)"
	@echo "AR = $(AR)"
	@echo "RANLIB = $(RANLIB)"

$(ALL_O): makefile ltests.h

# DO NOT EDIT BELOW (auto dependencies)