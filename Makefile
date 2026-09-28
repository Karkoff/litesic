# =============================================================================
# LiteSic - Makefile
# Player de audio estilo Winamp em C++ nativo + Win32/GDI
# =============================================================================

# Toolchain (MINGW64 por padrao, MINGW32 para ARCH=32)
ifeq ($(ARCH),32)
  CXX     := i686-w64-mingw32-g++
  CC      := i686-w64-mingw32-gcc
  WINDRES := windres.exe
else
  CXX     := g++.exe
  CC      := gcc.exe
  WINDRES := windres.exe
endif

# Add MinGW bin to PATH so tools are found
export PATH := /mingw64/bin:$(PATH)
ifeq ($(ARCH),32)
export PATH := /mingw32/bin:$(PATH)
endif

# Directories
SRC_DIR       := src
THIRD_PARTY   := third_party
RES_DIR       := res
BUILD_DIR     := build
TEST_DIR      := tests

# Output
TARGET        := $(BUILD_DIR)/LiteSic.exe

# Flags comuns (C++)
CXXFLAGS      := -std=c++17 -D_WIN32_WINNT=0x0601 -DUNICODE -D_UNICODE \
                 -Wall -Wextra -I$(SRC_DIR) -I$(THIRD_PARTY)

# Flags comuns (C — miniaudio + stb_vorbis)
CFLAGS        := -D_WIN32_WINNT=0x0601 -DUNICODE -D_UNICODE \
                 -Wall -Wextra -I$(SRC_DIR) -I$(THIRD_PARTY)

# Flags de debug (comuns)
ifeq ($(DEBUG),1)
  CXXFLAGS += -g -O0 -DDEBUG
  CFLAGS   += -g -O0 -DDEBUG
endif

LDFLAGS       := -mwindows -municode \
                  -luser32 -lkernel32 -lgdi32 -lm

# Source files (miniaudio_impl agora .cpp para linkar com libstdc++)
CPP_SRCS      := $(wildcard $(SRC_DIR)/*.cpp)
RC_SRCS       := $(RES_DIR)/app.rc

# Object files
CPP_OBJS      := $(patsubst $(SRC_DIR)/%.cpp,$(BUILD_DIR)/%.o,$(CPP_SRCS))
RC_OBJS       := $(patsubst $(RES_DIR)/%.rc,$(BUILD_DIR)/%.res.o,$(RC_SRCS))

OBJS          := $(CPP_OBJS) $(RC_OBJS)

# Test files
TEST_SRCS     := $(wildcard $(TEST_DIR)/*.c)
TEST_BINS     := $(patsubst $(TEST_DIR)/%.c,$(BUILD_DIR)/test-%,$(TEST_SRCS))

# =============================================================================
# Targets
# =============================================================================

all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(LDFLAGS) -o $@ $(OBJS)

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.o: $(SRC_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c -o $@ $<

$(BUILD_DIR)/%.res.o: $(RES_DIR)/%.rc
	@mkdir -p $(dir $@)
	$(WINDRES) -i $< -o $@

# Clean
clean:
	rm -rf $(BUILD_DIR)

# Tests
test: $(TEST_BINS)
	@echo "Running tests..."
	@for t in $(TEST_BINS); do \
		echo "==> $$t"; \
		$$t && echo "    OK" || (echo "    FAILED" && exit 1); \
	done
	@echo "All tests passed."

$(BUILD_DIR)/test-%: $(TEST_DIR)/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -o $@ $<

.PHONY: all clean test
