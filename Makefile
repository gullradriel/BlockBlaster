#
# BlockBlaster by GullRaDriel, started in February 2026
#
# Supports Linux / Windows (MinGW) / Emscripten (WASM) / Android build
#
# Usage:
#   make                  -- build the native Linux/Windows desktop binary
#   make wasm             -- build the Emscripten WebAssembly version
#   make android          -- build the debug Android APK
#   make android-release  -- build the release-signed Android APK
#   make android-aab      -- build the release-signed Android App Bundle
#   make clean            -- remove desktop build artefacts
#   make clean-all        -- remove all build artefacts (desktop + wasm + android)
#

# --------------------------------------------------------------------------
# Default toolchain and flags (overridden per-platform below)
# --------------------------------------------------------------------------
RM=rm -f
CC=gcc
EXT=
CLIBS=

# Release optimisation flags.  The commented-out line below enables extra
# hardening (stack protector, FORTIFY_SOURCE, static libgcc) for debug builds.
OPT=-W -Wall -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED -std=gnu99 -O3
#OPT=-g -W -Wall -Wextra -std=gnu99 -O3 -fstack-protector-strong -D_FORTIFY_SOURCE=2 -D_REENTRANT -D_XOPEN_SOURCE=600 -D_XOPEN_SOURCE_EXTENDED -static-libgcc -static-libstdc++

# Source files live in src/, object files in obj/
VPATH=src
INCLUDE=-Isrc
OBJDIR=obj

# Allegro 5 shared libraries required at link time
ALLEGRO_LIBS=-lallegro_acodec -lallegro_audio -lallegro_color -lallegro_image -lallegro_main -lallegro_primitives -lallegro_ttf -lallegro_font -lallegro

# Enable Allegro unstable API (ALLEGRO_FULLSCREEN_WINDOW, etc.)
CFLAGS+= -DALLEGRO_UNSTABLE

# --------------------------------------------------------------------------
# Platform detection and per-platform overrides
# --------------------------------------------------------------------------
ifeq ($(OS),Windows_NT)
    # Windows (MinGW / MSYS2) -- enables C99 printf format macros
    CFLAGS+= $(INCLUDE) -D__USE_MINGW_ANSI_STDIO $(OPT)
    RM= del /Q
    CC= gcc

    # MSYS2 MINGW32 environment (32-bit)
    ifeq (${MSYSTEM},MINGW32)
        RM=rm -f
        CFLAGS+= -m32
        EXT=.exe
        CLIBS=-IC:/msys64/mingw32/include -LC:/msys64/mingw32/lib
    endif

    # MSYS2 MINGW64 environment (64-bit, from MSYS2 shell)
    ifeq (${MSYSTEM},MINGW64)
        RM=rm -f
        CFLAGS+= -DARCH64BITS
        EXT=.exe
        CLIBS=-IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib
    endif

    # MSYS2 MINGW64 environment invoked from Code::Blocks IDE
    ifeq (${MSYSTEM},MINGW64CB)
        RM=del /Q
        CFLAGS+= -DARCH64BITS
        EXT=.exe
        CLIBS=-IC:/msys64/mingw64/include -LC:/msys64/mingw64/lib
    endif

    # Link Allegro statically for pthreads, dynamically for ws2_32, suppress console
    CLIBS+= $(ALLEGRO_LIBS) -Wl,-Bstatic -lpthread  -Wl,-Bdynamic -lws2_32  -L../LIB/. -mwindows
else
    # Unix-like systems (Linux, SunOS)
    UNAME_S= $(shell uname -s)
    RM=rm -f
    CC=gcc
    EXT=

    ifeq ($(UNAME_S),Linux)
        CFLAGS+= $(INCLUDE) $(OPT)
        CLIBS+= $(ALLEGRO_LIBS) -lpthread -lm -no-pie
    endif

    # SunOS / Solaris support (uses the Sun C compiler)
    ifeq ($(UNAME_S),SunOS)
        CC=cc
        CFLAGS+= -D_LARGEFILE64_SOURCE -D_FILE_OFFSET_BITS=64 -g -v -xc99 -I ../../LIB/include/ -mt -lm
        CLIBS+= $(ALLEGRO_LIBS) -lm -lsocket -lnsl -lpthread -lrt -L..
    endif
endif


# --------------------------------------------------------------------------
# Source files and object file generation
# --------------------------------------------------------------------------

# All C source files.  Nilorea utility library sources (n_*.c) and cJSON
# are compiled alongside the game sources.  The Emscripten-only files
# are conditionally compiled via #ifdef __EMSCRIPTEN__ inside the source.
SRC=n_common.c n_log.c n_str.c n_list.c cJSON.c \
	allegro_emscripten_mouse.c allegro_emscripten_fullscreen.c \
    blockblaster_audio.c blockblaster_game.c blockblaster_render.c \
    blockblaster_ui.c BlockBlaster.c

# Derive object file list from the source list
OBJ=$(patsubst %.c,$(OBJDIR)/%.o,$(SRC))

# Ensure the object directory exists before any compilation
$(shell mkdir -p $(OBJDIR))

# Pattern rule: compile each .c into obj/*.o
$(OBJDIR)/%.o: %.c
	$(CC) $(CFLAGS) -o $@ -c $<

# --------------------------------------------------------------------------
# Desktop binary target (default)
# --------------------------------------------------------------------------
BlockBlaster$(EXT): $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^ $(CLIBS)

all: BlockBlaster$(EXT)


# ==========================================================================
# Emscripten (WebAssembly) build
# ==========================================================================
#
# The WASM build compiles the same SRC list using emcc and links against a
# static Allegro 5 monolith built with Emscripten.  Third-party dependencies
# (libogg, libvorbis, FreeType) are cloned from upstream and cross-compiled
# into the deps prefix.
#

WASM_BUILD_DIR=build_emscripten
ANDROID_BUILD_DIR=build_android

# Upstream library source directories (cloned on demand by wasm-deps)
ALLEGRO_DIR=allegro5
LIBOGG_DIR=libogg
LIBVORBIS_DIR=libvorbis
LIBFREETYPE_DIR=freetype

# Upstream Git repositories for third-party dependencies
ALLEGRO_REPO=https://github.com/liballeg/allegro5.git
LIBOGG_REPO=https://github.com/xiph/ogg.git
LIBVORBIS_REPO=https://github.com/xiph/vorbis.git
LIBFREETYPE_REPO=https://gitlab.freedesktop.org/freetype/freetype.git

# Prefix for installed WASM headers and static libraries
DEPS_PREFIX=$(WASM_BUILD_DIR)/deps/prefix

ALLEGRO_BUILD_DIR=$(ALLEGRO_DIR)/build_emscripten
LIBOGG_BUILD_DIR=$(LIBOGG_DIR)/build_emscripten
LIBVORBIS_BUILD_DIR=$(LIBVORBIS_DIR)/build_emscripten

# Default ABI for single-ABI builds; ANDROID_ABIS lists all ABIs to ship.
# x86 and x86_64 are disabled by default (uncomment to add emulator support).
ANDROID_ABI?=arm64-v8a
ANDROID_ABIS?=armeabi-v7a arm64-v8a
#x86 x86_64
ANDROID_API?=24
ANDROID_PACKAGE?=org.gullradriel.blockblaster
ANDROID_APP_NAME?=BlockBlaster
ANDROID_ACTIVITY?=BlockBlasterActivity

ANDROID_MANIFEST=android/AndroidManifest.xml
ANDROID_RES_DIR=android/res

ANDROID_DEPS_PREFIX=$(ANDROID_BUILD_DIR)/deps/prefix/$(ANDROID_ABI)
ANDROID_ALLEGRO_BUILD_DIR=$(ALLEGRO_DIR)/build_android_$(ANDROID_ABI)
ANDROID_LIBOGG_BUILD_DIR=$(LIBOGG_DIR)/build_android_$(ANDROID_ABI)
ANDROID_LIBVORBIS_BUILD_DIR=$(LIBVORBIS_DIR)/build_android_$(ANDROID_ABI)
ANDROID_LIBFREETYPE_BUILD_DIR=$(LIBFREETYPE_DIR)/build_android_$(ANDROID_ABI)
ANDROID_NATIVE_LIB_DIR=$(ANDROID_BUILD_DIR)/native/$(ANDROID_ABI)
ANDROID_ALLEGRO_MONOLITH_LIB=$(ANDROID_DEPS_PREFIX)/lib/liballegro_monolith-static.a
ANDROID_APK_DIR=$(ANDROID_BUILD_DIR)/apk
ANDROID_UNSIGNED_APK=$(ANDROID_APK_DIR)/BlockBlaster-unsigned.apk
ANDROID_UNALIGNED_APK=$(ANDROID_APK_DIR)/BlockBlaster-unaligned.apk
ANDROID_APK=$(ANDROID_APK_DIR)/BlockBlaster-debug.apk
ANDROID_FINAL_APK?=BlockBlaster-debug.apk

ANDROID_SDK_ROOT?=$(HOME)/Android/Sdk
ANDROID_NDK?=$(firstword $(wildcard $(ANDROID_SDK_ROOT)/ndk/*) $(ANDROID_SDK_ROOT)/ndk-bundle)
ANDROID_TOOLCHAIN_FILE=$(ANDROID_NDK)/build/cmake/android.toolchain.cmake
ANDROID_HOST_TAG?=$(shell uname -s | tr '[:upper:]' '[:lower:]')-x86_64
ANDROID_PREBUILT=$(ANDROID_NDK)/toolchains/llvm/prebuilt/$(ANDROID_HOST_TAG)
ANDROID_CLANG=$(ANDROID_PREBUILT)/bin/clang
CMAKE_ANDROID_FLAGS=-Wno-dev --no-warn-unused-cli


ANDROID_NATIVE_APP_GLUE_DIR=$(ANDROID_NDK)/sources/android/native_app_glue
ANDROID_NATIVE_APP_GLUE_SRC=$(ANDROID_NATIVE_APP_GLUE_DIR)/android_native_app_glue.c
ifeq ($(ANDROID_ABI),armeabi-v7a)
ANDROID_TARGET_TRIPLE=armv7a-linux-androideabi
else ifeq ($(ANDROID_ABI),arm64-v8a)
ANDROID_TARGET_TRIPLE=aarch64-linux-android
else ifeq ($(ANDROID_ABI),x86)
ANDROID_TARGET_TRIPLE=i686-linux-android
else ifeq ($(ANDROID_ABI),x86_64)
ANDROID_TARGET_TRIPLE=x86_64-linux-android
else
$(error Unsupported ANDROID_ABI '$(ANDROID_ABI)'. Supported ABIs: armeabi-v7a arm64-v8a x86 x86_64)
endif

ANDROID_BUILD_TOOLS_VERSION?=$(shell ls -1 $(ANDROID_SDK_ROOT)/build-tools 2>/dev/null | sort -V | tail -n 1)
ANDROID_PLATFORM_VERSION?=$(shell ls -1 $(ANDROID_SDK_ROOT)/platforms 2>/dev/null | sed 's/android-//' | sort -V | tail -n 1)
ANDROID_BUILD_TOOLS=$(ANDROID_SDK_ROOT)/build-tools/$(ANDROID_BUILD_TOOLS_VERSION)
ANDROID_PLATFORM_JAR=$(ANDROID_SDK_ROOT)/platforms/android-$(ANDROID_PLATFORM_VERSION)/android.jar

AAPT=$(ANDROID_BUILD_TOOLS)/aapt
AAPT2=$(ANDROID_BUILD_TOOLS)/aapt2
ZIPALIGN=$(ANDROID_BUILD_TOOLS)/zipalign
APKSIGNER=$(ANDROID_BUILD_TOOLS)/apksigner
KEYTOOL?=keytool
JAVAC?=javac
JARSIGNER?=jarsigner
D8=$(ANDROID_BUILD_TOOLS)/d8
# bundletool is required for AAB builds; download from
# https://github.com/google/bundletool/releases and place as bundletool.jar
BUNDLETOOL_JAR?=bundletool.jar

ANDROID_JAVA_SRC_DIR=$(ANDROID_BUILD_DIR)/java
ANDROID_CLASSES_DIR=$(ANDROID_BUILD_DIR)/classes

ANDROID_DEBUG_KEYSTORE=$(ANDROID_BUILD_DIR)/debug.keystore
ANDROID_DEBUG_ALIAS=androiddebugkey
ANDROID_DEBUG_STOREPASS=android

#
# Play Store / release signing
# Copy keystore.properties.template → keystore.properties and fill in real values.
# keystore.properties and *.keystore are gitignored – NEVER commit them.
#
-include keystore.properties

ANDROID_VERSION_CODE?=1
ANDROID_VERSION_NAME?=1.0.0
ANDROID_JAVA_RELEASE?=11

ANDROID_RELEASE_KEYSTORE?=android/release.keystore
ANDROID_RELEASE_ALIAS?=CHANGE_ME_KEY_ALIAS
ANDROID_RELEASE_STOREPASS?=CHANGE_ME_STORE_PASSWORD
ANDROID_RELEASE_KEYPASS?=CHANGE_ME_KEY_PASSWORD

ANDROID_RELEASE_UNSIGNED_APK=$(ANDROID_APK_DIR)/BlockBlaster-release-unsigned.apk
ANDROID_RELEASE_UNALIGNED_APK=$(ANDROID_APK_DIR)/BlockBlaster-release-unaligned.apk
ANDROID_RELEASE_APK=$(ANDROID_APK_DIR)/BlockBlaster-release.apk
ANDROID_FINAL_RELEASE_APK?=BlockBlaster-release.apk

# AAB (Android App Bundle) paths – for Play Store submission
ANDROID_AAB_BUILD_DIR=$(ANDROID_BUILD_DIR)/aab
ANDROID_AAB_BASE_DIR=$(ANDROID_AAB_BUILD_DIR)/base
ANDROID_AAB_PROTO_DIR=$(ANDROID_AAB_BUILD_DIR)/linked-proto
ANDROID_UNSIGNED_AAB=$(ANDROID_AAB_BUILD_DIR)/BlockBlaster-unsigned.aab
ANDROID_RELEASE_AAB=$(ANDROID_AAB_BUILD_DIR)/BlockBlaster-release.aab
ANDROID_FINAL_RELEASE_AAB?=BlockBlaster-release.aab

# Emscripten SDK auto-detection (mirrors Android SDK auto-loading).
# If EMSDK is already set in the environment (i.e. emsdk_env.sh was sourced),
# its value is kept.  Otherwise the well-known default install path is used.
EMSDK_DIR?=$(HOME)/emsdk
ifeq ($(EMSDK),)
EMSDK:=$(EMSDK_DIR)
endif
EMCC?=$(EMSDK)/upstream/emscripten/emcc
EMCMAKE?=$(EMSDK)/upstream/emscripten/emcmake

# Flags passed during both compilation and linking.
# USE_SDL=2 tells Emscripten to use its built-in SDL2 backend (needed by
# Allegro's Emscripten port).  USE_FREETYPE / USE_LIBJPEG / USE_LIBPNG
# pull in the Emscripten-provided system ports.
WASM_COMPILE_FLAGS = -s USE_FREETYPE=1 \
                     -s USE_LIBJPEG=1 \
                     -s USE_SDL=2 \
                     -s USE_LIBPNG=1 \
                     -O3

# Linker-only settings -- must NOT be passed during compilation.
# ASYNCIFY is required because Allegro's main loop calls al_wait_for_event()
# which needs to yield back to the browser.
# INITIAL_MEMORY is set to ~2 GB to accommodate large grid sizes.
# FILESYSTEM=1 enables Emscripten's virtual filesystem (needed by IDBFS saves).
WASM_LINK_FLAGS = -s FULL_ES3=1 \
                  -s ASYNCIFY \
                  -s INITIAL_MEMORY=2147418112 \
                  -s STACK_SIZE=2097152 \
                  -s FILESYSTEM=1
# Debug options (uncomment to enable):
#            -s SAFE_HEAP=1 \
#            -s ASSERTIONS=2 \
#            -O0 \
#            -gsource-map

WASM_CFLAGS=$(WASM_COMPILE_FLAGS) \
            -Isrc \
            -I$(DEPS_PREFIX)/include \
            -I$(ALLEGRO_DIR)/include \
            -I$(ALLEGRO_BUILD_DIR)/include \
            -I$(ALLEGRO_DIR)/addons/font \
            -I$(ALLEGRO_DIR)/addons/ttf \
            -I$(ALLEGRO_DIR)/addons/primitives \
            -I$(ALLEGRO_DIR)/addons/image \
            -I$(ALLEGRO_DIR)/addons/audio \
            -I$(ALLEGRO_DIR)/addons/acodec \
            -I$(ALLEGRO_DIR)/addons/color \
            -I$(ALLEGRO_DIR)/addons/native_dialog \
            -DALLEGRO_UNSTABLE

WASM_LDFLAGS=$(WASM_COMPILE_FLAGS) $(WASM_LINK_FLAGS) --preload-file DATA \
             -s EXPORTED_RUNTIME_METHODS='["ccall","cwrap"]' \
             $(DEPS_PREFIX)/lib/libvorbisfile.a \
             $(DEPS_PREFIX)/lib/libvorbisenc.a \
             $(DEPS_PREFIX)/lib/libvorbis.a \
             $(DEPS_PREFIX)/lib/libogg.a \
             $(ALLEGRO_BUILD_DIR)/lib/liballegro_monolith-static.a \
             -lidbfs.js

wasm-deps:
	@echo "Ensuring wasm deps are present (git clone if missing)..."
	@if [ ! -d "$(LIBOGG_DIR)" ]; then \
		echo "Cloning $(LIBOGG_DIR)..."; \
		git clone --depth 1 "$(LIBOGG_REPO)" "$(LIBOGG_DIR)"; \
	fi
	@if [ ! -d "$(LIBVORBIS_DIR)" ]; then \
		echo "Cloning $(LIBVORBIS_DIR)..."; \
		git clone --depth 1 "$(LIBVORBIS_REPO)" "$(LIBVORBIS_DIR)"; \
	fi
	@if [ ! -d "$(ALLEGRO_DIR)" ]; then \
		echo "Cloning $(ALLEGRO_DIR)..."; \
		git clone --depth 1 "$(ALLEGRO_REPO)" "$(ALLEGRO_DIR)"; \
	fi
	@if [ ! -d "$(LIBFREETYPE_DIR)" ]; then \
		echo "Cloning $(LIBFREETYPE_DIR)..."; \
		git clone --depth 1 "$(LIBFREETYPE_REPO)" "$(LIBFREETYPE_DIR)"; \
	fi

wasm-setup:
	@echo "Setting up WebAssembly build environment..."
	@if [ ! -x "$(EMCC)" ]; then \
		echo "ERROR: emcc not found at $(EMCC)"; \
		echo "Either install Emscripten SDK at $(EMSDK_DIR), set EMSDK_DIR=<path>,"; \
		echo "or source /path/to/emsdk/emsdk_env.sh before running make."; \
		exit 1; \
	fi
	@echo "Emscripten SDK: $(EMSDK)"
	@echo "emcc:           $(EMCC)"
	@mkdir -p $(WASM_BUILD_DIR)
	@mkdir -p $(DEPS_PREFIX)
	@echo "Build directory created: $(WASM_BUILD_DIR)"

wasm-libogg: wasm-setup wasm-deps
	@echo "Building libogg (WASM)..."
	@mkdir -p $(LIBOGG_BUILD_DIR)
	@cd $(LIBOGG_BUILD_DIR) && \
		$(EMCMAKE) cmake .. \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(DEPS_PREFIX))" \
			-DCMAKE_C_FLAGS="$(WASM_COMPILE_FLAGS)" && \
		make -j4 && \
		make install
	@echo "libogg build complete."

wasm-libvorbis: wasm-libogg
	@echo "Building libvorbis (WASM)..."
	@mkdir -p $(LIBVORBIS_BUILD_DIR)
	@cd $(LIBVORBIS_BUILD_DIR) && \
		$(EMCMAKE) cmake .. \
			-DCMAKE_BUILD_TYPE=Release \
			-DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(DEPS_PREFIX))" \
			-DCMAKE_PREFIX_PATH="$(abspath $(DEPS_PREFIX))" \
			-DOGG_INCLUDE_DIR="$(abspath $(DEPS_PREFIX))/include" \
			-DOGG_LIBRARY="$(abspath $(DEPS_PREFIX))/lib/libogg.a" \
			-DCMAKE_C_FLAGS="$(WASM_COMPILE_FLAGS)" && \
		make -j4 && \
		make install
	@echo "libvorbis build complete."


wasm-allegro: wasm-libvorbis
	@echo "Building Allegro5 with Emscripten..."
	@if [ ! -d "$(ALLEGRO_DIR)" ]; then \
		echo "ERROR: $(ALLEGRO_DIR) directory not found!"; \
		exit 1; \
	fi
	@mkdir -p $(ALLEGRO_BUILD_DIR)
	@cd $(ALLEGRO_BUILD_DIR) && \
	if [ -z "${EM_CACHE}" ]; then \
		EM_CACHE=${EMSDK}/upstream/emscripten/cache ; \
	fi && \
	$(EMCMAKE) cmake .. \
		-DCMAKE_BUILD_TYPE=Release \
		-DALLEGRO_SDL=ON \
		-DSHARED=OFF \
		-DWANT_MONOLITH=ON \
		-DWANT_ALLOW_SSE=OFF \
		-DWANT_DOCS=OFF \
		-DWANT_TESTS=OFF \
		-DWANT_EXAMPLES=OFF \
		-DWANT_DEMO=OFF \
		-DWANT_OPENAL=OFF \
		-DALLEGRO_WAIT_EVENT_SLEEP=ON \
		-DCMAKE_PREFIX_PATH="$(abspath $(DEPS_PREFIX))" \
		-DOGG_INCLUDE_DIR="$(abspath $(DEPS_PREFIX))/include" \
		-DOGG_LIBRARY="$(abspath $(DEPS_PREFIX))/lib/libogg.a" \
		-DVORBIS_INCLUDE_DIR="$(abspath $(DEPS_PREFIX))/include" \
		-DVORBIS_LIBRARY="$(abspath $(DEPS_PREFIX))/lib/libvorbis.a" \
		-DVORBISFILE_LIBRARY="$(abspath $(DEPS_PREFIX))/lib/libvorbisfile.a" \
		-DSDL2_INCLUDE_DIR=${EM_CACHE}/sysroot/include \
		-DCMAKE_C_FLAGS="$(WASM_COMPILE_FLAGS)" \
		-DCMAKE_CXX_FLAGS="$(WASM_COMPILE_FLAGS)" \
		-DCMAKE_EXE_LINKER_FLAGS="$(WASM_COMPILE_FLAGS) $(WASM_LINK_FLAGS)" && make -j4
	@echo "Allegro5 build complete!"

wasm: wasm-allegro
	@echo "Compiling BlockBlaster for WebAssembly..."
	@echo "Note: Make sure you have sourced emsdk_env.sh first!"
	$(EMCC) $(WASM_CFLAGS) \
		$(addprefix src/,$(SRC)) \
		$(WASM_LDFLAGS) \
		-o $(WASM_BUILD_DIR)/BlockBlaster.html \
		--shell-file src/index.html
	@# --- Cache-busting: rename .js/.wasm/.data with content hash ---
	@HASH=$$(md5sum $(WASM_BUILD_DIR)/BlockBlaster.js | cut -c1-8); \
	echo "Cache-bust hash: $$HASH"; \
	rm -f $(WASM_BUILD_DIR)/BlockBlaster.*.js $(WASM_BUILD_DIR)/BlockBlaster.*.wasm $(WASM_BUILD_DIR)/BlockBlaster.*.data; \
	cp $(WASM_BUILD_DIR)/BlockBlaster.js   "$(WASM_BUILD_DIR)/BlockBlaster.$$HASH.js"; \
	cp $(WASM_BUILD_DIR)/BlockBlaster.wasm "$(WASM_BUILD_DIR)/BlockBlaster.$$HASH.wasm"; \
	if [ -f $(WASM_BUILD_DIR)/BlockBlaster.data ]; then \
		cp $(WASM_BUILD_DIR)/BlockBlaster.data "$(WASM_BUILD_DIR)/BlockBlaster.$$HASH.data"; \
	fi; \
	sed -i "s|BlockBlaster\.wasm|BlockBlaster.$$HASH.wasm|g" "$(WASM_BUILD_DIR)/BlockBlaster.$$HASH.js"; \
	sed -i "s|BlockBlaster\.data|BlockBlaster.$$HASH.data|g" "$(WASM_BUILD_DIR)/BlockBlaster.$$HASH.js"; \
	sed -i "s|BlockBlaster\.js|BlockBlaster.$$HASH.js|g" $(WASM_BUILD_DIR)/BlockBlaster.html
	@echo ""
	@echo "Build complete! Files generated in $(WASM_BUILD_DIR)/"
	@echo "To test, run from $(WASM_BUILD_DIR):"
	@echo "  python3 -m http.server"
	@echo "Then open http://localhost:8000/BlockBlaster.html in your browser"

wasm-clean:
	@echo "Cleaning WebAssembly build..."
	$(RM) -r $(WASM_BUILD_DIR)
	$(RM) -r $(LIBOGG_BUILD_DIR)
	$(RM) -r $(LIBVORBIS_BUILD_DIR)
	$(RM) -r $(ALLEGRO_BUILD_DIR)


# ==========================================================================
# Android build
# ==========================================================================
#
# The Android build cross-compiles the same SRC list into a shared library
# (libblockblaster.so) for each requested ABI using the Android NDK clang,
# then packages everything into a debug or release APK using aapt/d8/apksigner.
# An AAB (Android App Bundle) target is also provided for Play Store submission.
#

android-setup:
	@echo "Setting up Android build environment..."
	@if [ ! -d "$(ANDROID_SDK_ROOT)" ]; then \
		echo "ERROR: ANDROID_SDK_ROOT does not exist: $(ANDROID_SDK_ROOT)"; \
		exit 1; \
	fi
	@if [ ! -d "$(ANDROID_NDK)" ]; then \
		echo "ERROR: Android NDK not found in $(ANDROID_SDK_ROOT)/ndk or ndk-bundle"; \
		exit 1; \
	fi
	@if [ ! -f "$(ANDROID_TOOLCHAIN_FILE)" ]; then \
		echo "ERROR: Missing Android CMake toolchain file: $(ANDROID_TOOLCHAIN_FILE)"; \
		exit 1; \
	fi
	@if [ ! -x "$(ANDROID_CLANG)" ]; then \
		echo "ERROR: Android clang not found at $(ANDROID_CLANG)"; \
		echo "Set ANDROID_HOST_TAG (e.g. linux-x86_64, darwin-x86_64, windows-x86_64)"; \
		exit 1; \
	fi
	@if [ ! -x "$(AAPT)" ]; then \
		echo "ERROR: aapt not found at $(AAPT)"; \
		echo "Install Android build-tools and/or set ANDROID_BUILD_TOOLS_VERSION"; \
		exit 1; \
	fi
	@if [ ! -f "$(ANDROID_PLATFORM_JAR)" ]; then \
		echo "ERROR: android.jar not found at $(ANDROID_PLATFORM_JAR)"; \
		echo "Install Android platform and/or set ANDROID_PLATFORM_VERSION"; \
		exit 1; \
	fi
	@mkdir -p $(ANDROID_BUILD_DIR) $(ANDROID_DEPS_PREFIX) $(ANDROID_NATIVE_LIB_DIR) $(ANDROID_APK_DIR)
	@echo "Android SDK: $(ANDROID_SDK_ROOT)"
	@echo "Android NDK: $(ANDROID_NDK)"
	@echo "Build-tools: $(ANDROID_BUILD_TOOLS_VERSION)"
	@echo "Platform: android-$(ANDROID_PLATFORM_VERSION)"

android-libogg: android-setup wasm-deps
	@echo "Building libogg (Android $(ANDROID_ABI))..."
	@mkdir -p $(ANDROID_LIBOGG_BUILD_DIR)
	@cd $(ANDROID_LIBOGG_BUILD_DIR) && \
		cmake .. $(CMAKE_ANDROID_FLAGS) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_TOOLCHAIN_FILE="$(abspath $(ANDROID_TOOLCHAIN_FILE))" \
			-DANDROID_ABI=$(ANDROID_ABI) \
			-DANDROID_PLATFORM=android-$(ANDROID_API) \
			-DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(ANDROID_DEPS_PREFIX))" && \
		make -j4 && \
		make install

android-libvorbis: android-libogg
	@echo "Building libvorbis (Android $(ANDROID_ABI))..."
	@mkdir -p $(ANDROID_LIBVORBIS_BUILD_DIR)
	@cd $(ANDROID_LIBVORBIS_BUILD_DIR) && \
		cmake .. $(CMAKE_ANDROID_FLAGS) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_TOOLCHAIN_FILE="$(abspath $(ANDROID_TOOLCHAIN_FILE))" \
			-DANDROID_ABI=$(ANDROID_ABI) \
			-DANDROID_PLATFORM=android-$(ANDROID_API) \
			-DBUILD_SHARED_LIBS=OFF \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(ANDROID_DEPS_PREFIX))" \
			-DCMAKE_PREFIX_PATH="$(abspath $(ANDROID_DEPS_PREFIX))" \
			-DOGG_INCLUDE_DIR="$(abspath $(ANDROID_DEPS_PREFIX))/include" \
			-DOGG_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libogg.a" && \
		make -j4 && \
		make install

android-libfreetype: android-libogg
	@echo "Building FreeType (Android $(ANDROID_ABI))..."
	@mkdir -p $(ANDROID_LIBFREETYPE_BUILD_DIR)
	@cd $(ANDROID_LIBFREETYPE_BUILD_DIR) && \
		cmake .. $(CMAKE_ANDROID_FLAGS) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_TOOLCHAIN_FILE="$(abspath $(ANDROID_TOOLCHAIN_FILE))" \
			-DANDROID_ABI=$(ANDROID_ABI) \
			-DANDROID_PLATFORM=android-$(ANDROID_API) \
			-DBUILD_SHARED_LIBS=OFF \
			-DFT_DISABLE_ZLIB=OFF \
			-DFT_DISABLE_PNG=ON \
			-DFT_DISABLE_BZIP2=ON \
			-DFT_DISABLE_HARFBUZZ=ON \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(ANDROID_DEPS_PREFIX))" && \
		make -j4 && \
		make install

android-allegro: android-libvorbis android-libfreetype
	@echo "Building Allegro5 (Android $(ANDROID_ABI))..."
	@mkdir -p $(ANDROID_ALLEGRO_BUILD_DIR)
	@cd $(ANDROID_ALLEGRO_BUILD_DIR) && \
		cmake .. $(CMAKE_ANDROID_FLAGS) \
			-DCMAKE_BUILD_TYPE=Release \
			-DCMAKE_TOOLCHAIN_FILE="$(abspath $(ANDROID_TOOLCHAIN_FILE))" \
			-DANDROID_ABI=$(ANDROID_ABI) \
			-DANDROID_PLATFORM=android-$(ANDROID_API) \
			-DSHARED=OFF \
			-DWANT_MONOLITH=ON \
			-DWANT_DOCS=OFF \
			-DWANT_TESTS=OFF \
			-DWANT_EXAMPLES=OFF \
			-DWANT_DEMO=OFF \
			-DWANT_TOOLS=OFF \
			-DWANT_IMAGE=ON \
			-DWANT_FONT=ON \
			-DWANT_TTF=ON \
			-DCMAKE_INSTALL_PREFIX="$(abspath $(ANDROID_DEPS_PREFIX))" \
			-DCMAKE_PREFIX_PATH="$(abspath $(ANDROID_DEPS_PREFIX))" \
			-DOGG_INCLUDE_DIR="$(abspath $(ANDROID_DEPS_PREFIX))/include" \
			-DOGG_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libogg.a" \
			-DVORBIS_INCLUDE_DIR="$(abspath $(ANDROID_DEPS_PREFIX))/include" \
			-DVORBIS_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libvorbis.a" \
			-DVORBISFILE_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libvorbisfile.a" \
			-DVORBISENC_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libvorbisenc.a" \
			-DFREETYPE_INCLUDE_DIR_ft2build="$(abspath $(ANDROID_DEPS_PREFIX))/include/freetype2" \
			-DFREETYPE_INCLUDE_DIR_freetype2="$(abspath $(ANDROID_DEPS_PREFIX))/include/freetype2" \
			-DFREETYPE_INCLUDE_DIRS="$(abspath $(ANDROID_DEPS_PREFIX))/include/freetype2" \
			-DFREETYPE_LIBRARY="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libfreetype.a" \
			-DFREETYPE_LIBRARIES="$(abspath $(ANDROID_DEPS_PREFIX))/lib/libfreetype.a" \
			-DFREETYPE_FOUND=TRUE \
			-DTTF_COMPILES=TRUE \
			-DTTF_COMPILES_WITH_EXTRA_DEPS=TRUE
	@# Patch the Allegro5-generated Gradle project to suppress build warnings:
	@# 1. gradle.properties: silence Java 8 source/target deprecation reported by AGP
	@# 2. build.gradle:       bump compileOptions from VERSION_1_8 to VERSION_11 (proper fix)
	@# 3. build.gradle:       remove flatDir repository block and use direct file deps instead
	@GRADLE_DIR="$(ANDROID_ALLEGRO_BUILD_DIR)/android/gradle_project"; \
	if [ -d "$$GRADLE_DIR" ]; then \
		echo "Patching Allegro5 Gradle project to fix build warnings..."; \
		PROPS="$$GRADLE_DIR/gradle.properties"; \
		grep -q "suppressSourceTargetDeprecationWarning" "$$PROPS" 2>/dev/null \
			|| echo "android.javaCompile.suppressSourceTargetDeprecationWarning=true" >> "$$PROPS"; \
		for BGFILE in "$$GRADLE_DIR/allegro/build.gradle" "$$GRADLE_DIR/build.gradle"; do \
			if [ -f "$$BGFILE" ]; then \
				sed -i 's/VERSION_1_8/VERSION_11/g' "$$BGFILE"; \
				python3 -c "import re,sys;c=open(sys.argv[1]).read();c=re.sub(r'\n[ \t]*flatDir[ \t]*\{[^}]*\}','',c);c=re.sub(r'\n[ \t]*repositories[ \t]*\{[ \t]*\n[ \t]*\}','',c);c=re.sub(r\"implementation\(name: '([^']+)', ext: 'aar'\)\",r\"implementation files('libs/\1.aar')\",c);open(sys.argv[1],'w').write(c)" "$$BGFILE"; \
			fi; \
		done; \
	fi
	@cd $(ANDROID_ALLEGRO_BUILD_DIR) && make -j4 && make install

android-native: android-allegro
	@echo "Compiling native shared library (Android $(ANDROID_ABI))..."
	@mkdir -p $(ANDROID_NATIVE_LIB_DIR)
	@if [ ! -f "$(ANDROID_ALLEGRO_MONOLITH_LIB)" ]; then \
		echo "ERROR: Missing Allegro monolith static library: $(ANDROID_ALLEGRO_MONOLITH_LIB)"; \
		exit 1; \
	fi
	$(ANDROID_CLANG) --target=$(ANDROID_TARGET_TRIPLE)$(ANDROID_API) \
		-fPIC -shared -O3 -std=gnu99 -DALLEGRO_UNSTABLE \
		-L$(ANDROID_DEPS_PREFIX)/lib \
		-Isrc \
		-I$(ANDROID_DEPS_PREFIX)/include \
		-I$(ANDROID_DEPS_PREFIX)/include/freetype2 \
		-I$(ALLEGRO_DIR)/include \
		-I$(ANDROID_ALLEGRO_BUILD_DIR)/include \
		-I$(ALLEGRO_DIR)/addons/font \
		-I$(ALLEGRO_DIR)/addons/ttf \
		-I$(ALLEGRO_DIR)/addons/primitives \
		-I$(ALLEGRO_DIR)/addons/image \
		-I$(ALLEGRO_DIR)/addons/audio \
		-I$(ALLEGRO_DIR)/addons/acodec \
		-I$(ALLEGRO_DIR)/addons/color \
		-I$(ALLEGRO_DIR)/addons/native_dialog \
		$(addprefix src/,$(SRC)) \
		-o $(ANDROID_NATIVE_LIB_DIR)/libblockblaster.so \
		-Wl,--whole-archive \
		$(ANDROID_ALLEGRO_MONOLITH_LIB) \
		-Wl,--no-whole-archive \
		$(ANDROID_DEPS_PREFIX)/lib/libvorbisfile.a \
		$(ANDROID_DEPS_PREFIX)/lib/libvorbisenc.a \
		$(ANDROID_DEPS_PREFIX)/lib/libvorbis.a \
		$(ANDROID_DEPS_PREFIX)/lib/libogg.a \
		$(ANDROID_DEPS_PREFIX)/lib/libfreetype.a \
		-landroid -llog -lOpenSLES -lEGL -lGLESv2 -lm -lz -llog

android-native-all:
	@for abi in $(ANDROID_ABIS); do \
		echo "Building native library for $$abi..."; \
		$(MAKE) android-native ANDROID_ABI=$$abi; \
	done

android-dex: android-setup wasm-deps
	@echo "Compiling Allegro Java classes + stub activity into classes.dex..."
	@mkdir -p $(ANDROID_JAVA_SRC_DIR)/$(subst .,/,$(ANDROID_PACKAGE)) $(ANDROID_CLASSES_DIR) $(ANDROID_APK_DIR)
	@printf 'package $(ANDROID_PACKAGE);\nimport org.liballeg.android.AllegroActivity;\npublic class $(ANDROID_ACTIVITY) extends AllegroActivity {\n  public $(ANDROID_ACTIVITY)() { super("libblockblaster.so"); }\n}\n' \
		> $(ANDROID_JAVA_SRC_DIR)/$(subst .,/,$(ANDROID_PACKAGE))/$(ANDROID_ACTIVITY).java
	@ALLEGRO_JAVA=$$(find $(ALLEGRO_DIR)/android -name "*.java" 2>/dev/null); \
	if [ -z "$$ALLEGRO_JAVA" ]; then \
		echo "ERROR: No Allegro Java sources found under $(ALLEGRO_DIR)/android"; \
		exit 1; \
	fi; \
	$(JAVAC) --release $(ANDROID_JAVA_RELEASE) \
		-classpath $(ANDROID_PLATFORM_JAR) \
		-d $(ANDROID_CLASSES_DIR) \
		$$ALLEGRO_JAVA \
		$(ANDROID_JAVA_SRC_DIR)/$(subst .,/,$(ANDROID_PACKAGE))/$(ANDROID_ACTIVITY).java
	@$(D8) --release --lib $(ANDROID_PLATFORM_JAR) --min-api $(ANDROID_API) --output $(ANDROID_APK_DIR) \
		$$(find $(ANDROID_CLASSES_DIR) -name "*.class")

android-keystore: android-setup
	@if [ ! -f "$(ANDROID_DEBUG_KEYSTORE)" ]; then \
		echo "Generating debug keystore..."; \
		$(KEYTOOL) -genkey -v -keystore "$(ANDROID_DEBUG_KEYSTORE)" \
			-storepass "$(ANDROID_DEBUG_STOREPASS)" \
			-alias "$(ANDROID_DEBUG_ALIAS)" \
			-keypass "$(ANDROID_DEBUG_STOREPASS)" \
			-keyalg RSA -keysize 2048 -validity 10000 \
			-dname "CN=Android Debug,O=Android,C=US"; \
	fi

android: android-native-all android-dex android-icons android-keystore
	@echo "Packaging APK..."
	@mkdir -p $(ANDROID_BUILD_DIR)/lib
	@for abi in $(ANDROID_ABIS); do \
		mkdir -p $(ANDROID_BUILD_DIR)/lib/$$abi; \
		cp $(ANDROID_BUILD_DIR)/native/$$abi/libblockblaster.so $(ANDROID_BUILD_DIR)/lib/$$abi/libblockblaster.so; \
	done
	$(AAPT) package -f \
		--version-code $(ANDROID_VERSION_CODE) \
		--version-name $(ANDROID_VERSION_NAME) \
		-M $(ANDROID_MANIFEST) \
		-S $(ANDROID_RES_DIR) \
		-I $(ANDROID_PLATFORM_JAR) \
		-A DATA \
		-F $(ANDROID_UNSIGNED_APK)
	@cp $(ANDROID_UNSIGNED_APK) $(ANDROID_UNALIGNED_APK)
	@cd $(ANDROID_APK_DIR) && \
		$(abspath $(AAPT)) add "$(abspath $(ANDROID_UNALIGNED_APK))" classes.dex
	@cd $(ANDROID_BUILD_DIR) && \
		for abi in $(ANDROID_ABIS); do \
			$(abspath $(AAPT)) add "$(abspath $(ANDROID_UNALIGNED_APK))" lib/$$abi/libblockblaster.so; \
		done
	$(ZIPALIGN) -f 4 $(ANDROID_UNALIGNED_APK) $(ANDROID_APK)
	$(APKSIGNER) sign --ks $(ANDROID_DEBUG_KEYSTORE) \
		--ks-key-alias $(ANDROID_DEBUG_ALIAS) \
		--ks-pass pass:$(ANDROID_DEBUG_STOREPASS) \
		--key-pass pass:$(ANDROID_DEBUG_STOREPASS) \
		$(ANDROID_APK)
	@cp $(ANDROID_APK) $(ANDROID_FINAL_APK)
	@echo "APK generated: $(ANDROID_APK)"
	@echo "APK copied to: $(ANDROID_FINAL_APK)"

android-apk-path:
	@echo $(ANDROID_FINAL_APK)

android-icons:
	@echo "Checking launcher icons..."
	@missing=0; \
	for res in mipmap-mdpi mipmap-hdpi mipmap-xhdpi mipmap-xxhdpi mipmap-xxxhdpi; do \
		if [ ! -f "android/res/$$res/ic_launcher.png" ]; then \
			echo "  Missing: android/res/$$res/ic_launcher.png"; \
			missing=1; \
		fi; \
	done; \
	if [ $$missing -eq 1 ]; then \
		echo ""; \
		echo "Run 'make android-gen-icons' to generate placeholder icons (requires ImageMagick),"; \
		echo "or place your own 512x512 source icon and resize it into android/res/mipmap-*/"; \
		exit 1; \
	fi
	@echo "Launcher icons: OK"

# Generates simple placeholder launcher icons using ImageMagick.
# Replace with real artwork before submitting to the Play Store.
# Sizes: mdpi=48, hdpi=72, xhdpi=96, xxhdpi=144, xxxhdpi=192 (px)
android-gen-icons:
	@echo "Generating placeholder launcher icons (requires ImageMagick 'convert')..."
	@command -v convert >/dev/null 2>&1 || { \
		echo "ERROR: ImageMagick 'convert' not found."; \
		echo "Install with: sudo apt install imagemagick   (Debian/Ubuntu)"; \
		exit 1; \
	}
	@mkdir -p android/res/mipmap-mdpi android/res/mipmap-hdpi \
		android/res/mipmap-xhdpi android/res/mipmap-xxhdpi android/res/mipmap-xxxhdpi
	convert -size 48x48   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 12 -annotate 0 'BB' android/res/mipmap-mdpi/ic_launcher.png
	convert -size 48x48   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 12 -annotate 0 'BB' android/res/mipmap-mdpi/ic_launcher_round.png
	convert -size 72x72   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 18 -annotate 0 'BB' android/res/mipmap-hdpi/ic_launcher.png
	convert -size 72x72   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 18 -annotate 0 'BB' android/res/mipmap-hdpi/ic_launcher_round.png
	convert -size 96x96   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 24 -annotate 0 'BB' android/res/mipmap-xhdpi/ic_launcher.png
	convert -size 96x96   gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 24 -annotate 0 'BB' android/res/mipmap-xhdpi/ic_launcher_round.png
	convert -size 144x144 gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 36 -annotate 0 'BB' android/res/mipmap-xxhdpi/ic_launcher.png
	convert -size 144x144 gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 36 -annotate 0 'BB' android/res/mipmap-xxhdpi/ic_launcher_round.png
	convert -size 192x192 gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 48 -annotate 0 'BB' android/res/mipmap-xxxhdpi/ic_launcher.png
	convert -size 192x192 gradient:'#1565C0-#0D47A1' -fill white -gravity Center -pointsize 48 -annotate 0 'BB' android/res/mipmap-xxxhdpi/ic_launcher_round.png
	@echo "Placeholder icons generated in android/res/mipmap-*/"
	@echo "Replace with real artwork before submitting to the Play Store."

android-release-keystore: android-setup
	@echo "Verifying release keystore..."
	@if [ ! -f "$(ANDROID_RELEASE_KEYSTORE)" ]; then \
		echo "ERROR: Release keystore not found: $(ANDROID_RELEASE_KEYSTORE)"; \
		echo ""; \
		echo "Generate one with:"; \
		echo "  $(KEYTOOL) -genkey -v \\"; \
		echo "    -keystore $(ANDROID_RELEASE_KEYSTORE) \\"; \
		echo "    -alias YOUR_ALIAS \\"; \
		echo "    -keyalg RSA -keysize 4096 -validity 10000"; \
		echo ""; \
		echo "Then copy keystore.properties.template to keystore.properties and fill in the values."; \
		exit 1; \
	fi
	@if [ "$(ANDROID_RELEASE_ALIAS)" = "CHANGE_ME_KEY_ALIAS" ] || \
	   [ "$(ANDROID_RELEASE_STOREPASS)" = "CHANGE_ME_STORE_PASSWORD" ] || \
	   [ "$(ANDROID_RELEASE_KEYPASS)" = "CHANGE_ME_KEY_PASSWORD" ]; then \
		echo "ERROR: Release signing credentials are still set to placeholder values."; \
		echo "Copy keystore.properties.template to keystore.properties and fill in your values."; \
		exit 1; \
	fi
	@echo "Release keystore: OK"

android-release: android-native-all android-dex android-icons android-release-keystore
	@echo "Packaging release APK (versionCode=$(ANDROID_VERSION_CODE) versionName=$(ANDROID_VERSION_NAME))..."
	@mkdir -p $(ANDROID_BUILD_DIR)/lib
	@for abi in $(ANDROID_ABIS); do \
		mkdir -p $(ANDROID_BUILD_DIR)/lib/$$abi; \
		cp $(ANDROID_BUILD_DIR)/native/$$abi/libblockblaster.so $(ANDROID_BUILD_DIR)/lib/$$abi/libblockblaster.so; \
	done
	$(AAPT) package -f \
		--version-code $(ANDROID_VERSION_CODE) \
		--version-name $(ANDROID_VERSION_NAME) \
		-M $(ANDROID_MANIFEST) \
		-S $(ANDROID_RES_DIR) \
		-I $(ANDROID_PLATFORM_JAR) \
		-A DATA \
		-F $(ANDROID_RELEASE_UNSIGNED_APK)
	@cp $(ANDROID_RELEASE_UNSIGNED_APK) $(ANDROID_RELEASE_UNALIGNED_APK)
	@cd $(ANDROID_APK_DIR) && \
		$(abspath $(AAPT)) add "$(abspath $(ANDROID_RELEASE_UNALIGNED_APK))" classes.dex
	@cd $(ANDROID_BUILD_DIR) && \
		for abi in $(ANDROID_ABIS); do \
			$(abspath $(AAPT)) add "$(abspath $(ANDROID_RELEASE_UNALIGNED_APK))" lib/$$abi/libblockblaster.so; \
		done
	$(ZIPALIGN) -f 4 $(ANDROID_RELEASE_UNALIGNED_APK) $(ANDROID_RELEASE_APK)
	$(APKSIGNER) sign \
		--ks $(ANDROID_RELEASE_KEYSTORE) \
		--ks-key-alias $(ANDROID_RELEASE_ALIAS) \
		--ks-pass pass:$(ANDROID_RELEASE_STOREPASS) \
		--key-pass pass:$(ANDROID_RELEASE_KEYPASS) \
		$(ANDROID_RELEASE_APK)
	@cp $(ANDROID_RELEASE_APK) $(ANDROID_FINAL_RELEASE_APK)
	@echo ""
	@echo "Release APK: $(ANDROID_RELEASE_APK)"
	@echo "Copied to:   $(ANDROID_FINAL_RELEASE_APK)"
	@echo "Use 'make android-aab' to produce a Play Store AAB instead."

# android-aab: produces a signed Android App Bundle (AAB) for Play Store submission.
#
# Google Play requires AAB format for new apps (since August 2021).
# Unlike an APK, an AAB is not directly installable; Google Play re-packages it
# into device-optimised APKs (right ABI, screen density, language) at distribution
# time, resulting in smaller downloads for end-users.
#
# Extra prerequisites beyond android-release:
#   • aapt2  – part of Android SDK build-tools (same directory as aapt)
#   • bundletool.jar – standalone Google tool; download from
#       https://github.com/google/bundletool/releases
#     Place as bundletool.jar in the project root or set BUNDLETOOL_JAR=<path>.
#   • jarsigner – part of any JDK installation
#
# Build flow:
#   1. aapt2 compile  → compile resources into binary flat files
#   2. aapt2 link --proto-format → link into proto-format "APK" (manifest + resources.pb)
#   3. Assemble base/ module tree (manifest, dex, lib, assets, res, resources.pb)
#   4. bundletool build-bundle → unsigned .aab
#   5. jarsigner → signed .aab  (jarsigner is used for AABs; apksigner is APK-only)
android-aab: android-native-all android-dex android-icons android-release-keystore
	@echo "Building Android App Bundle (AAB) for Play Store..."
	@echo "  versionCode=$(ANDROID_VERSION_CODE)  versionName=$(ANDROID_VERSION_NAME)"
	@if [ ! -f "$(BUNDLETOOL_JAR)" ]; then \
		echo "ERROR: bundletool JAR not found: $(BUNDLETOOL_JAR)"; \
		echo "Download from https://github.com/google/bundletool/releases"; \
		echo "and place it as bundletool.jar, or set BUNDLETOOL_JAR=<path>."; \
		exit 1; \
	fi
	@if [ ! -x "$(AAPT2)" ]; then \
		echo "ERROR: aapt2 not found at $(AAPT2)"; \
		echo "Install Android build-tools and/or set ANDROID_BUILD_TOOLS_VERSION"; \
		exit 1; \
	fi
	# Create the base module directory tree
	@$(RM) -r $(ANDROID_AAB_BUILD_DIR)
	@mkdir -p $(ANDROID_AAB_BASE_DIR)/manifest \
	          $(ANDROID_AAB_BASE_DIR)/dex \
	          $(ANDROID_AAB_BASE_DIR)/assets \
	          $(ANDROID_AAB_PROTO_DIR)
	@for abi in $(ANDROID_ABIS); do \
		mkdir -p $(ANDROID_AAB_BASE_DIR)/lib/$$abi; \
		cp $(ANDROID_BUILD_DIR)/native/$$abi/libblockblaster.so \
		   $(ANDROID_AAB_BASE_DIR)/lib/$$abi/libblockblaster.so; \
	done
	@cp -r DATA/. $(ANDROID_AAB_BASE_DIR)/assets/
	@cp $(ANDROID_APK_DIR)/classes.dex $(ANDROID_AAB_BASE_DIR)/dex/classes.dex
	# Compile resources (flat binary format)
	$(AAPT2) compile --dir $(ANDROID_RES_DIR) -o $(ANDROID_AAB_BUILD_DIR)/compiled_res.zip
	# Link resources in proto format  (produces AndroidManifest.xml + resources.pb)
	$(AAPT2) link --proto-format \
		-o $(ANDROID_AAB_PROTO_DIR)/base.apk \
		-I $(ANDROID_PLATFORM_JAR) \
		--manifest $(ANDROID_MANIFEST) \
		--version-code $(ANDROID_VERSION_CODE) \
		--version-name "$(ANDROID_VERSION_NAME)" \
		-R $(ANDROID_AAB_BUILD_DIR)/compiled_res.zip \
		--auto-add-overlay
	# Extract proto manifest, resources.pb, and res/ from the linked proto APK
	cd $(ANDROID_AAB_PROTO_DIR) && unzip -o base.apk
	@cp $(ANDROID_AAB_PROTO_DIR)/AndroidManifest.xml \
	    $(ANDROID_AAB_BASE_DIR)/manifest/AndroidManifest.xml
	@cp $(ANDROID_AAB_PROTO_DIR)/resources.pb \
	    $(ANDROID_AAB_BASE_DIR)/resources.pb
	@if [ -d "$(ANDROID_AAB_PROTO_DIR)/res" ]; then \
		cp -r $(ANDROID_AAB_PROTO_DIR)/res $(ANDROID_AAB_BASE_DIR)/res; \
	fi
	# Pack base module zip (paths are relative to the module root, no 'base/' prefix)
	@cd $(ANDROID_AAB_BASE_DIR) && \
		zip -r ../base.zip manifest/ dex/ lib/ assets/ resources.pb && \
		if [ -d res ]; then zip -r ../base.zip res/; fi
	# Build the unsigned AAB
	java -jar $(BUNDLETOOL_JAR) build-bundle \
		--modules=$(ANDROID_AAB_BUILD_DIR)/base.zip \
		--output=$(ANDROID_UNSIGNED_AAB)
	# Sign the AAB (jarsigner signs in-place; copy first)
	cp $(ANDROID_UNSIGNED_AAB) $(ANDROID_RELEASE_AAB)
	$(JARSIGNER) \
		-keystore $(ANDROID_RELEASE_KEYSTORE) \
		-storepass "$(ANDROID_RELEASE_STOREPASS)" \
		-keypass "$(ANDROID_RELEASE_KEYPASS)" \
		-sigalg SHA256withRSA \
		-digestalg SHA-256 \
		$(ANDROID_RELEASE_AAB) $(ANDROID_RELEASE_ALIAS)
	@cp $(ANDROID_RELEASE_AAB) $(ANDROID_FINAL_RELEASE_AAB)
	@echo ""
	@echo "Release AAB: $(ANDROID_RELEASE_AAB)"
	@echo "Copied to:   $(ANDROID_FINAL_RELEASE_AAB)"
	@echo "Upload $(ANDROID_FINAL_RELEASE_AAB) to Google Play Console."

android-clean:
	@echo "Cleaning Android build..."
	$(RM) -r $(ANDROID_BUILD_DIR)
	$(RM) $(ANDROID_FINAL_APK)
	$(RM) $(ANDROID_FINAL_RELEASE_APK)
	$(RM) $(ANDROID_FINAL_RELEASE_AAB)
	$(RM) -r $(LIBOGG_DIR)/build_android_*
	$(RM) -r $(LIBVORBIS_DIR)/build_android_*
	$(RM) -r $(LIBFREETYPE_DIR)/build_android_*
	$(RM) -r $(ALLEGRO_DIR)/build_android_*

# --------------------------------------------------------------------------
# Cleanup targets
# --------------------------------------------------------------------------

# Remove desktop build artefacts only
clean:
	$(RM) $(OBJDIR)/*.o
	$(RM) BlockBlaster$(EXT)

# Remove all build artefacts: desktop, WASM, and Android
clean-all: clean wasm-clean android-clean

.PHONY: all clean clean-all wasm wasm-setup wasm-deps wasm-libogg wasm-libvorbis wasm-allegro wasm-clean android android-setup android-libogg android-libvorbis android-libfreetype android-allegro android-native android-native-all android-dex android-keystore android-icons android-gen-icons android-release-keystore android-release android-aab android-apk-path android-clean
