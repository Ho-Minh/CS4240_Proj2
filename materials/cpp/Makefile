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


# Objects
LIB_OBJ      := $(patsubst $(SRCDIR)/%.cpp,$(BUILDDIR)/%.o,$(LIB_SRC))

# Targets
LIB_NAME := libircpp.a
LIB_PATH := $(BINDIR)/$(LIB_NAME)

.PHONY: all clean dirs
all: dirs $(LIB_PATH)

dirs:
	@mkdir -p $(BUILDDIR) $(BINDIR)

# Library
$(LIB_PATH): $(LIB_OBJ)
	$(AR) $(ARFLAGS) $@ $^


# Compile rules
$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Deps
-include $(LIB_OBJ:.o=.d)

# Cleanup
clean:
	$(RM) -r $(BUILDDIR) $(BINDIR)
