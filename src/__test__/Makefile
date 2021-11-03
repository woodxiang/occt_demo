
CPPFLAGS += -g -std=c++17 

OpenCASCADE_MODULES := freetype TKRWMesh TKBinXCAF TKBin TKBinL TKOpenGl TKXCAF TKVCAF TKCAF TKV3d \
	TKHLR TKMesh TKService TKShHealing TKPrim TKTopAlgo TKGeomAlgo TKBRep TKGeomBase TKG3d TKG2d TKMath \
	TKLCAF TKCDF TKernel TKFillet TKBool TKBO TKOffset TKXSBase TKSTEPBase TKSTEPAttr TKSTEP TKSTEP209 TKSTL

LIBS:= $(foreach V, $(OpenCASCADE_MODULES),	-l$(V))

OpenCASCADE_INCLUDE_DIR := /usr/local/include/opencascade

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

all: stl_file_test RWStl_test
	@echo $(MAKE_VERSION)

stl_file_test.o:stl_file_test.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

stl_file.o: ../stl_file.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

help_algorithms.o: ../help_algorithms.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

stl_file_test: stl_file_test.o stl_file.o help_algorithms.o
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ $^

RWStl_Stream_Reader.o:../RWStl_Stream_Reader.cpp
	$(CXX) $(CPPFLAGS) -c -I$(OpenCASCADE_INCLUDE_DIR) -o $@ $< 

RWStl_test.o:RWStl_test.cpp
	$(CXX) $(CPPFLAGS) -c -I$(OpenCASCADE_INCLUDE_DIR) -o $@ $<

RWStl_test:RWStl_test.o RWStl_Stream_Reader.o
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	$(RM) -r *.o ../*.o *_test

.PHONY: all clean

print-%: ; @echo $* = $($*)
