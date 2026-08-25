CC ?= cc
ENGINE_DIR ?= ../kryon
BUILD_ROOT ?= build
KRYON_BACKEND ?= raylib
PLAN9PORT_DIR ?= /mnt/storage/Projects/plan9port
PREFIX ?= $(HOME)/.local
BINDIR ?= $(PREFIX)/bin
RILL_APP_HOSTDIR ?= $(PREFIX)/lib/rill/apps
INSTALL ?= install

UNAME_S := $(shell uname -s 2>/dev/null)
UNAME_M := $(shell uname -m 2>/dev/null)
ifeq ($(UNAME_M),amd64)
  ARCH := x86_64
else
  ARCH := $(UNAME_M)
endif
ifeq ($(UNAME_S),Linux)
  PLATFORM := linux
else ifeq ($(UNAME_S),FreeBSD)
  PLATFORM := freebsd
else ifeq ($(UNAME_S),Darwin)
  PLATFORM := macos
else
  PLATFORM := $(UNAME_S)
endif

BUILD_DIR ?= $(BUILD_ROOT)/$(PLATFORM)-$(ARCH)
ENGINE_BUILD_ROOT ?= $(ENGINE_DIR)/build
ENGINE_BUILD_DIR ?= $(ENGINE_BUILD_ROOT)/$(PLATFORM)-$(ARCH)
ENGINE_LIB = $(ENGINE_BUILD_DIR)/libkryon.a
RAYLIB_A = $(ENGINE_BUILD_DIR)/raylib/libraylib.a
LIBOQS_A = $(ENGINE_BUILD_DIR)/vendor/liboqs/lib/liboqs.a
CURL_A = $(ENGINE_BUILD_DIR)/vendor/curl/lib/libcurl.a
CMARK_A = $(ENGINE_BUILD_DIR)/vendor/cmark-gfm/src/libcmark-gfm.a
CMARK_EXT_A = $(ENGINE_BUILD_DIR)/vendor/cmark-gfm/extensions/libcmark-gfm-extensions.a
BOX2D_A = $(ENGINE_BUILD_DIR)/vendor/box2d/src/libbox2d.a

APP = $(BUILD_DIR)/bin/shelf
HOST_LIB = $(BUILD_DIR)/lib/libshelf_host.a
HOST_SO = $(BUILD_DIR)/lib/shelf-host.so
TEST = $(BUILD_DIR)/tests/shelf_model_test
APP_OBJS = $(BUILD_DIR)/src/main.o $(BUILD_DIR)/src/shelf.o
HOST_OBJS = $(BUILD_DIR)/src/shelf.o $(BUILD_DIR)/src/shelf_host.o
TEST_OBJS = $(BUILD_DIR)/tests/shelf_model_test.o $(BUILD_DIR)/src/shelf.o

RAY_SDL_CFLAGS ?= $(shell pkg-config --cflags sdl2 2>/dev/null)
RAY_SDL_LDLIBS ?= $(shell pkg-config --libs sdl2 2>/dev/null)
RAY_GL_CFLAGS ?= $(shell pkg-config --cflags libdrm gbm egl glesv2 2>/dev/null)
RAY_GL_LDLIBS ?= $(shell pkg-config --libs libdrm gbm egl glesv2 2>/dev/null)
RAY_LDLIBS ?= $(strip $(RAY_SDL_LDLIBS) $(RAY_GL_LDLIBS))
ifeq ($(KRYON_BACKEND),raylib)
  BACKEND_CFLAGS = $(RAY_SDL_CFLAGS) $(RAY_GL_CFLAGS)
  BACKEND_LIBS = $(RAYLIB_A)
  BACKEND_LDLIBS = $(RAY_LDLIBS)
else ifeq ($(KRYON_BACKEND),libdraw)
  BACKEND_CFLAGS = -DKRYON_BACKEND_LIBDRAW -I$(PLAN9PORT_DIR)/include
  BACKEND_LIBS =
  BACKEND_LDLIBS = -L$(PLAN9PORT_DIR)/lib -ldraw -lmemdraw -lmux -lthread -l9
else
  $(error Unknown KRYON_BACKEND '$(KRYON_BACKEND)' (expected raylib or libdraw))
endif
SYSTEM_THEME_PKG := $(shell if pkg-config --exists gtk+-3.0 2>/dev/null; then printf '%s' gtk+-3.0; fi)
SYSTEM_THEME_CFLAGS := $(shell if [ -n "$(SYSTEM_THEME_PKG)" ]; then pkg-config --cflags $(SYSTEM_THEME_PKG); fi)
SYSTEM_THEME_LDLIBS := $(shell if [ -n "$(SYSTEM_THEME_PKG)" ]; then pkg-config --libs $(SYSTEM_THEME_PKG); fi)
CURL_CODEC_LDLIBS ?= $(strip \
  $(shell pkg-config --libs libbrotlidec 2>/dev/null) \
  $(shell pkg-config --libs libbrotlicommon 2>/dev/null) \
  $(shell pkg-config --libs libzstd 2>/dev/null))
ifeq ($(PLATFORM),linux)
  PLATFORM_LDLIBS ?= -ldl -lrt
else
  PLATFORM_LDLIBS ?=
endif

CFLAGS ?= -Wall -Wextra -O2
ifeq ($(PLATFORM),linux)
  CFLAGS += -fPIC
endif
CPPFLAGS += -Isrc -I$(ENGINE_DIR)/include \
	$(BACKEND_CFLAGS) $(SYSTEM_THEME_CFLAGS) \
	-DHAS_LIBOQS=1 -I$(ENGINE_BUILD_DIR)/vendor/liboqs/include \
	-DHAS_LIBCURL=1 -DCURL_STATICLIB -I$(ENGINE_BUILD_DIR)/vendor/curl/include \
	-DKRYON_HAS_CMARK_GFM=1 \
	-I$(ENGINE_DIR)/vendor/cmark-gfm/src -I$(ENGINE_DIR)/vendor/cmark-gfm/extensions \
	-I$(ENGINE_BUILD_DIR)/vendor/cmark-gfm/src -I$(ENGINE_BUILD_DIR)/vendor/cmark-gfm/extensions
LDLIBS += $(BACKEND_LIBS) $(BOX2D_A) $(BACKEND_LDLIBS) $(LIBOQS_A) \
	$(CURL_A) -lssl -lcrypto -lpthread $(CMARK_EXT_A) $(CMARK_A) \
	$(SYSTEM_THEME_LDLIBS) $(CURL_CODEC_LDLIBS) -lz -lm $(PLATFORM_LDLIBS)

.PHONY: all run test clean install engine

all: $(APP) $(HOST_LIB) $(HOST_SO)

engine:
	$(MAKE) -C $(ENGINE_DIR) KRYON_BACKEND=$(KRYON_BACKEND) \
		BUILD_ROOT=$(ENGINE_BUILD_ROOT) $(ENGINE_LIB)

$(APP): engine $(APP_OBJS) $(ENGINE_LIB) $(BACKEND_LIBS) | $(BUILD_DIR)/bin
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(APP_OBJS) \
		-Wl,--whole-archive $(ENGINE_LIB) -Wl,--no-whole-archive \
		$(LDLIBS)

$(HOST_LIB): engine $(HOST_OBJS) | $(BUILD_DIR)/lib
	ar rcs $@ $(HOST_OBJS)

$(HOST_SO): engine $(HOST_OBJS) | $(BUILD_DIR)/lib
	$(CC) $(CFLAGS) -shared -o $@ $(HOST_OBJS)

$(TEST): engine $(TEST_OBJS) $(ENGINE_LIB) $(BACKEND_LIBS) | $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $(CPPFLAGS) -o $@ $(TEST_OBJS) \
		-Wl,--whole-archive $(ENGINE_LIB) -Wl,--no-whole-archive \
		$(LDLIBS)

$(BUILD_DIR)/src/%.o: src/%.c src/*.h | $(BUILD_DIR)/src
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/tests/%.o: tests/%.c src/*.h | $(BUILD_DIR)/tests
	$(CC) $(CFLAGS) $(CPPFLAGS) -c $< -o $@

$(BUILD_DIR)/bin $(BUILD_DIR)/lib $(BUILD_DIR)/src $(BUILD_DIR)/tests:
	mkdir -p $@

run: $(APP)
	$(APP)

test: $(TEST)
	$(TEST)

install: $(APP) $(HOST_SO)
	mkdir -p $(DESTDIR)$(BINDIR) $(DESTDIR)$(RILL_APP_HOSTDIR)
	$(INSTALL) -m 755 $(APP) $(DESTDIR)$(BINDIR)/shelf
	$(INSTALL) -m 755 $(HOST_SO) $(DESTDIR)$(RILL_APP_HOSTDIR)/shelf-host.so

clean:
	rm -rf $(BUILD_ROOT)
