# Project: cvimext
# Generates cvimext.dll with gcc.
# Can be used for Cygwin and MingW (MingW ignores -mno-cygwin)
#
# Originally, the DLL base address was fixed: -Wl,--image-base=0x1C000000
# Now it is allocated dymanically by the linker by evaluating all DLLs
# already loaded in memory. The binary image contains as well information
# for automatic pseudo-rebasing, if needed by the system. ALV 2004-02-29

# If cross-compiling set this to yes, else set it to no
CROSS = no
#CROSS = yes
# For the old MinGW 2.95 (the one you get e.g. with debian woody)
# set the following variable to yes and check if the executables are
# really named that way.
# If you have a newer MinGW or you are using cygwin set it to no and
# check also the executables
MINGWOLD = no

ifeq ($(CROSS),yes)
DEL = rm
ifeq ($(MINGWOLD),yes)
CXX = i586-mingw32msvc-g++
CXXFLAGS := -O2 -mno-cygwin -fvtable-thunks
WINDRES = i586-mingw32msvc-windres
else
CXX = i386-mingw32msvc-g++
CXXFLAGS := -O2 -mno-cygwin
WINDRES = i386-mingw32msvc-windres
endif
else
CXX := g++
WINDRES := windres
CXXFLAGS := -O2 -mno-cygwin
ifneq (sh.exe, $(SHELL))
DEL = rm
else
DEL = del
endif
endif
LIBS :=  -luuid
RES  := cvimext.res
DEFFILE = cvimext_ming.def
OBJ  := cvimext.o

DLL  := cvimext.dll

.PHONY: all all-before all-after clean clean-custom

all: all-before $(DLL) all-after

$(DLL): $(OBJ) $(RES) $(DEFFILE)
	$(CXX) -shared $(CXXFLAGS) -s -o $@ \
		-Wl,--enable-auto-image-base \
		-Wl,--enable-auto-import \
		-Wl,--whole-archive \
			$^ \
		-Wl,--no-whole-archive \
			$(LIBS)

cvimext.o: cvimext.cpp
	$(CXX) $(CXXFLAGS) -DFEAT_GETTEXT -c $? -o $@

$(RES): cvimext_ming.rc
	$(WINDRES) --input-format=rc --output-format=coff -DMING $? -o $@

clean: clean-custom
	-$(DEL)  $(OBJ) $(RES) $(DLL)
