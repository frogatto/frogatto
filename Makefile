CC  ?= ccache gcc
CXX ?= ccache g++

# set to 'yes' to optimize code using -O2
OPTIMIZE=yes

ifeq ($(OPTIMIZE),yes)
CXXFLAGS += -O2
endif

CXXFLAGS += -Wnon-virtual-dtor -fno-inline-functions `sdl-config --cflags` -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type -fthreadsafe-statics -g

CPPFLAGS += $(shell pkg-config --cflags sdl) \
	    -I/usr/include/boost \
	    -I/sw/include/SDL \
	    -Isrc/
LIBS += $(shell pkg-config --libs x11 ) \
	-lSDLmain -lSDL_ttf -lSDL_mixer \
	$(shell pkg-config --libs sdl glu glew SDL_image libpng zlib) \

LDFLAGS += -L/sw/lib \
	   -L/usr/lib \
	   -L.

include Makefile.common

%.o : src/%.cpp
	$(CXX) \
		$(CXXFLAGS) -fno-inline-functions -fthreadsafe-statics $(CPPFLAGS) -DIMPLEMENT_SAVE_PNG \
		-c $<

game: $(objects)
	$(CXX) \
		$(LDFLAGS) \
		$(CXXFLAGS) -fno-inline-functions $(CPPFLAGS) \
		$(objects) -o game \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lpthread -fthreadsafe-statics

server: $(server_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(CXXFLAGS) -fno-inline-functions -fthreadsafe-statics $(CPPFLAGS) \
		$(server_objects) -o server \
		$(LIBS) -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt

formula_test: $(formula_test_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(CXXFLAGS) $(CPPFLAGS) -DUNIT_TEST_FORMULA \
		src/formula.cpp $(formula_test_objects) -o test \
		$(LIBS) -lboost_regex

wml_modify_test: $(wml_modify_test_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(CXXFLAGS) -framework Cocoa $(CPPFLAGS) -DUNIT_TEST_WML_MODIFY \
		src/wml_modify.cpp $(wml_modify_test_objects) -o test \
		-lboost_regex

wml_schema_test: $(wml_schema_test_objects)
	$(CXX) \
		$(LDFLAGS) \
		$(CXXFLAGS) -framework Cocoa $(CPPFLAGS) -DUNIT_TEST_WML_SCHEMA \
		src/wml_schema.cpp $(wml_schema_test_objects) -o test \
		-lboost_regex

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