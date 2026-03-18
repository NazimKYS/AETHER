ifndef VERBOSE
	QUIET:=@
endif


SOURCEDIR    = src
BUILDDIR     = build

LLVM_CONFIG?=llvm-config-18
#LLVM_CONFIG?='-I/home/nazim/llvm-project/llvm/include -I/home/nazim/llvm-project/build/include -std=c++14 -fno-exceptions -D_GNU_SOURCE -D_DEBUG -D__STDC_CONSTANT_MACROS -D__STDC_FORMAT_MACROS -D__STDC_LIMIT_MACROS'
LDFLAGS+=$(shell $(LLVM_CONFIG) --ldflags)
LDFLAGS+=-ldl -ltinfo -lpthread
LDFLAGS += -L/usr/lib/llvm-18/lib -ldl -ltinfo -lpthread
#COMMON_FLAGS=-Wall  -Wextra -fexceptions 
COMMON_FLAGS= -Wextra -fexceptions 

LLVM_PREFIX   := $(shell $(LLVM_CONFIG) --prefix)
CLANG_VERSION := $(shell $(LLVM_CONFIG) --version | cut -d. -f1)

CXXFLAGS+=$(COMMON_FLAGS) $(shell $(LLVM_CONFIG) --cxxflags)
CPPFLAGS+=$(shell $(LLVM_CONFIG) --cppflags) -I$(SOURCEDIR)
CPPFLAGS += -I/usr/lib/llvm-18/include
CPPFLAGS += -DLLVM_PREFIX='"$(LLVM_PREFIX)"' -DCLANG_VERSION_STRING='"$(CLANG_VERSION)"'
# For debug builds: 
# SANITIZE_FLAGS = -fsanitize=address -fno-omit-frame-pointer -g
#temporarily disable AddressSanitizer for debugging (ASan can interfere with breakpoints and stack traces):
# SANITIZE_FLAGS = -fsanitize=address -fno-omit-frame-pointer -g
# CXXFLAGS += $(SANITIZE_FLAGS)
# LDFLAGS += $(SANITIZE_FLAGS)

DEBUG ?= 1
ifeq ($(DEBUG),1)
  CXXFLAGS += -g -O0 -fno-omit-frame-pointer
  # No ASan during debugging
endif

#CPPFLAGS+=-I/home/nazim/llvm-project/build/lib/clang/11.1.0/include/
PYTHONLIB= \
-I/usr/include/python3.6 \
 -lpython3.6 

CLANGLIBS = -lclang-cpp
LLVMLIBS=$(shell $(LLVM_CONFIG) --libs --system-libs )



SOURCE    = main 
HEAD      = main

HEADER = $(join $(addsuffix $(SOURCEDIR)/, $(dir $(HEAD))), $(notdir $(HEAD:=.h)))
OBJECT = $(join $(addsuffix $(BUILDDIR)/, $(dir $(SOURCE))), $(notdir $(SOURCE:=.o)))

EXECUTABLE = aether

all: default

default: directory $(OBJECT)
	@echo Linking Project
	@echo $(CXX) -o $(EXECUTABLE) $(CXXFLAGS) $(OBJECT) $(CLANGLIBS) $(LLVMLIBS) $(LDFLAGS)
	$(QUIET)$(CXX) -o $(EXECUTABLE) $(CXXFLAGS) $(OBJECT) $(CLANGLIBS) $(LLVMLIBS) $(LDFLAGS)

directory:
	mkdir -p $(BUILDDIR) 

# Track all .cpp and .h files in src/ so editing any of them triggers a recompile
SRC_DEPS := $(wildcard $(SOURCEDIR)/*.cpp $(SOURCEDIR)/*.h)

$(BUILDDIR)/%.o : $(SOURCEDIR)/%.cpp $(SRC_DEPS)
	@echo Compiling $*.cpp
	$(QUIET)$(CXX) -o $@ -c $(CPPFLAGS) $(CXXFLAGS) $<


UNIT_TEST_BIN = tests/unit_tests

unit-test: $(UNIT_TEST_BIN)
	@echo Running unit tests...
	$(UNIT_TEST_BIN)

$(UNIT_TEST_BIN): tests/unit_tests.cpp $(SRC_DEPS) | directory
	@echo Compiling unit_tests.cpp
	$(QUIET)$(CXX) -o $@ $(CPPFLAGS) $(CXXFLAGS) $< $(CLANGLIBS) $(LLVMLIBS) $(LDFLAGS)

test: unit-test
	@echo Running integration tests...
	bash tests/run_tests.sh

clean:
	$(QUIET)rm -rf $(EXECUTABLE) $(BUILDDIR) $(UNIT_TEST_BIN)
