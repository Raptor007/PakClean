SYSTEM = $(shell uname -s)

ifneq (,$(findstring MINGW,$(SYSTEM))$(findstring CYGWIN,$(SYSTEM)))
WIN32 = true
endif

EXE = PakClean

ifdef WIN32
EXE := $(EXE).exe
endif


$(EXE): PakClean.o
	g++ -g -static $< -o $@
	-chmod +x $@

.cpp.o:
	g++ -g -c $< -o $@

clean:
	rm -f $(EXE) *.o

.PHONY: default install clean
