# LiteSic: GNU Make dentro do MSYS2 MINGW64/MINGW32 (MSVCRT).
.DEFAULT_GOAL := all
SHELL := /usr/bin/bash
.DELETE_ON_ERROR:

ARCH ?= 64
ifeq ($(ARCH),32)
  TOOLCHAIN := /mingw32/bin
else ifeq ($(ARCH),64)
  TOOLCHAIN := /mingw64/bin
else
  $(error ARCH deve ser 32 ou 64)
endif
CC := $(TOOLCHAIN)/gcc.exe
CXX := $(TOOLCHAIN)/g++.exe
WINDRES := $(TOOLCHAIN)/windres.exe
export PATH := $(TOOLCHAIN):/usr/bin:$(PATH)

ifeq ($(DEBUG),1)
  CONFIG := debug
  OPTFLAGS := -g -O0 -DDEBUG
else
  CONFIG := release
  OPTFLAGS := -O2
endif

# Caminhos fixos por variante impedem mistura de objetos e limpeza arbitraria.
BUILD_DIR := build/$(ARCH)/$(CONFIG)
INTERNAL_DIR := build/internal/$(ARCH)/$(CONFIG)
OBJ_DIR := $(INTERNAL_DIR)/obj
TEST_DIR := $(INTERNAL_DIR)/tests
DIAG_DIR := $(INTERNAL_DIR)/diagnostics
TARGET := $(BUILD_DIR)/LiteSic.exe
CPPFLAGS := -D_WIN32_WINNT=0x0601 -DUNICODE -D_UNICODE -Isrc -Ithird_party
CFLAGS := -std=c17 -Wall -Wextra $(OPTFLAGS)
CXXFLAGS := -std=c++17 -Wall -Wextra $(OPTFLAGS)
DEPFLAGS := -MMD -MP
LDFLAGS := -static -static-libgcc -static-libstdc++ -Wl,--no-insert-timestamp
LDLIBS := -lcomdlg32 -lcomctl32 -luser32 -lkernel32 -lgdi32 -lole32 -lshell32 -lwinmm -ld2d1 -ldwrite -loleacc -loleaut32 -luuid -lm

CPP_SRCS := $(wildcard src/*.cpp)
CPP_OBJS := $(patsubst src/%.cpp,$(OBJ_DIR)/%.o,$(CPP_SRCS))
MA_OBJ := $(OBJ_DIR)/miniaudio_impl.o
RC_OBJ := $(OBJ_DIR)/app.res.o
OBJS := $(CPP_OBJS) $(MA_OBJ) $(RC_OBJ)
TEST_SRCS := $(wildcard tests/*.c)
TEST_BINS := $(patsubst tests/%.c,$(TEST_DIR)/test-%.exe,$(TEST_SRCS))
CPP_TEST_SRCS := $(wildcard tests/*.cpp)
TEST_BINS += $(patsubst tests/%.cpp,$(TEST_DIR)/test-%.exe,$(CPP_TEST_SRCS))
TEST_LOGIC_OBJS := $(addprefix $(OBJ_DIR)/,playlist.o playlist_view.o m3u.o tags.o playback.o playlist_worker.o audio-test.o util.o miniaudio_impl.o ui_theme.o eq_dsp.o eq_state.o i18n.o language_selection.o runtime.o)
TEST_UI_OBJS := $(addprefix $(OBJ_DIR)/,skin.o plwnd.o mainwnd.o visualizer.o ui_draw.o ui_player.o eqwnd.o theme_window.o ui_tooltip.o ui_accessibility.o)
$(TEST_DIR)/test-test_playlist_window.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_playlist_window.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_player_window.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_player_window.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_ui_draw.exe: TEST_EXTRA_OBJS := $(OBJ_DIR)/ui_draw.o
$(TEST_DIR)/test-test_ui_draw.exe: $(OBJ_DIR)/ui_draw.o
$(TEST_DIR)/test-test_themes.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_themes.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_themes.exe: LDLIBS += -lpsapi
$(TEST_DIR)/test-test_i18n.exe: LDLIBS += -lpsapi
$(TEST_DIR)/test-test_language_selection.exe: LDLIBS += -lpsapi
$(TEST_DIR)/test-test_tooltip.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_tooltip.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_tooltip.exe: LDLIBS += -lpsapi -limm32
$(TEST_DIR)/test-test_language_window.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_language_window.exe: $(TEST_UI_OBJS) $(RC_OBJ)
$(TEST_DIR)/test-test_language_window.exe: TEST_EXTRA_OBJS += $(RC_OBJ)
$(TEST_DIR)/test-test_i18n_window.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_i18n_window.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_eq_window.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_eq_window.exe: $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_window_integration.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_window_integration.exe: $(TEST_UI_OBJS)

$(TEST_DIR)/test-test_m8_remaining.exe: TEST_EXTRA_OBJS := $(TEST_UI_OBJS)
$(TEST_DIR)/test-test_m8_remaining.exe: $(TEST_UI_OBJS)

all: $(TARGET) locales

# Runtime and developer executables each resolve locales beside themselves.
# Copy every catalog (also when a source is restored with an older timestamp).
locales:
	@mkdir -p $(BUILD_DIR)/locales
	@cp -r locales/. $(BUILD_DIR)/locales/

test-locales:
	@mkdir -p $(TEST_DIR)/locales
	@cp -r locales/. $(TEST_DIR)/locales/

diagnostic-locales:
	@mkdir -p $(DIAG_DIR)/locales
	@cp -r locales/. $(DIAG_DIR)/locales/

$(DIAG_DIR)/eq-probe.exe: scripts/diagnostics/eq_probe.cpp $(TEST_LOGIC_OBJS) $(TEST_UI_OBJS)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DLITESIC_TEST $(LDFLAGS) -o $@ $< $(TEST_LOGIC_OBJS) $(TEST_UI_OBJS) $(LDLIBS) -lpsapi

# Pre-distribution performance audit probe (docs/PERFORMANCE-AUDIT.md).
$(DIAG_DIR)/perf-probe.exe: scripts/diagnostics/perf_probe.cpp $(TEST_LOGIC_OBJS) $(TEST_UI_OBJS) $(RC_OBJ)
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DLITESIC_TEST $(LDFLAGS) -o $@ $< $(TEST_LOGIC_OBJS) $(TEST_UI_OBJS) $(RC_OBJ) $(LDLIBS) -lpsapi

perf-probe: $(DIAG_DIR)/perf-probe.exe diagnostic-locales

eq-probe: $(DIAG_DIR)/eq-probe.exe diagnostic-locales
	@set -e; for mode in 6 0 1 2 3 4 5; do "$(DIAG_DIR)/eq-probe.exe" $$mode; done

$(TARGET): $(OBJS) Makefile
	@mkdir -p $(@D)
	$(CXX) $(LDFLAGS) -mwindows -municode -o $@ $(OBJS) $(LDLIBS)

$(OBJ_DIR)/%.o: src/%.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) $(DEPFLAGS) -c -o $@ $<

$(OBJ_DIR)/%.o: src/%.c Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -c -o $@ $<

$(RC_OBJ): res/app.rc res/app.manifest res/icon.ico Makefile
	@mkdir -p $(@D)
	$(WINDRES) -Ires -i $< -o $@

$(TEST_DIR)/test-%.exe: tests/%.c $(OBJ_DIR)/util.o Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -MF $(@:.exe=.d) $(LDFLAGS) -o $@ $< $(OBJ_DIR)/util.o $(LDLIBS)

# O smoke test usa a implementacao real e a mesma configuracao do aplicativo.
$(TEST_DIR)/test-build_third_party_smoke.exe: tests/build_third_party_smoke.c $(MA_OBJ) Makefile
	@mkdir -p $(@D)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(DEPFLAGS) -MF $(@:.exe=.d) $(LDFLAGS) -o $@ $< $(MA_OBJ) $(LDLIBS)

# Audio offline usa a mesma unidade de producao, com acesso de teste ao engine.
$(OBJ_DIR)/audio-test.o: src/audio.cpp Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DLITESIC_TEST $(DEPFLAGS) -c -o $@ $<

$(TEST_DIR)/test-%.exe: tests/%.cpp $(TEST_LOGIC_OBJS) Makefile
	@mkdir -p $(@D)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -DLITESIC_TEST $(DEPFLAGS) -MF $(@:.exe=.d) $(LDFLAGS) -o $@ $< $(TEST_LOGIC_OBJS) $(TEST_EXTRA_OBJS) $(LDLIBS)

test: $(TEST_BINS) test-locales
	@set -e; for t in $(TEST_BINS); do echo "==> $$t"; "$$t"; done
	@echo "All tests passed."

run: all
	./$(TARGET)

clean:
	rm -f -- $(OBJS) $(OBJS:.o=.d) $(OBJ_DIR)/audio-test.o $(OBJ_DIR)/audio-test.d $(TEST_BINS) $(TEST_BINS:.exe=.d) $(DIAG_DIR)/eq-probe.exe $(DIAG_DIR)/perf-probe.exe $(TARGET)

-include $(CPP_OBJS:.o=.d) $(MA_OBJ:.o=.d) $(OBJ_DIR)/audio-test.d $(TEST_BINS:.exe=.d)
.PHONY: all test run clean locales test-locales diagnostic-locales perf-probe eq-probe
