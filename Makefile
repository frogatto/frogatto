CC  = ccache gcc
CXX = ccache g++

OPT = -O2 -lX11 -lz -fno-inline-functions

PREFIX = /usr/local/share/frogatto

include Makefile.common

%.o : src/%.cpp
	$(CXX) $(CCFLAGS) -DIMPLEMENT_SAVE_PNG -fno-inline-functions `sdl-config --cflags` -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type -fthreadsafe-statics -g $(OPT) -c $<

game: $(objects)
	$(CXX) $(CCFLAGS) -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type $(objects) -o game -L. -L/sw/lib -L. -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lGLEW -lSDL_image -lSDL_ttf -lSDL_mixer -lpng -lboost_regex-mt -lboost_system-mt -lpthread -g $(OPT) -fthreadsafe-statics

server: $(server_objects)
	$(CXX) -fno-inline-functions -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Werror=return-type -fthreadsafe-statics $(server_objects) -o server -L/sw/lib -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt -g $(OPT)

formula_test: $(formula_test_objects)
	$(CXX) -O2 -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_FORMULA -Wnon-virtual-dtor -Werror=return-type src/formula.cpp $(formula_test_objects) -o test -L/usr/lib -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex

wml_modify_test: $(wml_modify_test_objects)
	$(CXX) -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_MODIFY -Wnon-virtual-dtor -Werror=return-type src/wml_modify.cpp $(wml_modify_test_objects) -o test -L/usr/lib -lboost_regex

wml_schema_test: $(wml_schema_test_objects)
	$(CXX) -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_SCHEMA -Wnon-virtual-dtor -Werror=return-type src/wml_schema.cpp $(wml_schema_test_objects) -o test -L/usr/lib -lboost_regex

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

install:
	install -m 0755 game $(PREFIX)
	install -m 0644 UbuntuMono-R.ttf $(PREFIX)
	install -m 0644 README
	install -m 0644 CHANGELOG
    # XXX still needs work

clean:
	rm -f *.o game
