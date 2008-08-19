objects = background.o character.o character_type.o color_utils.o custom_object.o custom_object_functions.o custom_object_type.o draw_scene.o draw_tile.o editor.o entity.o filesystem.o fluid.o font.o formula.o formula_function.o formula_tokenizer.o frame.o geometry.o item.o item_type.o joystick.o key.o level.o level_logic.o level_object.o load_level.o main.o message_dialog.o prop.o raster.o sound.o string_utils.o surface_cache.o surface_formula.o surface.o texture.o tile_map.o variant.o wml_node.o wml_parser.o wml_utils.o wml_writer.o 

formula_test_objects = filesystem.o formula_function.o formula_tokenizer.o string_utils.o variant.o wml_node.o wml_parser.o wml_utils.o wml_writer.o

%.o : src/%.cpp
	g++ -g -O2 -I/usr/local/include/boost-1_34 -I/sw/include/SDL -I/usr/X11R6/include -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -fthreadsafe-statics -c $<

game: $(objects)
	g++ -g -O2 -L/sw/lib -framework OpenGL -L/usr/X11R6/lib -lX11 -framework Cocoa -D_GNU_SOURCE=1 -D_REENTRANT -Wnon-virtual-dtor -Wreturn-type -L/usr/lib -lSDLmain -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex -lboost_thread-mt -fthreadsafe-statics *.o -o game

formula_test: $(formula_test_objects)
	g++ -O2 -g -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DUNIT_TEST_FORMULA -Wnon-virtual-dtor -Wreturn-type -L/usr/lib -lSDL -lGL -lGLU -lSDL_image -lSDL_ttf -lSDL_mixer -lboost_regex formula.cpp $(formula_test_objects) -o test

clean:
	rm *.o game
