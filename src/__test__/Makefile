
CPPFLAGS += -g -std=c++17 

%.o: %.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

all: stl_file_test
	@echo $(MAKE_VERSION)

stl_file_test.o:stl_file_test.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

stl_file.o: ../stl_file.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

help_algorithms.o: ../help_algorithms.cpp
	$(CXX) $(CPPFLAGS) -c -o $@ $<

stl_file_test: stl_file_test.o stl_file.o help_algorithms.o
	$(CXX) $(CPPFLAGS) $(CFLAGS) -o $@ stl_file_test.o stl_file.o help_algorithms.o

clean:
	$(RM) -r *.o ../*.o stl_file_test

.PHONY: all clean

print-%: ; @echo $* = $($*)
