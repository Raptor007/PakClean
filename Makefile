CXX := g++
CXXFLAGS := -g -Og -static -Wall -Wextra


SYSTEM = $(shell uname -s)

ifneq (,$(findstring MINGW,$(SYSTEM))$(findstring CYGWIN,$(SYSTEM)))
WIN32 = true
endif

EXE = PakClean

ifdef WIN32
EXE := $(EXE).exe
endif


$(EXE): PakClean.o
	$(CXX) $(CXXFLAGS) $< -o $@
	-chmod +x $@

.cpp.o:
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(EXE) *.o

.PHONY: default install clean
