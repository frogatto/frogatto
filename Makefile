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

SDL2_CONFIG?=sdl2-config
USE_SDL2?=$(shell which $(SDL2_CONFIG) 2>&1 > /dev/null && echo yes)

# Initial compiler options, used before CXXFLAGS and CPPFLAGS.
BASE_CXXFLAGS += -g -fno-inline-functions -fthreadsafe-statics -Wnon-virtual-dtor -Werror -Wignored-qualifiers -Wformat -Wswitch -DUSE_GLES2 -DUTILITY_IN_PROC

# Compiler include options, used after CXXFLAGS and CPPFLAGS.
ifeq ($(USE_SDL2),yes)
INC := -Isrc -Isrc/server $(shell pkg-config --cflags x11 sdl2 glu glew SDL2_image libpng zlib)
else
INC := -Isrc -Isrc/server $(shell pkg-config --cflags x11 sdl glu glew SDL_image libpng zlib)
endif

# Linker library options.
ifeq ($(USE_SDL2),yes)
LIBS := $(shell pkg-config --libs x11 ) -lSDL2main \
	$(shell pkg-config --libs sdl2 glu glew SDL2_image libpng zlib) -lSDL2_ttf -lSDL2_mixer
else
LIBS := $(shell pkg-config --libs x11 ) -lSDLmain \
	$(shell pkg-config --libs sdl glu glew SDL_image libpng zlib) -lSDL_ttf -lSDL_mixer
endif

include Makefile.common

src/%.o : src/%.cpp
	@echo "Building:" $<
	@$(CCACHE) $(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG -c -o $@ $<
	@$(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG -MM $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|src/$*.o:|' < $*.d.tmp > src/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> src/$*.d
	@rm -f $*.d.tmp

src/server/%.o : src/server/%.cpp
	@echo "Building: " $<
	@$(CCACHE) $(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG -c -o $@ $<
	@$(CXX) $(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG -MM $< > $*.d
	@mv -f $*.d $*.d.tmp
	@sed -e 's|.*:|src/server/$*.o:|' < $*.d.tmp > src/server/$*.d
	@sed -e 's/.*://' -e 's/\\$$//' < $*.d.tmp | fmt -1 | \
		sed -e 's/^ *//' -e 's/$$/:/' >> src/server/$*.d
	@rm -f $*.d.tmp

game: $(objects)
	@echo "Linking : game"
	@$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) \
		$(objects) -o game \
		$(LIBS) -lboost_regex -lboost_system -lpthread -fthreadsafe-statics

# pull in dependency info for *existing* .o files
-include $(objects:.o=.d)

server: $(server_objects)
	$(CCACHE) $(CXX) \
		$(BASE_CXXFLAGS) $(LDFLAGS) $(CXXFLAGS) $(CPPFLAGS) \
		$(server_objects) -o server \
		$(LIBS) -lboost_regex -lboost_system -lboost_thread -lboost_iostreams

clean:
	rm -f src/*.o src/*.d src/server/*.o src/server/*.d *.o *.d game server
	
assets:
	./game --utility=compile_levels
	./game --utility=compile_objects
