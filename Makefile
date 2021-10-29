
BUILD_DEBUG:=false

lib1_SRCS := $(wildcard src/views/*.cpp)
lib1_OBJS := $(lib1_SRCS:.cpp=.o)

ifndef EMSDK
$(error EMSDK not defined)
endif

OpenCASCADE_INCLUDE_DIR :=$(EMSDK)/upstream/emscripten/cache/sysroot/include/opencascade
OpenCASCADE_LIB_DIR :=$(EMSDK)/upstream/emscripten/cache/sysroot/lib

freetyp_DIR := $(EMSDK)/upstream/emscripten/cache/sysroot/include/freetype
freetype_LIB_DIR := $(EMSDK)/upstream/emscripten/cache/sysroot/lib

OpenCASCADE_MODULES := freetype TKRWMesh TKBinXCAF TKBin TKBinL TKOpenGles TKXCAF TKVCAF TKCAF TKV3d TKHLR TKMesh TKService TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath TKLCAF TKCDF TKernel TKFillet TKBool TKBO TKOffset TKXSBase TKSTEPBase TKSTEPAttr TKSTEP TKSTEP209
RUNTIME_METHOD_NAMES := ccall,cwrap,lengthBytesUTF8
METHOD_NAMES := _main, _test

LIBS:= $(foreach V, $(OpenCASCADE_MODULES),	$(OpenCASCADE_LIB_DIR)/lib$(V).a)

EXPORT_METHODS = -s EXPORTED_FUNCTIONS='[$(METHOD_NAMES)]' -s EXPORTED_RUNTIME_METHODS='[$(RUNTIME_METHOD_NAMES)]' -s EXPORT_NAME='createOccViewerModule' -s MODULARIZE=1 -s MAX_WEBGL_VERSION=2 --ts-typings

EXTERN_POST_JS = --extern-post-js src/occt-webgl-viewer.js

CPPFLAGS += -std=c++17 -g -fdebug-compilation-dir="../"
CPPFLAGS += -I$(OpenCASCADE_INCLUDE_DIR)

%.o: src/%.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

all: js/demo_app.js
	@echo $(MAKE_VERSION)


js/demo_app.js: main.o model_factory.o stl_file.o help_algorithms.o $(lib1_OBJS)
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ main.o model_factory.o stl_file.o help_algorithms.o $(lib1_OBJS) -L$(OpenCASCADE_LIB_DIR) $(LIBS) -s ALLOW_MEMORY_GROWTH=1 $(EXPORT_METHODS) $(EXTERN_POST_JS) -s LLD_REPORT_UNDEFINED --bind

clean:
	$(RM) -r *.o js/*.wasm js/*.js src/views/*.o

.PHONY: all clean

print-%: ; @echo $* = $($*)
