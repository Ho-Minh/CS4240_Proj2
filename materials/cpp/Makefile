# Makefile equivalent of the provided CMakeLists.txt

# Toolchain
CXX      := g++
AR       := ar
ARFLAGS  := rcs

# Dirs
SRCDIR   := src
INCDIR   := include
BUILDDIR := build/obj
BINDIR   := bin

# Flags
CXXFLAGS := -std=c++17 -I$(INCDIR) -O2 -Wall -Wextra -Wpedantic -MMD -MP

# Sources
LIB_SRC := \
  $(SRCDIR)/ir_core.cpp \
  $(SRCDIR)/ir_types.cpp \
  $(SRCDIR)/ir_reader.cpp \
  $(SRCDIR)/ir_cfg.cpp \
  $(SRCDIR)/reaching_def.cpp \
  $(SRCDIR)/dead_code.cpp \
  $(SRCDIR)/ir_optimizer.cpp

IR_OPT_SRC := $(SRCDIR)/ir_optimizer_demo.cpp
CFG_TEST_SRC := $(SRCDIR)/cfg_test.cpp

# Objects
LIB_OBJ      := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(LIB_SRC))
IR_OPT_OBJ   := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(IR_OPT_SRC))
CFG_TEST_OBJ := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(CFG_TEST_SRC))

# Targets
LIB_NAME := libircpp.a
LIB_PATH := $(BINDIR)/$(LIB_NAME)

IR_OPT_BIN := $(BINDIR)/ir_optimizer
CFG_TEST_BIN := $(BINDIR)/cfg_test

.PHONY: all clean dirs
all: dirs $(LIB_PATH) $(IR_OPT_BIN) $(CFG_TEST_BIN)

dirs:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Library
$(LIB_PATH): $(LIB_OBJ)
	$(AR) $(ARFLAGS) $@ $^

# Executables
$(IR_OPT_BIN): $(IR_OPT_OBJ) $(LIB_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $(IR_OPT_OBJ) $(LIB_PATH)

$(CFG_TEST_BIN): $(CFG_TEST_OBJ) $(LIB_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $(CFG_TEST_OBJ) $(LIB_PATH)

# Compile rules
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Deps
-include $(LIB_OBJ:.o=.d) $(IR_OPT_OBJ:.o=.d) $(CFG_TEST_OBJ:.o=.d)

# Cleanup
clean:
	$(RM) -r $(BUILDDIR) $(BINDIR)
