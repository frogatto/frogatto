CXX ?= ccache g++

# set to 'yes' to optimize code using -O2
OPTIMIZE=yes

ifeq ($(OPTIMIZE),yes)
BASE_CXXFLAGS += -O2
endif

BASE_CXXFLAGS += -g -fno-inline-functions -fthreadsafe-statics -Wnon-virtual-dtor -Werror=return-type

INC = $(shell pkg-config --cflags x11 sdl glu glew SDL_image libpng zlib)

# CPPFLAGS +=

LIBS = $(shell pkg-config --libs x11 ) -lSDLmain \
	$(shell pkg-config --libs sdl glu glew SDL_image libpng zlib) -lSDL_ttf -lSDL_mixer

# This is currently unused.
# LDFLAGS += -L.

include Makefile.common

%.o : src/%.cpp
	$(CXX) \
		$(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) -DIMPLEMENT_SAVE_PNG \
		-c $<

game: $(objects)
	$(CXX) \
		$(LDFLAGS) \
		$(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) $(INC) \
		$(objects) -o game \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lpthread -fthreadsafe-statics

server: $(server_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) \
		$(server_objects) -o server \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt

formula_test: $(formula_test_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(BASE_CXXFLAGS) $(CXXFLAGS) $(CPPFLAGS) -DUNIT_TEST_FORMULA \
		src/formula.cpp $(formula_test_objects) -o test \
		$(LIBS) -lboost_regex

update-pot:
	utils/make-pot.sh > po/frogatto.pot.bak
	msguniq -F po/frogatto.pot.bak > po/frogatto.pot
	rm po/frogatto.pot.bak

%.po: po/frogatto.pot
	msgmerge $@ po/frogatto.pot -o $@.part
	mv $@.part $@

LINGUAS=ar de el en_GB eo es fr hu_HU id it ja ms_MY nl pl pt_BR ru sk sv tt zh_CN

update-po:
	(for lang in ${LINGUAS}; do \
		${MAKE} po/$$lang.po ; \
	done)

update-mo:
	(for lang in ${LINGUAS}; do \
		mkdir -p locale/$$lang/LC_MESSAGES ; \
		msgfmt po/$$lang.po -o locale/$$lang/LC_MESSAGES/frogatto.mo ; \
	done)

clean:
	rm -f *.o game
	
assets:
	./game --utility=compile_levels
	./game --utility=compile_objects