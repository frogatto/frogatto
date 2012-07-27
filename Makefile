#
# Main Makefile, intended for use on Linux/X11 and compatible platforms
# using GNU Make.
#
# It should guess the paths to the game dependencies on its own, except for
# Boost which is assumed to be installed to the default locations. If you have
# installed Boost to a non-standard location, you will need to override CXXFLAGS
# and LDFLAGS with any applicable -I and -L arguments.
#
# The main options are:
#
#   CCACHE           The ccache binary that should be used when USE_CCACHE is
#                     enabled (see below). Defaults to 'ccache'.
#   CXX              C++ compiler comand line.
#   CXXFLAGS         Additional C++ compiler options.
#   OPTIMIZE         If set to 'yes' (default), builds with compiler
#                     optimizations enabled (-O2). You may alternatively use
#                     CXXFLAGS to set your own optimization options.
#   LDFLAGS          Additional linker options.
#   USE_CCACHE       If set to 'yes' (default), builds using the CCACHE binary
#                     to run the compiler. If ccache is not installed (i.e.
#                     found in PATH), this option has no effect.
#

OPTIMIZE=yes
CCACHE?=ccache
USE_CCACHE?=$(shell which $(CCACHE) 2>&1 > /dev/null && echo yes)
ifneq ($(USE_CCACHE),yes)
CCACHE=
endif

ifeq ($(OPTIMIZE),yes)
BASE_CXXFLAGS += -O2
endif

# Initial compiler options, used before CXXFLAGS and CPPFLAGS.
BASE_CXXFLAGS += -g -fno-inline-functions -fthreadsafe-statics -Wnon-virtual-dtor -Werror=return-type

# Compiler include options, used after CXXFLAGS and CPPFLAGS.
INC := $(shell pkg-config --cflags x11 sdl glu glew SDL_image libpng zlib)

# Linker library options.
LIBS := $(shell pkg-config --libs x11 ) -lSDLmain \
	$(shell pkg-config --libs sdl glu glew SDL_image libpng zlib) -lSDL_ttf -lSDL_mixer

include Makefile.common

%.o : src/%.cpp
	$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG \
		-c $<

game: $(objects)
	$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) \
		$(objects) -o game \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lpthread -fthreadsafe-statics

server: $(server_objects)
	$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) \
		$(server_objects) -o server \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt

formula_test: $(formula_test_objects)
	$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) -DUNIT_TEST_FORMULA \
		src/formula.cpp $(formula_test_objects) -o test \
		$(LIBS) -lboost_regex

clean:
	rm -f *.o game
	
assets:
	./game --utility=compile_levels
	./game --utility=compile_objects
