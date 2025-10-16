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
  $(SRCDIR)/mips_instructions.cpp \
  $(SRCDIR)/register_manager.cpp \
  $(SRCDIR)/instruction_selector.cpp \
  $(SRCDIR)/arithmetic_selector.cpp \
  $(SRCDIR)/branch_selector.cpp \
  $(SRCDIR)/memory_selector.cpp \
  $(SRCDIR)/call_selector.cpp \

# Executable sources
BIN_SRC := $(SRCDIR)/ir_to_mips.cpp


# Objects
LIB_OBJ      := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(LIB_SRC))
BIN_OBJ      := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(BIN_SRC))

# Targets
LIB_NAME := libircpp.a
LIB_PATH := $(BINDIR)/$(LIB_NAME)
IR_TO_MIPS_BIN := $(BINDIR)/ir_to_mips

.PHONY: all clean dirs
all: dirs $(LIB_PATH) $(IR_TO_MIPS_BIN)

dirs:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Library
$(LIB_PATH): $(LIB_OBJ)
	$(AR) $(ARFLAGS) $@ $^

# Executable
$(IR_TO_MIPS_BIN): $(BIN_OBJ) $(LIB_PATH)
	$(CXX) $(CXXFLAGS) -o $@ $(BIN_OBJ) $(LIB_PATH)


# Compile rules
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Deps
-include $(LIB_OBJ:.o=.d) $(BIN_OBJ:.o=.d)

# Cleanup
clean:
	$(RM) -r $(BUILDDIR) $(BINDIR)
