# win32_layer -- the Win32 and DirectX surface of a 1990s Windows game,
# implemented over SDL3, as a static library.
#
#     make              build/libwin32_layer.a
#     make clean
#
# 32-bit only (i386): the programs it serves assume 4-byte pointers. Needs an
# i686-capable g++ and SDL3's i686 development files. Link an application as
#
#     g++ -m32 app.o build/libwin32_layer.a -lSDL3 -lpthread -lm
#
# and compile it with the same `-isystem include -include callconv.h`, so it
# sees this layer's windows.h, ddraw.h, dsound.h, ... instead of an SDK's.

CXX      ?= g++
CXXFLAGS ?= -O2 -g
FLAGS    := -m32 -Wall -Wextra -std=gnu++14 -fno-strict-aliasing \
            -fno-exceptions -fno-rtti -isystem include -include callconv.h
SRC      := $(wildcard src/*.cpp)
OBJ      := $(patsubst src/%.cpp,build/%.o,$(SRC)) build/tomlc99.o

.PHONY: all clean
all: build/libwin32_layer.a

build/%.o: src/%.cpp
	@mkdir -p build
	$(CXX) $(FLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

# tomlc99 (MIT, third_party/tomlc99): the registry file's parser, plain C.
build/tomlc99.o: third_party/tomlc99/toml.c
	@mkdir -p build
	$(CC) -m32 -O2 -g -c $< -o $@

build/libwin32_layer.a: $(OBJ)
	ar rcs $@ $^

clean:
	rm -rf build

-include $(OBJ:.o=.d)
