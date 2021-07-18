##
#
# Author: Nikolaus Mayer, 2021 
#
##

## Compiler
CXX ?= g++-10

CIMG_VERSION ?= 2.9.7
FLTK_VERSION ?= 1.3.6

## Compiler flags; extended in 'debug'/'release' rules
CXXFLAGS += -fPIC -W -Wall -Wextra -Wpedantic -std=c++20 -pthread -isystem src/agg-2.4/include -Dcimg_use_png -Dcimg_display=0 $(shell ./src/fltk-$(FLTK_VERSION)/install/bin/fltk-config --use-gl --cxxflags)

## Linker flags
LDFLAGS = -Lsrc/agg-2.4/src -pthread -lagg -lpng $(shell ./src/fltk-$(FLTK_VERSION)/install/bin/fltk-config --use-gl --ldflags)

## Default name for the built executable
TARGET = hanabi

## Every *.c/*.cc/*.cpp file is a source file
SOURCES = $(wildcard src/*.c src/*.cc src/*.cpp)
## Every *.h/*.hh/*.hpp file is a header file
HEADERS = $(wildcard *.h *.hh *.hpp)

## Build a *.o object file for every source file
OBJECTS = $(addsuffix .o, $(basename $(SOURCES)))

## External dependencies to be downloaded and built
OTHERS = CImg AGG FLTK


## Tell the 'make' program that e.g. 'make clean' is not supposed to 
## create a file 'clean'
##
## "Why is it called 'phony'?" -- because it's not a real target. That 
## is, the target name isn't a file that is produced by the commands 
## of that target.
.PHONY: all clean purge debug release $(OTHERS)


## Default is release build mode
all: release
	
## When in debug mode, don't optimize, and create debug symbols
debug: CXXFLAGS += -O0 -g -D_DEBUG
debug: BUILDMODE ?= DEBUG
debug: $(TARGET)
	
## When in release mode, optimize
release: CXXFLAGS += -O3 -DNDEBUG
release: BUILDMODE ?= RELEASE
release: $(TARGET)

## Remove built object files and the main executable
clean:
	$(info ... deleting built object files and executable  ...)
	-rm src/*.o $(TARGET)

purge:
	$(info ... deleting all downloaded, built, and generated files ...)
	-rm -r src/*.o $(TARGET) src/agg-2.4 src/fltk-1.3.6 src/CImg.h agg-2.4.tar.gz fltk-1.3.6-source.tar.gz v.2.9.7.tar.gz CImg-v.2.9.7

CImg: 
	$(info ... downloading CImg ...)
	test -f v.$(CIMG_VERSION).tar.gz || wget https://github.com/dtschump/CImg/archive/v.$(CIMG_VERSION).tar.gz
	test -d CImg-v.$(CIMG_VERSION) || tar xfz v.$(CIMG_VERSION).tar.gz
	test -f src/CImg.h || cp CImg-v.$(CIMG_VERSION)/CImg.h src/

AGG:
	$(info ... downloading and building AGG ...)
	test -f agg-2.4.tar.gz || wget https://github.com/nikolausmayer/AntiGrainGeometry-v2.4/raw/master/agg-2.4.tar.gz
	test -d src/agg-2.4 || ( tar xfz agg-2.4.tar.gz && mv agg-2.4 src/ )
	test -f src/agg-2.4/src/libagg.a || ( cd src/agg-2.4 && patch include/agg_scanline_u.h < ../AntiGrainGeometry-agg_scanline_u-patch.txt && patch include/agg_renderer_outline_aa.h < ../AntiGrainGeometry-agg_renderer_outline_aa-patch.txt && make -j4 )

FLTK:
	$(info ... downloading and building FLTK ...)
	test -f fltk-$(FLTK_VERSION)-source.tar.gz || wget https://www.fltk.org/pub/fltk/$(FLTK_VERSION)/fltk-$(FLTK_VERSION)-source.tar.gz 
	test -d src/fltk-$(FLTK_VERSION) || ( tar xfz fltk-$(FLTK_VERSION)-source.tar.gz && mv fltk-$(FLTK_VERSION) src/ )
	test -d src/fltk-$(FLTK_VERSION)/install || ( cd src/fltk-$(FLTK_VERSION) && ./configure --disable-xft --enable-shared --prefix=`pwd`/install && make -j4 && make install )

## The main executable depends on all object files of all source files
$(TARGET): $(OBJECTS) Makefile $(OTHERS)
	$(info ... linking $@ ...)
	$(CXX) $(OBJECTS) $(LDFLAGS) -o $@

## Every object file depends on its source. It may also depend on
## potentially all header files, and of course the makefile itself.
%.o: %.c Makefile $(HEADERS) $(OTHERS)
	$(info ... compiling $@ ...)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

%.o: %.cc Makefile $(HEADERS) $(OTHERS)
	$(info ... compiling $@ ...)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@

%.o: %.cpp Makefile $(HEADERS) $(OTHERS)
	$(info ... compiling $@ ...)
	$(CXX) $(CXXFLAGS) $(INCLUDE_DIRS) -c $< -o $@


