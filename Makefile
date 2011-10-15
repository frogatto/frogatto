CC  = ccache gcc
CXX = ccache g++

OPT = -O2 -fno-inline-functions

include Makefile.common

%.o : src/%.cpp
	$(CXX) -DIMPLEMENT_SAVE_PNG -fno-inline-functions -g $(OPT) `sdl-config --cflags` -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -fthreadsafe-statics -c $<

game: $(objects)
	$(CXX) -g $(OPT) -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type $(objects) -o game -L. -L/sw/lib -L. -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lGLEW -lSDL_image -lSDL_ttf -lSDL_mixer -lpng -lboost_regex-mt -lboost_system-mt -lpthread -fthreadsafe-statics

server: $(server_objects)
	$(CXX) -fno-inline-functions -g $(OPT) -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -fthreadsafe-statics $(server_objects) -o server -L/sw/lib -L/usr/lib `sdl-config --libs` -lSDLmain -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex-mt -lboost_system-mt -lboost_thread-mt -lboost_iostreams-mt

formula_test: $(formula_test_objects)
	$(CXX) -O2 -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_FORMULA -Wnon-virtual-dtor -Wreturn-type src/formula.cpp $(formula_test_objects) -o test -L/usr/lib -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex

wml_modify_test: $(wml_modify_test_objects)
	$(CXX) -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_MODIFY -Wnon-virtual-dtor -Wreturn-type src/wml_modify.cpp $(wml_modify_test_objects) -o test -L/usr/lib -lboost_regex

wml_schema_test: $(wml_schema_test_objects)
	$(CXX) -O2 -g -framework Cocoa -I/usr/local/include/boost-1_34 -I/sw/include/SDL -Isrc/ -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_WML_SCHEMA -Wnon-virtual-dtor -Wreturn-type src/wml_schema.cpp $(wml_schema_test_objects) -o test -L/usr/lib -lboost_regex

update-pot:
	utils/make-pot.sh > po/frogatto.pot

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
